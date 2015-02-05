/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012,2013,2014 Uladzimir Pylinski aka barthess

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

#include "mtd.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#if defined(SAM7_PLATFORM)
#define EEPROM_I2C_CLOCK (MCK / (((this->cfg->i2cp->config->cwgr & 0xFF) + ((this->cfg->i2cp->config->cwgr >> 8) & 0xFF)) * (1 << ((this->cfg->i2cp->config->cwgr >> 16) & 7)) + 6))
#else
#define EEPROM_I2C_CLOCK (this->cfg->i2cp->config->clock_speed)
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
 * @brief   Split one uint16_t address to two uint8_t.
 *
 * @param[in] txbuf pointer to driver transmit buffer
 * @param[in] addr  internal EEPROM device address
 */
void Mtd::split_addr(uint8_t *txbuf, size_t addr){
  txbuf[0] = (addr >> 8) & 0xFF;
  txbuf[1] = addr & 0xFF;
}

/**
 * @brief     Calculates requred timeout.
 */
systime_t Mtd::calc_timeout(size_t txbytes, size_t rxbytes){
  const uint32_t bitsinbyte = 10;
  uint32_t tmo;
  tmo = ((txbytes + rxbytes + 1) * bitsinbyte * 1000);
  tmo /= EEPROM_I2C_CLOCK;
  tmo += 10; /* some additional milliseconds to be safer */
  return MS2ST(tmo);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd::Mtd(const MtdConfig *cfg) :
cfg(cfg),
i2cflags(I2C_NO_ERROR)
#if (MTD_USE_MUTUAL_EXCLUSION && !CH_CFG_USE_MUTEXES)
  ,semaphore(1)
#endif
{
  return;
}

/**
 *
 */
void Mtd::acquire(void) {
#if MTD_USE_MUTUAL_EXCLUSION
  #if CH_CFG_USE_MUTEXES
    mutex.lock();
  #elif CH_CFG_USE_SEMAPHORES
    semaphore.wait();
  #endif
#endif /* MTD_USE_MUTUAL_EXCLUSION */
}

/**
 *
 */
void Mtd::release(void) {
#if MTD_USE_MUTUAL_EXCLUSION
  #if CH_CFG_USE_MUTEXES
    mutex.unlock();
  #elif CH_CFG_USE_SEMAPHORES
    semaphore.signal();
  #endif
#endif /* MTD_USE_MUTUAL_EXCLUSION */
}

/**
 *
 */
msg_t Mtd::read(uint8_t *data, size_t len, size_t offset){

  msg_t status = MSG_RESET;

#if defined(STM32F1XX_I2C)
  if (1 == len)
    return stm32_f1x_read_byte(data, offset);
#endif /* defined(STM32F1XX_I2C) */

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  this->acquire();
  split_addr(writebuf, offset);
  status = busReceive(data, len);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(cfg->i2cp);
  this->release();

  return status;
}

/**
 * @brief   Write data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd::write_impl(const uint8_t *data, size_t len, size_t offset){
  msg_t status = MSG_RESET;

  osalDbgCheck((sizeof(writebuf) - NVRAM_ADDRESS_BYTES) >= len);

  mtd_led_on();
  this->acquire();

  split_addr(writebuf, offset);                         /* store address bytes */
  memcpy(&(writebuf[NVRAM_ADDRESS_BYTES]), data, len);  /* store data bytes */
  status = busTransmit(writebuf, len+NVRAM_ADDRESS_BYTES);

  wait_for_sync();
  this->release();
  mtd_led_off();

  if (status == MSG_OK)
    return len;
  else
    return 0;
}

/**
 *
 */
msg_t Mtd::shred_impl(uint8_t pattern){
  msg_t status = MSG_RESET;

  mtd_led_on();
  this->acquire();

  size_t blocksize = (sizeof(writebuf) - NVRAM_ADDRESS_BYTES);
  size_t total_writes = capacity() / blocksize;

  memset(writebuf, pattern, sizeof(writebuf));
  for (size_t i=0; i<total_writes; i++){
    split_addr(writebuf, i * blocksize);
    status = busTransmit(writebuf, sizeof(writebuf));
    osalDbgCheck(MSG_OK == status);
    wait_for_sync();
  }

  this->release();
  mtd_led_off();
  return MSG_OK;
}

/**
 *
 */
msg_t Mtd::busReceive(uint8_t *data, size_t len){

  msg_t status = MSG_RESET;
  systime_t tmo = calc_timeout(NVRAM_ADDRESS_BYTES, len);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cAcquireBus(cfg->i2cp);
#endif

  status = i2cMasterTransmitTimeout(cfg->i2cp, cfg->addr, writebuf,
                                    NVRAM_ADDRESS_BYTES, data, len, tmo);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(cfg->i2cp);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cReleaseBus(cfg->i2cp);
#endif

  return status;
}

/**
 *
 */
msg_t Mtd::busTransmit(const uint8_t *data, size_t len){

  msg_t status = MSG_RESET;
  systime_t tmo = calc_timeout(len + NVRAM_ADDRESS_BYTES, 0);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cAcquireBus(cfg->i2cp);
#endif

  status = i2cMasterTransmitTimeout(cfg->i2cp, cfg->addr, data, len, NULL, 0, tmo);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(cfg->i2cp);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cReleaseBus(cfg->i2cp);
#endif

  return status;
}

/*
 * Ugly workaround.
 * Stupid I2C cell in STM32F1x does not allow to read single byte.
 * So we must read 2 bytes and return needed one.
 */
msg_t Mtd::stm32_f1x_read_byte(uint8_t *buf, size_t absoffset){
  uint8_t tmp[2];
  msg_t ret;

  this->acquire();

  /* if NOT last byte of file requested */
  if ((absoffset - 1) < capacity()){
    ret = read(tmp, absoffset, 2);
    buf[0] = tmp[0];
  }
  else{
    ret = read(tmp, absoffset - 1, 2);
    buf[0] = tmp[1];
  }
  this->release();

  return ret;
}

#if NVRAM_FS_USE_DELETE_AND_RESIZE

/**
 *
 */
msg_t Mtd::move_left(size_t len, size_t blkstart, size_t shift) {

  const size_t N = 16;
  uint8_t buf[N];
  size_t dst, src, progress, tail;
  msg_t ret;

  osalDbgAssert((blkstart >= shift), "MTD underflow");
  osalDbgCheck(len > 0);

  if (0 == shift)
    return MSG_OK; /* nothing to do */

  /* process blocks */
  progress = N;
  while (progress < len) {
    src  = blkstart + progress - N;
    dst = src - shift;

    ret = read(buf, src, N);
    if (MSG_OK != ret)
      return ret;

    ret = write(buf, dst, N);
    if (MSG_OK != ret)
      return ret;

    progress += N;
  }

  /* process tail (if any) */
  tail = len % N;
  if (tail > 0) {
    src  = blkstart + len - tail;
    dst = src - shift;

    ret = read(buf, src, tail);
    if (MSG_OK != ret)
      return ret;

    ret = write(buf, dst, tail);
    if (MSG_OK != ret)
      return ret;
  }

  return MSG_OK;
}

/**
 *
 */
msg_t Mtd::move_right(size_t len, size_t blkstart, size_t shift) {
  const size_t N = 16;
  uint8_t buf[N];
  size_t dst, src, progress, tail;
  msg_t ret;

  osalDbgAssert((len + blkstart + shift) < capacity(), "MTD overflow");
  osalDbgCheck(len > 0);

  if (0 == shift)
    return MSG_OK; /* nothing to do */

  /* process blocks */
  progress = N;
  while (progress < len) {
    src  = blkstart + len - progress;
    dst = src + shift;

    ret = read(buf, src, N);
    if (MSG_OK != ret)
      return ret;

    ret = write(buf, dst, N);
    if (MSG_OK != ret)
      return ret;

    progress += N;
  }

  /* process tail (if any) */
  tail = len % N;
  if (tail > 0) {
    src  = blkstart;
    dst = src + shift;

    ret = read(buf, src, tail);
    if (MSG_OK != ret)
      return ret;

    ret = write(buf, dst, tail);
    if (MSG_OK != ret)
      return ret;
  }

  return MSG_OK;
}

#endif /* NVRAM_FS_USE_DELETE_AND_RESIZE */

} /* namespace */
