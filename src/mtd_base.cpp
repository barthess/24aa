/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

    This file is part of 24AA lib.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <cstring>
#include <cstdlib>

#include "ch.hpp"

#include "mtd_base.hpp"

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
void MtdBase::addr2buf(uint8_t *buf, uint32_t addr, size_t addr_len) {
  osalDbgCheck(addr_len <= sizeof(addr));

  switch (addr_len) {
  case 1:
    buf[0] = addr & 0xFF;
    break;
  case 2:
    buf[0] = (addr >> 8) & 0xFF;
    buf[1] =  addr       & 0xFF;
    break;
  case 3:
    buf[0] = (addr >> 16) & 0xFF;
    buf[1] = (addr >> 8)  & 0xFF;
    buf[2] =  addr        & 0xFF;
    break;
  case 4:
    buf[0] = (addr >> 24) & 0xFF;
    buf[1] = (addr >> 16) & 0xFF;
    buf[2] = (addr >> 8)  & 0xFF;
    buf[3] =  addr        & 0xFF;
    break;
  default:
    osalSysHalt("Incorrect address length");
    break;
  }
}

/**
 *
 */
void MtdBase::acquire(void) {
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
void MtdBase::release(void) {
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
size_t MtdBase::fitted_write(const uint8_t *txdata, size_t len, uint32_t offset) {

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
size_t MtdBase::split_by_buffer(const uint8_t *txdata, size_t len, uint32_t offset) {
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
 * @return  Number of written bytes.
 */
size_t MtdBase::split_by_page(const uint8_t *txdata, size_t len, uint32_t offset) {

  /* bytes to be written at one transaction */
  size_t L = 0;
  /* total bytes successfully written */
  size_t written = 0;
  size_t status = 0;
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
    status = fitted_write(txdata, L, offset);
    if (status != L)
      goto EXIT;
    else
      written += status;
    txdata += L;
    offset += L;

    /* now writes blocks at a size of pages (may be no one) */
    L = pagesize;
    while ((len - written) > pagesize){
      status = fitted_write(txdata, L, offset);
      if (status != L)
        goto EXIT;
      else
        written += status;
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

/*
 * The rules of thumb for best performance write buffer.
 * It must be big enough to store:
 * 1) Size of your IC's page + ADDRESS_BYTES + command bytes (if any) for EEPROM.
 * 2) for FRAM there is no such strict rule - good chose is 16..64
 */
MtdBase::MtdBase(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size) :
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
 * @return number of written bytes
 */
size_t MtdBase::write(const uint8_t *data, size_t len, uint32_t offset) {
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
 * @return number of read bytes
 */
size_t MtdBase::read(uint8_t *rxbuf, size_t len, uint32_t offset) {
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
bool MtdBase::is_fram(void) {
  return 0 == cfg.programtime;
}

} /* namespace */
