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

/**
 *
 */
typedef struct {
  /**
   * Driver connecte to IC.
   */
  I2CDriver     *i2cp;
  /**
   * Address of IC on I2C bus.
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
  virtual msg_t write(const uint8_t *data, size_t len, size_t offset) = 0;
  virtual msg_t move(size_t blklen, size_t blkoffset, int32_t shift) = 0;
  virtual msg_t shred(uint8_t pattern) = 0;
  virtual size_t capacity(void) = 0;
  virtual size_t page_size(void) = 0;
protected:
  msg_t busTransmit(const uint8_t *txbuf, size_t txbytes);
  msg_t busReceive(uint8_t *rxbuf, size_t rxbytes);
  msg_t stm32_f1x_read_byte(uint8_t *data, size_t offset);
  void split_addr(uint8_t *txbuf, size_t addr);
  systime_t calc_timeout(size_t txbytes, size_t rxbytes);
  void acquire(void);
  void release(void);
  uint8_t writebuf[MTD_WRITE_BUF_SIZE];
  const MtdConfig *cfg;
  i2cflags_t i2cflags;

  #if MTD_USE_MUTUAL_EXCLUSION
    #if CH_CFG_USE_MUTEXES
      chibios_rt::Mutex             mutex;
    #elif CH_CFG_USE_SEMAPHORES
      chibios_rt::CounterSemaphore  semaphore;
    #endif
  #endif /* MTD_USE_MUTUAL_EXCLUSION */
};

#endif /* MTD_HPP_ */
