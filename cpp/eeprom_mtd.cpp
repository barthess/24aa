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
#include <stdlib.h>

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
 *
 */
void EepromMtd::wait_for_sync(void){
  chThdSleep(eeprom_cfg->writetime);
}

/**
 * @brief   Write data that can be fitted in single page boundary
 */
size_t EepromMtd::fitted_write(const uint8_t *data, size_t len, size_t offset){

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((offset + len) <= (eeprom_cfg->pages * eeprom_cfg->pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((offset / eeprom_cfg->pagesize) ==
             ((offset + len - 1) / eeprom_cfg->pagesize)),
             "Data can not be fitted in single page");

  return write_impl(data, len, offset);
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
  osalDbgAssert(sizeof(writebuf) == eeprom_cfg->pagesize + NVRAM_ADDRESS_BYTES,
      "Buffer size must be equal pagesize + address bytes");
}

/**
 *
 */
msg_t EepromMtd::write(const uint8_t *bp, size_t len, size_t offset){

  /* bytes to be written at one transaction */
  size_t L = 0;
  /* total bytes successfully written */
  size_t written = 0;
  /* cached value */
  uint16_t pagesize = eeprom_cfg->pagesize;
  /* first page to be affected during transaction */
  uint32_t firstpage = offset / pagesize;
  /* last page to be affected during transaction */
  uint32_t lastpage = (offset + len - 1) / pagesize;

  /* data fits in single page */
  if (firstpage == lastpage){
    L = len;
    written += fitted_write(bp, L, offset);
    bp += L;
    offset += L;
    goto EXIT;
  }
  else{
    /* write first piece of data to the first page boundary */
    L = firstpage * pagesize + pagesize - offset;
    written += fitted_write(bp, L, offset);
    bp += L;
    offset += L;

    /* now writes blocks at a size of pages (may be no one) */
    L = pagesize;
    while ((len - written) > pagesize){
      written += fitted_write(bp, L, offset);
      bp += L;
      offset += L;
    }

    /* write tail */
    L = len - written;
    if (0 == L)
      goto EXIT;
    else{
      written += fitted_write(bp, L, offset);
    }
  }

EXIT:
  if (written == len)
    return MSG_OK;
  else
    return MSG_RESET;
}

/**
 *
 */
msg_t EepromMtd::shred(uint8_t pattern){

  osalDbgAssert(sizeof(writebuf) == eeprom_cfg->pagesize + NVRAM_ADDRESS_BYTES,
          "Buffer size must be equal pagesize + address bytes");

  return shred_impl(pattern);
}

/**
 * @brief   Return device capacity in bytes
 */
size_t EepromMtd::capacity(void){
  return eeprom_cfg->pagesize * eeprom_cfg->pages;
}

/**
 * @brief   Move big block of data.
 */
msg_t EepromMtd::datamove(size_t blklen, size_t blkoffset, int32_t shift){
  msg_t status = MSG_RESET;

  osalSysHalt("Unfinished function");

  osalDbgCheck((blklen > 0) && ((blkoffset + blklen) < capacity()));
  osalDbgAssert((shift < 0) && ((blkoffset + shift) <= 0), "Underflow");
  osalDbgAssert((shift > 0) && ((shift + blkoffset) < capacity()), "Overflow");

  if (0 == shift)
    return MSG_OK; /* nothing to do */

  return status;
}







