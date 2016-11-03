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

#include "mtd_24aa.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

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
static systime_t calc_timeout(size_t bytes, uint32_t clock) {
  const uint32_t bitsinbyte = 10;
  uint32_t tmo_ms;

  tmo_ms = ((bytes + 1) * bitsinbyte * 1000);
  tmo_ms /= clock;
  tmo_ms += 10; /* some additional milliseconds to be safer */
  return MS2ST(tmo_ms);
}

/**
 *
 */
msg_t Mtd24aa::i2c_read(uint8_t *rxbuf, size_t len,
                   uint8_t *writebuf, size_t preamble_len) {

#if defined(STM32F1XX_I2C)
#error "Ugly workaround for single byte reading is not implemented yet"
  if (1 == len)
    return stm32_f1x_read_single_byte(rxbuf, offset);
#endif /* defined(STM32F1XX_I2C) */

  msg_t status;
  systime_t tmo = calc_timeout(len + preamble_len, this->bus_clk);
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
msg_t Mtd24aa::i2c_write(const uint8_t *txdata, size_t len,
                      uint8_t *writebuf, size_t preamble_len) {

#if defined(STM32F1XX_I2C)
#error "Ugly workaround for single byte reading is not implemented yet"
  if (1 == len)
    return stm32_f1x_read_single_byte(txdata, offset);
#endif /* defined(STM32F1XX_I2C) */

  msg_t status;
  systime_t tmo = calc_timeout(len + preamble_len, this->bus_clk);

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
bool Mtd24aa::wait_op_complete(void) {
  if (0 != cfg.programtime)
    osalThreadSleep(cfg.programtime);

  return OSAL_SUCCESS;
}

/*
 * Ugly workaround.
 * Stupid I2C cell in STM32F1x does not allow to read single byte.
 * So we must read 2 bytes and return needed one.
 */
msg_t Mtd24aa::stm32_f1x_read_single_byte(const uint8_t *txbuf, size_t txbytes,
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
size_t Mtd24aa::bus_write(const uint8_t *txdata, size_t len, uint32_t offset) {
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
size_t Mtd24aa::bus_read(uint8_t *rxbuf, size_t len, uint32_t offset) {
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

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd24aa::Mtd24aa(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size,
                                            I2CDriver *i2cp, i2caddr_t addr) :
MtdBase(cfg, writebuf,  writebuf_size),
i2cp(i2cp),
addr(addr),
bus_clk(cfg.bus_clk)
{
  return;
}

} /* namespace */
