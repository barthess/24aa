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

#ifndef MTD_BASE_HPP_
#define MTD_BASE_HPP_

#include "ch.hpp"
#include "hal.h"

#include "mtd_conf.h"

#if !defined(MTD_USE_MUTUAL_EXCLUSION)
#define MTD_USE_MUTUAL_EXCLUSION                FALSE
#endif

namespace nvram {

class MtdBase; /* forward declaration */

typedef void (*mtdcb_t)(MtdBase *mtd);

typedef void (*spiselect_t)(void);

/**
 *
 */
struct MtdConfig {
  /**
   * @brief   Time needed (worst case) by IC for single page writing.
   * @note    Set it to 0 for FRAM.
   * @note    It is system ticks NOT milliseconds.
   */
  systime_t     programtime;
  /**
   * @brief   Time needed (worst case) by IC for full erase.
   * @note    Set it to 0 for FRAM or for memory without auto-erase.
   * @note    It is system ticks NOT milliseconds.
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
   * @brief   Bus clock in Hz for operation timeout calculation.
   */
  uint32_t      bus_clk;
  /**
   * @brief   SPI Bus (un)select function pointer. Set to nullptr for I2C.
   */
  spiselect_t   spi_select;
  spiselect_t   spi_unselect;
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
class MtdBase {
public:
  MtdBase(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size);
  size_t write(const uint8_t *txdata, size_t len, uint32_t offset);
  size_t read(uint8_t *rxbuf, size_t len, uint32_t offset);
  uint32_t capacity(void) {return cfg.pages * cfg.pagesize;}
  uint32_t pagesize(void) {return cfg.pagesize;}
  uint32_t pagecount(void) {return cfg.pages;}
  bool is_fram(void);
protected:
  virtual size_t bus_write(const uint8_t *txdata, size_t len, uint32_t offset) = 0;
  virtual size_t bus_read(uint8_t *rxbuf, size_t len, uint32_t offset) = 0;

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

#endif /* MTD_BASE_HPP_ */
