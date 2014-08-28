/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_MTD_HPP_
#define EEPROM_MTD_HPP_

#include "ch.hpp"
#include "hal.h"
#include "eeprom_mtd_conf.h"

typedef uint16_t eeprom_size_t;

#if !defined(EEPROM_MTD_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define EEPROM_MTD_USE_MUTUAL_EXCLUSION    FALSE
#endif

#if EEPROM_MTD_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES && !CH_USE_SEMAPHORES
#error "EEPROM_MTD_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES and/or CH_USE_SEMAPHORES"
#endif

typedef struct EepromMtdConfig {
  /**
   * Driver connecte to IC.
   */
  I2CDriver     *i2cp;
  /**
   * Time needed by IC for single page writing.
   * Check datasheet!!!
   */
  systime_t     writetime;
  /**
   * Size of memory array in pages.
   * Check datasheet!!!
   */
  size_t        pages;
  /**
   * Size of single page in bytes.
   * Check datasheet!!!
   */
  size_t        pagesize;
  /**
   * Address of IC on I2C bus.
   */
  i2caddr_t     addr;
}EepromMtdConfig;


class EepromMtd{
public:
  EepromMtd(const EepromMtdConfig *cfg);
  msg_t read(uint8_t *data, size_t absoffset, size_t len);
  msg_t write(const uint8_t *data, size_t absoffset, size_t len);
  size_t getPageSize(void);
  msg_t massErase(void);

private:
  void acquire(void);
  void release(void);
  const EepromMtdConfig *cfg;
  uint8_t writebuf[EEPROM_PAGE_SIZE + 2];

#if EEPROM_MTD_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
  chibios_rt::Mutex             mutex;
#elif CH_USE_SEMAPHORES
  chibios_rt::CounterSemaphore  semaphore;
#endif
#endif /* EEPROM_MTD_USE_MUTUAL_EXCLUSION */
};

#endif /* EEPROM_MTD_HPP_ */
