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
void Mtd::addr2buf(uint8_t *txbuf, uint32_t addr, size_t addr_len) {
  osalDbgCheck(addr_len <= sizeof(addr));

  switch (addr_len) {
  case 1:
    txbuf[0] = addr & 0xFF;
    break;
  case 2:
    txbuf[0] = (addr >> 8) & 0xFF;
    txbuf[1] = addr & 0xFF;
    break;
  case 3:
    txbuf[0] = (addr >> 16) & 0xFF;
    txbuf[1] = (addr >> 8) & 0xFF;
    txbuf[2] = addr & 0xFF;
    break;
  case 4:
    txbuf[0] = (addr >> 24) & 0xFF;
    txbuf[1] = (addr >> 16) & 0xFF;
    txbuf[2] = (addr >> 8) & 0xFF;
    txbuf[3] = addr & 0xFF;
    break;
  default:
    osalSysHalt("Incorrect address length");
    break;
  }
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd::Mtd(Bus &bus) :
bus(bus)
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
msg_t Mtd::read_type24(uint8_t *data, size_t len, size_t offset) {

  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  this->acquire();
  addr2buf(writebuf, offset, NVRAM_ADDRESS_BYTES);
  status = bus.exchange(writebuf, NVRAM_ADDRESS_BYTES, data, len);
  this->release();

  return status;
}

/**
 * @brief   Write data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd::write_type24(const uint8_t *data, size_t len, size_t offset) {
  msg_t status;

  osalDbgCheck((sizeof(writebuf) - NVRAM_ADDRESS_BYTES) >= len);

  mtd_led_on();
  this->acquire();

  addr2buf(writebuf, offset, NVRAM_ADDRESS_BYTES);    /* store address bytes */
  memcpy(&(writebuf[NVRAM_ADDRESS_BYTES]), data, len);  /* store data bytes */
  status = bus.exchange(writebuf, len+NVRAM_ADDRESS_BYTES, nullptr, 0);

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
msg_t Mtd::erase_type24(void) {
  msg_t status;

  mtd_led_on();
  this->acquire();

  size_t blocksize = (sizeof(writebuf) - NVRAM_ADDRESS_BYTES);
  size_t total_writes = capacity() / blocksize;

  memset(writebuf, 0xFF, sizeof(writebuf));
  for (size_t i=0; i<total_writes; i++){
    addr2buf(writebuf, i * blocksize, NVRAM_ADDRESS_BYTES);
    status = bus.exchange(writebuf, sizeof(writebuf), nullptr, 0);
    osalDbgCheck(MSG_OK == status);
    wait_for_sync();
  }

  this->release();
  mtd_led_off();
  return MSG_OK;
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
