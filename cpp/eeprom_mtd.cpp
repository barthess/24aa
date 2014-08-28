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

#include <string.h>

#include "ch.hpp"

#include "eeprom_mtd.hpp"

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
void EepromMtd::fitted_write(const uint8_t *data, size_t len,
                            size_t offset, uint32_t *written){
  msg_t status = MSG_RESET;

  eeprom_led_on();

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((offset + len) <= (eeprom_cfg->pages * eeprom_cfg->pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((offset / eeprom_cfg->pagesize) ==
             ((offset + len - 1) / eeprom_cfg->pagesize)),
             "Data can not be fitted in single page");

  this->acquire();

  split_addr(writebuf, offset);          /* write address bytes */
  memcpy(&(writebuf[2]), data, len);        /* write data bytes */
  status = busTransmit(writebuf, len+2);

  /* wait until EEPROM process data */
  chThdSleep(eeprom_cfg->writetime);
  this->release();
  eeprom_led_off();

  if (status == MSG_OK){
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
  osalDbgAssert(sizeof(writebuf) == eeprom_cfg->pagesize + 2,
          "Buffer size must be equal pagesize + 2 bytes for address");
}

/**
 *
 */
msg_t EepromMtd::write(const uint8_t *bp, size_t n, size_t offset){

  /* bytes to be written at one transaction */
  size_t len = 0;
  /* total bytes successfully written */
  uint32_t written = 0;
  /* cached value */
  uint16_t pagesize = eeprom_cfg->pagesize;
  /* first page to be affected during transaction */
  uint32_t firstpage = offset / pagesize;
  /* last page to be affected during transaction */
  uint32_t lastpage = (offset + n - 1) / pagesize;

  /* data fits in single page */
  if (firstpage == lastpage){
    len = n;
    fitted_write(bp, len, offset, &written);
    bp += len;
    offset += len;
    goto EXIT;
  }
  else{
    /* write first piece of data to the first page boundary */
    len = firstpage * pagesize + pagesize - offset;
    fitted_write(bp, len, offset, &written);
    bp += len;
    offset += len;

    /* now writes blocks at a size of pages (may be no one) */
    len = pagesize;
    while ((n - written) > pagesize){
      fitted_write(bp, len, offset, &written);
      bp += len;
      offset += len;
    }

    /* write tail */
    len = n - written;
    if (0 == len)
      goto EXIT;
    else{
      fitted_write(bp, len, offset, &written);
    }
  }

EXIT:
  if (written == n)
    return MSG_OK;
  else
    return MSG_RESET;
}

/**
 *
 */
msg_t EepromMtd::shred(uint8_t pattern){

  msg_t status = MSG_RESET;

  eeprom_led_on();
  this->acquire();

  for (size_t i=0; i<eeprom_cfg->pages; i++){
    memset(writebuf, pattern, sizeof(writebuf));
    split_addr(writebuf, i * eeprom_cfg->pagesize);
    status = busTransmit(writebuf, sizeof(writebuf));
    osalDbgCheck(MSG_OK == status);
    chThdSleep(eeprom_cfg->writetime);
  }

  this->release();
  eeprom_led_off();
  return MSG_OK;
}

/**
 * @brief   Return device capacity in bytes
 */
size_t EepromMtd::capacity(void){
  return eeprom_cfg->pagesize * eeprom_cfg->pages;
}




