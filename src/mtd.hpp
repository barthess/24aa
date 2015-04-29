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

#define NVRAM_ADDRESS_BYTES                     2

#if !defined(MTD_USE_MUTUAL_EXCLUSION)
#define MTD_USE_MUTUAL_EXCLUSION                FALSE
#endif

/* The rule of thumb for best performance:
 * 1) for EEPROM set it to size of your IC's page + NVRAM_ADDRESS_BYTES
 * 2) for FRAM there is no such strict rule - chose it from 16..64 */
#if !defined(MTD_WRITE_BUF_SIZE)
#error "Buffer size must be defined in mtd_conf.h"
#endif

#if !defined(NVRAM_FS_USE_DELETE_AND_RESIZE)
#define NVRAM_FS_USE_DELETE_AND_RESIZE          FALSE
#endif

#if NVRAM_FS_USE_DELETE_AND_RESIZE
#warning "Experimental untested feature enabled"
#endif

namespace nvram {

/**
 *
 */
class Mtd {
public:
  Mtd(Bus &bus);
  virtual msg_t read(uint8_t *data, size_t len, size_t offset) = 0;
  virtual msg_t write(const uint8_t *data, size_t len, size_t offset) = 0;
  virtual msg_t erase(void) = 0;
  virtual uint32_t capacity(void) = 0;
  virtual uint32_t pagesize(void) = 0;
protected:
  msg_t erase_type24(void);
  size_t write_type24(const uint8_t *data, size_t len, size_t offset);
  msg_t read_type24(uint8_t *data, size_t len, size_t offset);
  virtual void wait_for_sync(void) = 0;
  void split_addr(uint8_t *txbuf, uint32_t addr, size_t addr_len);
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
};

} /* namespace */

#endif /* MTD_HPP_ */
