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

#include <string.h>
#include <stdlib.h>

#include "ch.hpp"

#include "blk_dev.hpp"

namespace nvram {

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

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
BlkDev::BlkDev(Mtd &mtd):
mtd(mtd)
{
  return;
}

/**
 * @brief   Splits big transaction into smaller ones fitted into memory page.
 */
msg_t BlkDev::write(const uint8_t *data, size_t len, size_t offset) {

  /* bytes to be written at one transaction */
  size_t L = 0;
  /* total bytes successfully written */
  size_t written = 0;
  /* cached value */
  uint32_t pagesize = mtd.pagesize();
  /* first page to be affected during transaction */
  uint32_t firstpage = offset / pagesize;
  /* last page to be affected during transaction */
  uint32_t lastpage = (offset + len - 1) / pagesize;

  /* data fits in single page */
  if (firstpage == lastpage) {
    L = len;
    written += mtd.write(data, L, offset);
    data += L;
    offset += L;
    goto EXIT;
  }
  else{
    /* write first piece of data to the first page boundary */
    L = firstpage * pagesize + pagesize - offset;
    written += mtd.write(data, L, offset);
    data += L;
    offset += L;

    /* now writes blocks at a size of pages (may be no one) */
    L = pagesize;
    while ((len - written) > pagesize){
      written += mtd.write(data, L, offset);
      data += L;
      offset += L;
    }

    /* write tail */
    L = len - written;
    if (0 == L)
      goto EXIT;
    else{
      written += mtd.write(data, L, offset);
    }
  }

EXIT:
  if (written == len)
    return MSG_OK;
  else
    return MSG_RESET;
}

} /* namespace */
