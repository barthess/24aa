/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

    This file is part of 24AA lib.

    24AA lib is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    24AA lib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstring>
#include <cstdlib>

#include "ch.hpp"

#include "mtd24.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#if defined(SAM7_PLATFORM)
#define EEPROM_I2C_CLOCK (MCK / (((this->i2cp->config->cwgr & 0xFF) + ((this->cfg->i2cp->config->cwgr >> 8) & 0xFF)) * (1 << ((this->cfg->i2cp->config->cwgr >> 16) & 7)) + 6))
#else
#define EEPROM_I2C_CLOCK (this->i2cp->config->clock_speed)
#endif
/*
 ******************************************************************************
 * EXTERNS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * PROTOTYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */
/**
 * @brief     Calculates requred timeout.
 */
systime_t Mtd24::calc_timeout(size_t bytes) {
  const uint32_t bitsinbyte = 10;
  uint32_t tmo;
  tmo = ((bytes + 1) * bitsinbyte * 1000);
  tmo /= EEPROM_I2C_CLOCK;
  tmo += 10; /* some additional milliseconds to be safer */
  return MS2ST(tmo);
}

/**
 *
 */
msg_t Mtd24::i2c_read(uint8_t *rxbuf, size_t len,
                   uint8_t *writebuf, size_t preamble_len) {

#if defined(STM32F1XX_I2C)
#error "Ugly workaround for single byte reading is not implemented yet"
  if (1 == len)
    return stm32_f1x_read_single_byte(rxbuf, offset);
#endif /* defined(STM32F1XX_I2C) */

  msg_t status;
  systime_t tmo = calc_timeout(len + preamble_len);
  osalDbgCheck((nullptr != rxbuf) && (0 != len));

#if I2C_USE_MUTUAL_EXCLUSION
  i2cAcquireBus(this->i2cp);
#endif

  status = i2cMasterTransmitTimeout(this->i2cp, this->addr,
                    writebuf, preamble_len, rxbuf, len, tmo);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(this->i2cp);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cReleaseBus(this->i2cp);
#endif

  return status;
}

/**
 *
 */
msg_t Mtd24::i2c_write(const uint8_t *txdata, size_t len,
                      uint8_t *writebuf, size_t preamble_len) {

#if defined(STM32F1XX_I2C)
#error "Ugly workaround for single byte reading is not implemented yet"
  if (1 == len)
    return stm32_f1x_read_single_byte(txdata, offset);
#endif /* defined(STM32F1XX_I2C) */

  msg_t status;
  systime_t tmo = calc_timeout(len + preamble_len);

  if ((nullptr != txdata) && (0 != len))
    memcpy(&writebuf[preamble_len], txdata, len);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cAcquireBus(this->i2cp);
#endif

  status = i2cMasterTransmitTimeout(this->i2cp, this->addr,
                  writebuf, preamble_len + len, nullptr, 0, tmo);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(this->i2cp);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cReleaseBus(this->i2cp);
#endif

  return status;
}

/**
 *
 */
void Mtd24::wait_op_complete(void) {
  if (0 == cfg.programtime)
    return;
  else
    osalThreadSleep(cfg.programtime);
}

/*
 * Ugly workaround.
 * Stupid I2C cell in STM32F1x does not allow to read single byte.
 * So we must read 2 bytes and return needed one.
 */
msg_t Mtd24::stm32_f1x_read_single_byte(const uint8_t *txbuf, size_t txbytes,
                                         uint8_t *rxbuf, systime_t tmo) {
  uint8_t tmp[2];
  msg_t status;

  osalSysHalt("Unrealized untested");
  status = i2cMasterTransmitTimeout(this->i2cp, this->addr,
                                    txbuf, txbytes, tmp, 2, tmo);
  rxbuf[0] = tmp[0];
  return status;
}

/**
 * @brief   Accepts data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd24::bus_write(const uint8_t *txdata, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgCheck((this->writebuf_size - cfg.addr_len) >= len);
  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  this->acquire();

  /* write preamble. Only address bytes for this memory type */
  addr2buf(writebuf, offset, cfg.addr_len);
  status = i2c_write(txdata, len, writebuf, cfg.addr_len);

  wait_op_complete();
  this->release();

  if (status == MSG_OK)
    return len;
  else
    return 0;
}

/**
 *
 */
size_t Mtd24::bus_read(uint8_t *rxbuf, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
  osalDbgCheck(this->writebuf_size >= cfg.addr_len);

  this->acquire();
  addr2buf(writebuf, offset, cfg.addr_len);
  status = i2c_read(rxbuf, len, writebuf, cfg.addr_len);
  this->release();

  if (MSG_OK == status)
    return len;
  else
    return 0;
}

/**
 *
 */
msg_t Mtd24::bus_erase(void) {
  msg_t status;

  this->acquire();

  size_t blocksize = (this->writebuf_size - cfg.addr_len);
  size_t total_writes = capacity() / blocksize;

  osalDbgAssert(0 == (capacity() % blocksize),
      "Capacity must be divided by block size without remainder");

  memset(writebuf, 0xFF, writebuf_size);
  status = MSG_RESET;
  for (size_t i=0; i<total_writes; i++) {
    addr2buf(writebuf, i * blocksize, cfg.addr_len);
    status = i2c_write(nullptr, 0, writebuf, this->writebuf_size);
    wait_op_complete();
    if (MSG_OK != status)
      break;
  }

  this->release();
  return status;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd24::Mtd24(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size,
                                            I2CDriver *i2cp, i2caddr_t addr) :
Mtd(cfg, writebuf,  writebuf_size),
i2cp(i2cp),
addr(addr)
{
  return;
}

} /* namespace */
