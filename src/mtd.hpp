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

#ifndef MTD_HPP_
#define MTD_HPP_

#include "ch.hpp"
#include "hal.h"

#include "mtd_conf.h"
#include "bus.hpp"

#if !defined(MTD_USE_MUTUAL_EXCLUSION)
#define MTD_USE_MUTUAL_EXCLUSION                FALSE
#endif

/* The rule of thumb for best performance:
 * 1) for EEPROM set it to size of your IC's page + NVRAM_ADDRESS_BYTES
 * 2) for FRAM there is no such strict rule - chose it from 16..64 */
#if !defined(MTD_WRITE_BUF_SIZE)
#error "Buffer size must be defined in mtd_conf.h"
#endif

namespace nvram {

/**
 *
 */
enum class NvramType {
  at24,
  s25,
  fm24,
  fm25
};

typedef void (*mtdcb_t)(void);

/**
 *
 */
struct MtdConfig {
  /**
   * @brief   Time needed by IC for single page writing.
   */
  systime_t     writetime;
  /**
   * @brief   Size of memory array in pages.
   * @note    For FRAM set it to 1.
   */
  uint32_t      pages;
  /**
   * @brief   Size of single page in bytes.
   * @note    For FRAM set it to whole array size.
   */
  uint32_t      pagesize;
  /**
   * @brief   Address length in bytes.
   */
  size_t        addr_len;
  /**
   * @brief   Memory type.
   */
  NvramType     type;
  /**
   * @brief   Debug hooks.
   */
  mtdcb_t       start_write;
  mtdcb_t       stop_write;
  mtdcb_t       start_read;
  mtdcb_t       stop_read;
  mtdcb_t       start_erase;
  mtdcb_t       stop_erase;
};

/**
 *
 */
class Mtd {
public:
  Mtd(Bus &bus, const MtdConfig &cfg);
  msg_t write(const uint8_t *data, size_t len, uint32_t offset);
  msg_t read(uint8_t *data, size_t len, uint32_t offset);
  msg_t erase(void);
  uint32_t capacity(void) {return cfg.pages * cfg.pagesize;}
  uint32_t pagesize(void) {return cfg.pagesize;}
protected:
  msg_t split_buffer(const uint8_t *data, size_t len, uint32_t offset);
  msg_t split_page(const uint8_t *data, size_t len, uint32_t offset);
  msg_t fitted_write(const uint8_t *data, size_t len, uint32_t offset);
  size_t write_type24(const uint8_t *data, size_t len, uint32_t offset);
  size_t write_type25(const uint8_t *data, size_t len, uint32_t offset);
  msg_t erase_type24(void);
  msg_t erase_type25(void);
  msg_t read_type24(uint8_t *data, size_t len, size_t uint32_t);
  msg_t read_type25(uint8_t *data, size_t len, size_t uint32_t);
  void wait_for_sync(void);
  void addr2buf(uint8_t *txbuf, uint32_t addr, size_t addr_len);
  void acquire(void);
  void release(void);
  #if MTD_USE_MUTUAL_EXCLUSION
    #if CH_CFG_USE_MUTEXES
      chibios_rt::Mutex             mutex;
    #elif CH_CFG_USE_SEMAPHORES
      chibios_rt::CounterSemaphore  semaphore;
    #endif
  #endif /* MTD_USE_MUTUAL_EXCLUSION */
  uint8_t writebuf[MTD_WRITE_BUF_SIZE];
  Bus &bus;
  const MtdConfig &cfg;
};

} /* namespace */

#endif /* MTD_HPP_ */
