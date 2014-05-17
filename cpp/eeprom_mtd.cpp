/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#include <string.h>
#include "eeprom_mtd.hpp"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#if defined(SAM7_PLATFORM)
#define EEPROM_I2C_CLOCK (MCK / (((i2cp->config->cwgr & 0xFF) + ((i2cp->config->cwgr >> 8) & 0xFF)) * (1 << ((i2cp->config->cwgr >> 16) & 7)) + 6))
#else
#define EEPROM_I2C_CLOCK (i2cp->config->clock_speed)
#endif

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
static void eeprom_split_addr(uint8_t *txbuf, size_t addr){
  txbuf[0] = (addr >> 8) & 0xFF;
  txbuf[1] = addr & 0xFF;
}

/**
 * @brief     Calculates requred timeout.
 */
static systime_t calc_timeout(I2CDriver *i2cp, size_t txbytes, size_t rxbytes){
  const uint32_t bitsinbyte = 10;
  uint32_t tmo;
  tmo = ((txbytes + rxbytes + 1) * bitsinbyte * 1000);
  tmo /= EEPROM_I2C_CLOCK;
  tmo += 10; /* some additional milliseconds to be safer */
  return MS2ST(tmo);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 * @brief   Constructor
 */
EepromMtd::EepromMtd(const EepromMtdConfig *cfg)
#if (EEPROM_MTD_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES)
  : semaphore(1);
#endif
{
  this->cfg = cfg;
}

/**
 * @brief   EEPROM read routine.
 *
 * @param[in] data        pointer to buffer where red data to be stored
 * @param[in] absoffset   address of first byte relatively to eeprom start
 * @param[in] len         number of bytes to be red
 */
msg_t EepromMtd::read(uint8_t *data, size_t absoffset, size_t len){
  msg_t status = RDY_RESET;
  systime_t tmo = calc_timeout(cfg->i2cp, 2, len);
  volatile i2cflags_t flags;

  chDbgCheck((absoffset + len) <= (cfg->pages * cfg->pagesize),
             "Transaction out of device bounds");

  this->acquire();

  eeprom_split_addr(writebuf, absoffset);

  #if I2C_USE_MUTUAL_EXCLUSION
    i2cAcquireBus(cfg->i2cp);
  #endif

  status = i2cMasterTransmitTimeout(cfg->i2cp, cfg->addr, writebuf,
                                    2, data, len, tmo);
  if (RDY_OK != status)
    flags = i2cGetErrors(cfg->i2cp);

  #if I2C_USE_MUTUAL_EXCLUSION
    i2cReleaseBus(cfg->i2cp);
  #endif

  this->release();

  (void)flags;
  return status;
}

/**
 * @brief   EEPROM write routine.
 * @details Function writes data to EEPROM.
 * @pre     Data must be fit to single EEPROM page.
 *
 * @param[in] data        pointer to buffer containing data to be written
 * @param[in] absoffset   address of first byte relatively to eeprom start
 * @param[in] len         number of bytes to be written
 */
msg_t EepromMtd::write(const uint8_t *data, size_t absoffset, size_t len){

  eeprom_led_on();

  msg_t status = RDY_RESET;
  systime_t tmo = calc_timeout(cfg->i2cp, (len + 2), 0);

  chDbgCheck((absoffset + len) <= (cfg->pages * cfg->pagesize),
             "Transaction out of device bounds");

  chDbgCheck(((absoffset / cfg->pagesize) ==
             ((absoffset + len - 1) / cfg->pagesize)),
             "Data can not be fitted in single page");

  this->acquire();

  /* write address bytes */
  eeprom_split_addr(writebuf, absoffset);
  /* write data bytes */
  memcpy(&(writebuf[2]), data, len);

  #if I2C_USE_MUTUAL_EXCLUSION
    i2cAcquireBus(cfg->i2cp);
  #endif

  status = i2cMasterTransmitTimeout(cfg->i2cp, cfg->addr, writebuf,
                                    (len + 2), NULL, 0, tmo);

  #if I2C_USE_MUTUAL_EXCLUSION
    i2cReleaseBus(cfg->i2cp);
  #endif

  /* wait until EEPROM process data */
  chThdSleep(cfg->writetime);
  eeprom_led_off();

  this->release();

  return status;
}

/**
 *
 */
size_t EepromMtd::getPageSize(void){
  return this->cfg->pagesize;
}

/**
 *
 */
msg_t EepromMtd::massErase(void){
  msg_t status = RDY_RESET;
  size_t absoffset = 0;
  uint8_t erasebuf[EEPROM_PAGE_SIZE];

  memset(erasebuf, 0xFF, sizeof(erasebuf));
  while (absoffset < EEPROM_PAGES * EEPROM_PAGE_SIZE){
    status = write(erasebuf, absoffset, sizeof(erasebuf));
    if (RDY_OK != status)
      return status;
    else
      absoffset += sizeof(erasebuf);
  }

  return status;
}


/**
 *
 */
void EepromMtd::acquire(void) {
#if EEPROM_MTD_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
  mutex.lock();
#elif CH_USE_SEMAPHORES
  semaphore.wait();
#endif
#endif /* EEPROM_MTD_USE_MUTUAL_EXCLUSION */
}

/**
 *
 */
void EepromMtd::release(void) {
#if EEPROM_MTD_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
  chMtxUnlock();
#elif CH_USE_SEMAPHORES
  semaphore.signal();
#endif
#endif /* EEPROM_MTD_USE_MUTUAL_EXCLUSION */
}
