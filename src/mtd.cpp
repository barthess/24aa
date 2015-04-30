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

/**
 *
 */
void Mtd::wait_for_sync(void) {
  if (0 == cfg.writetime)
    return;
  else
    osalThreadSleep(cfg.writetime);
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
 * @brief   Write data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd::write_type25(const uint8_t *data, size_t len, uint32_t offset) {
  (void)data;
  (void)len;
  (void)offset;
  osalSysHalt("Unrealized");
  return 0;
}

/**
 * @brief   Write data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd::write_type24(const uint8_t *data, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgCheck((sizeof(writebuf) - cfg.addr_len) >= len);

  mtd_led_on();
  this->acquire();

  addr2buf(writebuf, offset, cfg.addr_len);    /* store address bytes */
  memcpy(&(writebuf[cfg.addr_len]), data, len);  /* store data bytes */
  status = bus.exchange(writebuf, len+cfg.addr_len, nullptr, 0);

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
msg_t Mtd::fitted_write(const uint8_t *data, size_t len, uint32_t offset) {

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((offset + len) <= (cfg.pages * cfg.pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((offset / cfg.pagesize) ==
             ((offset + len - 1) / cfg.pagesize)),
             "Data can not be fitted in single page");

  switch (cfg.type) {
  case NvramType::at24:
  case NvramType::fm24:
    if (len == write_type24(data, len, offset))
      return MSG_OK;
    else
      return MSG_RESET;
    break;

  case NvramType::s25:
  case NvramType::fm25:
    if (len == write_type25(data, len, offset))
      return MSG_OK;
    else
      return MSG_RESET;
    break;

  default:
    osalSysHalt("Unrealized");
    return MSG_RESET;
    break;
  }
}

/**
 * @brief   Splits big transaction into smaller ones fitted into driver's buffer.
 */
msg_t Mtd::split_buffer(const uint8_t *data, size_t len, uint32_t offset) {
  size_t written = 0;
  const uint32_t blocksize = sizeof(writebuf) - cfg.addr_len;
  const uint32_t big_writes = len / blocksize;
  const uint32_t small_write_size = len % blocksize;

  /* write big blocks */
  for (size_t i=0; i<big_writes; i++){
    written = fitted_write(data, blocksize, offset);
    if (blocksize == written){
      data += written;
      offset += written;
    }
    else
      return MSG_RESET;
  }

  /* write tail (if any) */
  if (small_write_size > 0){
    written = fitted_write(data, small_write_size, offset);
    if (small_write_size != written)
      return MSG_RESET;
  }

  return MSG_OK; /* */
}

/**
 * @brief   Splits big transaction into smaller ones fitted into memory page.
 */
msg_t Mtd::split_page(const uint8_t *data, size_t len, uint32_t offset) {

  /* bytes to be written at one transaction */
  size_t L = 0;
  /* total bytes successfully written */
  size_t written = 0;
  /* cached value */
  const uint32_t pagesize = cfg.pagesize;
  /* first page to be affected during transaction */
  const uint32_t firstpage = offset / pagesize;
  /* last page to be affected during transaction */
  const uint32_t lastpage = (offset + len - 1) / pagesize;

  /* data fits in single page */
  if (firstpage == lastpage) {
    L = len;
    written += fitted_write(data, L, offset);
    data += L;
    offset += L;
    goto EXIT;
  }
  else{
    /* write first piece of data to the first page boundary */
    L = firstpage * pagesize + pagesize - offset;
    written += fitted_write(data, L, offset);
    data += L;
    offset += L;

    /* now writes blocks at a size of pages (may be no one) */
    L = pagesize;
    while ((len - written) > pagesize){
      written += fitted_write(data, L, offset);
      data += L;
      offset += L;
    }

    /* write tail */
    L = len - written;
    if (0 == L)
      goto EXIT;
    else{
      written += fitted_write(data, L, offset);
    }
  }

EXIT:
  if (written == len)
    return MSG_OK;
  else
    return MSG_RESET;
}

/**
 *
 */
msg_t Mtd::read_type24(uint8_t *data, size_t len, size_t offset) {

  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  this->acquire();
  addr2buf(writebuf, offset, cfg.addr_len);
  status = bus.exchange(writebuf, cfg.addr_len, data, len);
  this->release();

  return status;
}

/**
 *
 */
msg_t Mtd::read_type25(uint8_t *data, size_t len, size_t offset) {
  (void)data;
  (void)len;
  (void)offset;
  osalSysHalt("Unrealized");
  return MSG_RESET;
}

/**
 *
 */
msg_t Mtd::erase_type24(void) {
  msg_t status;

  mtd_led_on();
  this->acquire();

  size_t blocksize = (sizeof(writebuf) - cfg.addr_len);
  size_t total_writes = capacity() / blocksize;

  memset(writebuf, 0xFF, sizeof(writebuf));
  for (size_t i=0; i<total_writes; i++){
    addr2buf(writebuf, i * blocksize, cfg.addr_len);
    status = bus.exchange(writebuf, sizeof(writebuf), nullptr, 0);
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
msg_t Mtd::erase_type25(void) {
  osalSysHalt("Unrealized");
  return MSG_RESET;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd::Mtd(Bus &bus, const MtdConfig &cfg) :
bus(bus),
cfg(cfg)
#if (MTD_USE_MUTUAL_EXCLUSION && !CH_CFG_USE_MUTEXES)
  ,semaphore(1)
#endif
{
  return;
}

/**
 *
 */
msg_t Mtd::write(const uint8_t *data, size_t len, uint32_t offset) {

  switch(cfg.type) {
  case NvramType::at24:
  case NvramType::s25:
    return split_page(data, len, offset);
    break;

  case NvramType::fm24:
  case NvramType::fm25:
    return split_buffer(data, len, offset);
    break;

  default:
    osalSysHalt("Unrealized");
    return MSG_RESET;
    break;
  }
}

/**
 *
 */
msg_t Mtd::read(uint8_t *data, size_t len, uint32_t offset) {

  switch(cfg.type) {
  case NvramType::at24:
  case NvramType::fm24:
    return read_type24(data, len, offset);
    break;

  case NvramType::s25:
  case NvramType::fm25:
    return read_type25(data, len, offset);
    break;

  default:
    osalSysHalt("Unrealized");
    return MSG_RESET;
    break;
  }
}

} /* namespace */
