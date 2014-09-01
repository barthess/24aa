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

#include "ch.hpp"

#include "fram_mtd.hpp"

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
  return;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */
/**
 *
 */
FramMtd::FramMtd(const MtdConfig *mtd_cfg, const FramConfig *fram_cfg):
Mtd(mtd_cfg),
fram_cfg(fram_cfg)
{
  osalDbgAssert(sizeof(writebuf) >= (8 + NVRAM_ADDRESS_BYTES),
      "Buffer size too small for reliable functioning");
}

/**
 *
 */
msg_t FramMtd::write(const uint8_t *data, size_t len, size_t offset){
  if (write_impl(data, len, offset) == len)
    return MSG_OK;
  else
    return MSG_RESET;
}

/**
 * @brief   Return device capacity in bytes
 */
size_t FramMtd::capacity(void){
  return fram_cfg->size;
}

/**
 *
 */
msg_t EepromMtd::shred(uint8_t pattern){

  osalDbgAssert(capacity() % (sizeof(writebuf) - NVRAM_ADDRESS_BYTES),
      "Device size must be divided by write buffer without remainder");

  return shred_impl(pattern);
}

/**
 *
 */
msg_t FramMtd::datamove(size_t blklen, size_t blkoffset, int32_t shift){
  (void)blklen;
  (void)blkoffset;
  (void)shift;
  osalSysHalt("Unrealized");
  return MSG_RESET;
}

