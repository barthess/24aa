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

#include "mtd_at24.hpp"

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
/**
 *
 */
void MtdAT24::wait_for_sync(void) {
  osalThreadSleep(eeprom_cfg->writetime);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
MtdAT24::MtdAT24(Bus &bus, const AT24Config *eeprom_cfg) :
Mtd(bus),
eeprom_cfg(eeprom_cfg)
{
  osalDbgAssert(sizeof(writebuf) == eeprom_cfg->pagesize + NVRAM_ADDRESS_BYTES,
      "Buffer size must be equal to (pagesize + address bytes)");
}

/**
 *
 */
msg_t MtdAT24::read(uint8_t *data, size_t len, size_t offset) {
  return read_type24(data, len, offset);
}

/**
 *
 */
msg_t MtdAT24::write(const uint8_t *data, size_t len, size_t offset) {

  osalDbgAssert(len != 0, "something broken in higher level");
  osalDbgAssert((offset + len) <= (eeprom_cfg->pages * eeprom_cfg->pagesize),
             "Transaction out of device bounds");
  osalDbgAssert(((offset / eeprom_cfg->pagesize) ==
             ((offset + len - 1) / eeprom_cfg->pagesize)),
             "Data can not be fitted in single page");

  return write_type24(data, len, offset);
}

/**
 *
 */
msg_t MtdAT24::erase() {

  osalDbgAssert(sizeof(writebuf) == eeprom_cfg->pagesize + NVRAM_ADDRESS_BYTES,
          "Buffer size must be equal pagesize + address bytes");

  return erase_type24();
}

/**
 * @brief   Return device capacity in bytes
 */
uint32_t MtdAT24::capacity(void) {
  return eeprom_cfg->pagesize * eeprom_cfg->pages;
}

/**
 *
 */
uint32_t MtdAT24::pagesize(void) {
  return eeprom_cfg->pagesize;
}

} /* namespace */
