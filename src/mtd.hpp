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

namespace nvram {

/**
 *
 */
typedef struct {
  /**
   * Driver connected to NVRAM IC.
   */
  I2CDriver     *i2cp;
  /**
   * Address of NVRAM IC on I2C bus.
   */
  i2caddr_t     addr;
}MtdConfig;

/**
 *
 */
class Mtd {
public:
  Mtd(const MtdConfig *cfg);
  msg_t read(uint8_t *data, size_t len, size_t offset);
#if NVRAM_FS_USE_DELETE_AND_RESIZE
  msg_t move(size_t len, size_t blkstart, int shift);
#endif
  virtual msg_t write(const uint8_t *data, size_t len, size_t offset) = 0;
  virtual msg_t shred(uint8_t pattern) = 0;
  virtual size_t capacity(void) = 0;
private:
  msg_t move_right(size_t len, size_t blkstart, size_t shift);
  msg_t move_left(size_t len, size_t blkstart, size_t shift);
protected:
  msg_t shred_impl(uint8_t pattern);
  size_t write_impl(const uint8_t *data, size_t len, size_t offset);
  virtual void wait_for_sync(void) = 0;
  msg_t busTransmit(const uint8_t *txbuf, size_t txbytes);
  msg_t busReceive(uint8_t *rxbuf, size_t rxbytes);
  msg_t stm32_f1x_read_byte(uint8_t *data, size_t offset);
  void split_addr(uint8_t *txbuf, size_t addr);
  systime_t calc_timeout(size_t txbytes, size_t rxbytes);
  void acquire(void);
  void release(void);
  const MtdConfig *cfg;
  i2cflags_t i2cflags;
  #if MTD_USE_MUTUAL_EXCLUSION
    #if CH_CFG_USE_MUTEXES
      chibios_rt::Mutex             mutex;
    #elif CH_CFG_USE_SEMAPHORES
      chibios_rt::CounterSemaphore  semaphore;
    #endif
  #endif /* MTD_USE_MUTUAL_EXCLUSION */
  uint8_t writebuf[MTD_WRITE_BUF_SIZE];
};

} /* namespace */

#endif /* MTD_HPP_ */
