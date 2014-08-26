/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

/*****************************************************************************
 * DATASHEET NOTES
 *****************************************************************************
Write cycle time (byte or page) — 5 ms

Note:
Page write operations are limited to writing
bytes within a single physical page,
regardless of the number of bytes
actually being written. Physical page
boundaries start at addresses that are
integer multiples of the page buffer size (or
‘page size’) and end at addresses that are
integer multiples of [page size – 1]. If a
Page Write command attempts to write
across a physical page boundary, the
result is that the data wraps around to the
beginning of the current page (overwriting
data previously stored there), instead of
being written to the next page as might be
expected.
*********************************************************************/

#include "ch.hpp"

#include "eeprom_mtd.hpp"

//using namespace chibios_rt;

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

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
 * @brief   Write data that can be fitted in single page boundary
 */
void EepromMtd::fitted_write(const uint8_t *data, size_t absoffset,
                                        size_t len, uint32_t *written){
  msg_t status = MSG_RESET;
  systime_t tmo = calc_timeout(len + 2, 0);

  eeprom_led_on();

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((absoffset + len) <= (eeprom_cfg->pages * eeprom_cfg->pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((absoffset / eeprom_cfg->pagesize) ==
             ((absoffset + len - 1) / eeprom_cfg->pagesize)),
             "Data can not be fitted in single page");

  this->acquire();

  osalSysHalt("splitting of data in smaller buffer unrealized");

  /* write address bytes */
  split_addr(writebuf, absoffset);
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
  chThdSleep(eeprom_cfg->writetime);
  this->release();
  eeprom_led_off();

  if (status == MSG_OK){
    osalSysHalt("splitting of data in smaller buffer unrealized");
    *written += len;
  }
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
EepromMtd::EepromMtd(const MtdConfig *mtd_cfg, const EepromConfig *eeprom_cfg) :
Mtd(mtd_cfg),
eeprom_cfg(eeprom_cfg)
{
  return;
}

/**
 *
 */
msg_t EepromMtd::read(uint8_t *data, size_t absoffset, size_t len){

  msg_t status = MSG_RESET;
  systime_t tmo = calc_timeout(2, len);

  osalDbgAssert((absoffset + len) <= (eeprom_cfg->pages * eeprom_cfg->pagesize),
             "Transaction out of device bounds");

  this->acquire();

  split_addr(writebuf, absoffset);

  #if I2C_USE_MUTUAL_EXCLUSION
    i2cAcquireBus(cfg->i2cp);
  #endif

  status = i2cMasterTransmitTimeout(cfg->i2cp, cfg->addr, writebuf,
                                    2, data, len, tmo);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(cfg->i2cp);

  #if I2C_USE_MUTUAL_EXCLUSION
    i2cReleaseBus(cfg->i2cp);
  #endif

  this->release();
  return status;
}

/**
 *
 */
msg_t EepromMtd::write(const uint8_t *bp, size_t absoffset, size_t n){

  /* bytes to be written at one transaction */
  size_t len;
  /* total bytes successfully written */
  uint32_t written;
  /* cached value */
  uint16_t pagesize = this->getPageSize();
  /* first page to be affected during transaction */
  uint32_t firstpage = absoffset / pagesize;
  /* last page to be affected during transaction */
  uint32_t lastpage = (absoffset + n - 1) / pagesize;

  len = 0;
  written = 0;

  /* data can be fitted in single page */
  if (firstpage == lastpage){
    len = n;
    fitted_write(bp, absoffset, len, &written);
    bp += len;
    absoffset += len;
    return written;
  }
  else{
    /* write first piece of data to the first page boundary */
    len = firstpage * pagesize - absoffset;
    fitted_write(bp, absoffset, len, &written);
    bp += len;
    absoffset += len;

    /* now writes blocks at a size of pages (may be no one) */
    len = pagesize;
    while ((n - written) > pagesize){
      fitted_write(bp, absoffset, len, &written);
      bp += len;
      absoffset += len;
    }

    /* write tail */
    len = n - written;
    if (0 == len)
      return written;
    else{
      fitted_write(bp, absoffset, len, &written);
    }
  }

  return written;
}

/**
 *
 */
msg_t EepromMtd::shred(uint8_t pattern){
  (void)pattern;
  osalSysHalt("unrealized");
  return MSG_RESET;
}

/**
 *
 */
size_t EepromMtd::getPageSize(void){
  return eeprom_cfg->pagesize;
}






