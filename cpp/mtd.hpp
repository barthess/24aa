/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef MTD_HPP_
#define MTD_HPP_

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
  virtual msg_t read(uint8_t *data, size_t absoffset, size_t len) = 0;
  virtual msg_t write(const uint8_t *data, size_t absoffset, size_t len) = 0;
  virtual msg_t shred(uint8_t pattern) = 0;
private:
  void acquire(void);
  void release(void);
  void split_addr(uint8_t *txbuf, size_t addr);
  systime_t calc_timeout(size_t txbytes, size_t rxbytes);

  const MtdConfig *cfg;
  uint8_t writebuf[MTD_WRITE_BUF_SIZE];
  i2cflags_t i2cflags;

  #if EEPROM_MTD_USE_MUTUAL_EXCLUSION
    #if CH_CFG_USE_MUTEXES
      chibios_rt::Mutex             mutex;
    #elif CH_CFG_USE_SEMAPHORES
      chibios_rt::CounterSemaphore  semaphore;
    #endif
  #endif /* EEPROM_MTD_USE_MUTUAL_EXCLUSION */
};

#endif /* MTD_HPP_ */
