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
 * @brief   Split multibyte address into uint8_t array.
 *
 * @param[in] txbuf pointer to driver transmit buffer
 * @param[in] addr  internal EEPROM device address
 */
void Mtd::addr2buf(uint8_t *buf, uint32_t addr, size_t addr_len) {
  osalDbgCheck(addr_len <= sizeof(addr));

  switch (addr_len) {
  case 1:
    buf[0] = addr & 0xFF;
    break;
  case 2:
    buf[0] = (addr >> 8) & 0xFF;
    buf[1] = addr & 0xFF;
    break;
  case 3:
    buf[0] = (addr >> 16) & 0xFF;
    buf[1] = (addr >> 8) & 0xFF;
    buf[2] = addr & 0xFF;
    break;
  case 4:
    buf[0] = (addr >> 24) & 0xFF;
    buf[1] = (addr >> 16) & 0xFF;
    buf[2] = (addr >> 8) & 0xFF;
    buf[3] = addr & 0xFF;
    break;
  default:
    osalSysHalt("Incorrect address length");
    break;
  }
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
size_t Mtd::fitted_write(const uint8_t *txdata, size_t len, uint32_t offset) {

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((offset + len) <= (cfg.pages * cfg.pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((offset / cfg.pagesize) == ((offset + len - 1) / cfg.pagesize)),
             "Data can not be fitted in single page");

 return bus_write(txdata, len, offset);
}

/**
 * @brief   Splits big transaction into smaller ones fitted into MTD's buffer.
 */
size_t Mtd::split_by_buffer(const uint8_t *txdata, size_t len, uint32_t offset) {
  size_t written = 0;
  size_t tmp;
  const uint32_t blocksize = this->writebuf_size - cfg.addr_len;
  const uint32_t big_writes = len / blocksize;
  const uint32_t small_write_size = len % blocksize;

  /* write big blocks */
  for (size_t i=0; i<big_writes; i++) {
    tmp = fitted_write(txdata, blocksize, offset);
    if (blocksize == tmp) {
      txdata += tmp;
      offset += tmp;
      written += tmp;
    }
    else
      goto EXIT;
  }

  /* write tail (if any) */
  if (small_write_size > 0) {
    tmp = fitted_write(txdata, small_write_size, offset);
    if (small_write_size == tmp)
      written += tmp;
  }

EXIT:
  return written;
}

/**
 * @brief   Splits big transaction into smaller ones fitted into memory page.
 */
size_t Mtd::split_by_page(const uint8_t *txdata, size_t len, uint32_t offset) {

  /* bytes to be written at one transaction */
  size_t L = 0;
  /* total bytes successfully written */
  size_t written = 0;
  size_t ret = 0;
  /* cached value */
  const uint32_t pagesize = cfg.pagesize;
  /* first page to be affected during transaction */
  const uint32_t firstpage = offset / pagesize;
  /* last page to be affected during transaction */
  const uint32_t lastpage = (offset + len - 1) / pagesize;

  /* data fits in single page */
  if (firstpage == lastpage) {
    L = len;
    written = fitted_write(txdata, L, offset);
    txdata += L;
    offset += L;
    goto EXIT;
  }
  else{
    /* write first piece of data to the first page boundary */
    L = firstpage * pagesize + pagesize - offset;
    ret = fitted_write(txdata, L, offset);
    if (ret != L)
      goto EXIT;
    else
      written += ret;
    txdata += L;
    offset += L;

    /* now writes blocks at a size of pages (may be no one) */
    L = pagesize;
    while ((len - written) > pagesize){
      ret = fitted_write(txdata, L, offset);
      if (ret != L)
        goto EXIT;
      else
        written += ret;
      txdata += L;
      offset += L;
    }

    /* write tail */
    L = len - written;
    if (0 == L)
      goto EXIT;
    else{
      written += fitted_write(txdata, L, offset);
    }
  }

EXIT:
  return written;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/* The rule of thumb for best performance write buffer:
 * 1) for EEPROM set it to size of your IC's page + ADDRESS_BYTES +
 *    command bytes (if any).
 * 2) for FRAM there is no such strict rule - good chose is 16..64 */
Mtd::Mtd(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size) :
cfg(cfg),
writebuf(writebuf),
writebuf_size(writebuf_size)
#if (MTD_USE_MUTUAL_EXCLUSION && !CH_CFG_USE_MUTEXES)
  ,semaphore(true)
#endif
{
  return;
}

/**
 *
 */
size_t Mtd::write(const uint8_t *data, size_t len, uint32_t offset) {
  size_t ret;

  if (nullptr != cfg.hook_start_write)
    cfg.hook_start_write(this);

  if (1 == cfg.pages) { /* FRAM */
    ret = split_by_buffer(data, len, offset);
  }
  else {  /* page organized EEPROM */
    ret = split_by_page(data, len, offset);
  }

  if (nullptr != cfg.hook_stop_write)
    cfg.hook_stop_write(this);

  return ret;
}

/**
 *
 */
size_t Mtd::read(uint8_t *rxbuf, size_t len, uint32_t offset) {
  size_t ret;

  if (nullptr != cfg.hook_start_read)
    cfg.hook_start_read(this);

  ret = bus_read(rxbuf, len, offset);

  if (nullptr != cfg.hook_stop_read)
    cfg.hook_stop_read(this);

  return ret;
}

/**
 *
 */
msg_t Mtd::erase(void) {
  size_t ret;

  if (nullptr != cfg.hook_start_erase)
    cfg.hook_start_erase(this);

  ret = bus_erase();

  if (nullptr != cfg.hook_stop_erase)
    cfg.hook_stop_erase(this);

  return ret;
}

} /* namespace */
