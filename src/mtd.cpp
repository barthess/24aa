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
#define     S25_CMD_READ    0x03
#define     S25_CMD_4READ   0x13
#define     S25_CMD_PP      0x02  /* page program */
#define     S25_CMD_4PP     0x12
#define     S25_CMD_BE      0x60  /* bulk erase */

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
 * @brief   Accepts data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd::write_type25(const uint8_t *txdata, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgCheck(sizeof(writebuf) >= cfg.pagesize + cfg.addr_len + 1);
  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  mtd_led_on();
  this->acquire();

  /* fill preamble */
  if (4 == cfg.addr_len)
    writebuf[0] = S25_CMD_4PP;
  else
    writebuf[0] = S25_CMD_PP;
  addr2buf(&writebuf[1], offset, cfg.addr_len);

  status = bus.write(txdata, len, writebuf, 1+cfg.addr_len);

  wait_for_sync();
  this->release();
  mtd_led_off();

  if (MSG_OK == status)
    return len;
  else
    return 0;
}

/**
 * @brief   Accepts data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd::write_type24(const uint8_t *txdata, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgCheck((sizeof(writebuf) - cfg.addr_len) >= len);
  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  mtd_led_on();
  this->acquire();

  /* write preamble. Only address bytes for this memory type */
  addr2buf(writebuf, offset, cfg.addr_len);
  status = bus.write(txdata, len, writebuf, cfg.addr_len);

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
size_t Mtd::fitted_write(const uint8_t *txdata, size_t len, uint32_t offset) {

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((offset + len) <= (cfg.pages * cfg.pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((offset / cfg.pagesize) == ((offset + len - 1) / cfg.pagesize)),
             "Data can not be fitted in single page");

  switch (cfg.type) {
  case NvramType::at24:
  case NvramType::fm24:
    return write_type24(txdata, len, offset);

  case NvramType::s25:
  case NvramType::fm25:
    return write_type25(txdata, len, offset);

  default:
    osalSysHalt("Unrealized");
    return 0;
  }
}

/**
 * @brief   Splits big transaction into smaller ones fitted into driver's buffer.
 */
size_t Mtd::split_buffer(const uint8_t *txdata, size_t len, uint32_t offset) {
  size_t written = 0;
  size_t tmp;
  const uint32_t blocksize = sizeof(writebuf) - cfg.addr_len;
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
size_t Mtd::split_page(const uint8_t *txdata, size_t len, uint32_t offset) {

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
    written += fitted_write(txdata, L, offset);
    txdata += L;
    offset += L;
    goto EXIT;
  }
  else{
    /* write first piece of data to the first page boundary */
    L = firstpage * pagesize + pagesize - offset;
    written += fitted_write(txdata, L, offset);
    txdata += L;
    offset += L;

    /* now writes blocks at a size of pages (may be no one) */
    L = pagesize;
    while ((len - written) > pagesize){
      written += fitted_write(txdata, L, offset);
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

/**
 *
 */
size_t Mtd::read_type24(uint8_t *rxbuf, size_t len, size_t offset) {
  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
  osalDbgCheck(sizeof(writebuf) >= cfg.addr_len);

  this->acquire();
  addr2buf(writebuf, offset, cfg.addr_len);
  status = bus.read(rxbuf, len, writebuf, cfg.addr_len);
  this->release();

  if (MSG_OK == status)
    return len;
  else
    return 0;
}

/**
 *
 */
size_t Mtd::read_type25(uint8_t *rxbuf, size_t len, size_t offset) {
  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
  osalDbgCheck(sizeof(writebuf) >= cfg.addr_len + 1);

  this->acquire();

  /* fill preamble */
  if (4 == cfg.addr_len)
    writebuf[0] = S25_CMD_4READ;
  else
    writebuf[0] = S25_CMD_READ;
  addr2buf(&writebuf[1], offset, cfg.addr_len);

  status = bus.read(rxbuf, len, writebuf, 1+cfg.addr_len);

  this->release();

  if (MSG_OK == status)
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

  size_t blocksize = (sizeof(writebuf) - cfg.addr_len);
  size_t total_writes = capacity() / blocksize;

  memset(writebuf, 0xFF, sizeof(writebuf));
  status = MSG_RESET;
  for (size_t i=0; i<total_writes; i++) {
    addr2buf(writebuf, i * blocksize, cfg.addr_len);
    status = bus.write(nullptr, 0, writebuf, sizeof(writebuf));
    wait_for_sync();
    if (MSG_OK != status)
      break;
  }

  this->release();
  mtd_led_off();
  return status;
}

/**
 *
 */
msg_t Mtd::erase_type25(void) {

  mtd_led_on();
  this->acquire();

  writebuf[0] = S25_CMD_BE;
  bus.write(nullptr, 0, writebuf, 1);

  osalSysHalt("Wait erase completeness unrealized yet");

  this->release();
  mtd_led_off();
  return MSG_OK;
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
size_t Mtd::write(const uint8_t *data, size_t len, uint32_t offset) {

  switch(cfg.type) {
  case NvramType::at24:
  case NvramType::s25:
    return split_page(data, len, offset);

  case NvramType::fm24:
  case NvramType::fm25:
    return split_buffer(data, len, offset);

  default:
    osalSysHalt("Unrealized");
    return 0;
  }
}

/**
 *
 */
size_t Mtd::read(uint8_t *rxbuf, size_t len, uint32_t offset) {

  switch(cfg.type) {
  case NvramType::at24:
  case NvramType::fm24:
    return read_type24(rxbuf, len, offset);

  case NvramType::s25:
  case NvramType::fm25:
    return read_type25(rxbuf, len, offset);

  default:
    osalSysHalt("Unrealized");
    return 0;
  }
}

} /* namespace */
