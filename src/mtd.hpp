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

#ifndef MTD_HPP_
#define MTD_HPP_

#include "ch.hpp"
#include "hal.h"

#include "mtd_conf.h"

#if !defined(MTD_USE_MUTUAL_EXCLUSION)
#define MTD_USE_MUTUAL_EXCLUSION                FALSE
#endif

namespace nvram {

class Mtd; /* forward declaration */

typedef void (*mtdcb_t)(Mtd *mtd);

/**
 *
 */
struct MtdConfig {
  /**
   * @brief   Time needed (worst case) by IC for single page writing.
   * @note    Set it to 0 for FRAM.
   */
  systime_t     programtime;
  /**
   * @brief   Time needed (worst case) by IC for full erase.
   * @note    Set it to 0 for FRAM or for memory without hardware erase.
   */
  systime_t     erasetime;
  /**
   * @brief   Size of memory array in pages.
   * @note    Set it to 1 for FRAM.
   */
  uint32_t      pages;
  /**
   * @brief   Size of single page in bytes.
   * @note    Set it to whole array size for FRAM.
   */
  uint32_t      pagesize;
  /**
   * @brief   Address length in bytes.
   */
  size_t        addr_len;
  /**
   * @brief   Debug hooks. Set to nullptr if unused.
   */
  mtdcb_t       hook_start_write;
  mtdcb_t       hook_stop_write;
  mtdcb_t       hook_start_read;
  mtdcb_t       hook_stop_read;
  mtdcb_t       hook_start_erase;
  mtdcb_t       hook_stop_erase;
};

/**
 *
 */
class Mtd {
public:
  Mtd(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size);
  size_t write(const uint8_t *txdata, size_t len, uint32_t offset);
  size_t read(uint8_t *rxbuf, size_t len, uint32_t offset);
  msg_t erase(void);
  uint32_t capacity(void) {return cfg.pages * cfg.pagesize;}
  uint32_t pagesize(void) {return cfg.pagesize;}
  uint32_t pagecount(void) {return cfg.pages;}
protected:
  virtual size_t bus_write(const uint8_t *txdata, size_t len, uint32_t offset) = 0;
  virtual size_t bus_read(uint8_t *rxbuf, size_t len, uint32_t offset) = 0;
  virtual msg_t bus_erase(void) = 0;

  size_t split_by_buffer(const uint8_t *txdata, size_t len, uint32_t offset);
  size_t split_by_page  (const uint8_t *txdata, size_t len, uint32_t offset);
  size_t fitted_write(const uint8_t *txdata, size_t len, uint32_t offset);

  void addr2buf(uint8_t *buf, uint32_t addr, size_t addr_len);
  void acquire(void);
  void release(void);

  #if MTD_USE_MUTUAL_EXCLUSION
    #if CH_CFG_USE_MUTEXES
      chibios_rt::Mutex             mutex;
    #elif CH_CFG_USE_SEMAPHORES
      chibios_rt::CounterSemaphore  semaphore;
    #endif
  #endif /* MTD_USE_MUTUAL_EXCLUSION */

  const MtdConfig &cfg;
  uint8_t *writebuf;
  size_t writebuf_size;
};

} /* namespace */

#endif /* MTD_HPP_ */
