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
void FramMtd::wait_for_sync(void){
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

  size_t written = 0;
  const size_t blocksize = sizeof(writebuf) - NVRAM_ADDRESS_BYTES;
  const size_t big_writes = len / blocksize;
  const size_t small_write_size = len % blocksize;

  /* write big blocks */
  for (size_t i=0; i<big_writes; i++){
    written = write_impl(data, blocksize, offset);
    if (blocksize == written){
      data += written;
      offset += written;
    }
    else
      return MSG_RESET;
  }

  /* write tail (if any) */
  if (small_write_size > 0){
    written = write_impl(data, small_write_size, offset);
    if (small_write_size != written)
      return MSG_RESET;
  }

  return MSG_OK; /* */
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
msg_t FramMtd::shred(uint8_t pattern){

  osalDbgAssert(0 == (capacity() % (sizeof(writebuf) - NVRAM_ADDRESS_BYTES)),
      "Device size must be divided by write buffer without remainder");

  return shred_impl(pattern);
}

/**
 *
 */
msg_t FramMtd::datamove(size_t blklen, size_t blkoffset, int32_t shift){
  msg_t status = MSG_RESET;

  osalSysHalt("Unfinished function");

  osalDbgAssert((blklen > 0) && ((blkoffset + blklen) < capacity()), "Not enough space");
  osalDbgAssert((shift < 0) && ((blkoffset + shift) <= 0), "Underflow");
  osalDbgAssert((shift > 0) && ((shift + blkoffset) < capacity()), "Overflow");

  if (0 == shift)
    return MSG_OK; /* nothing to do */

  return status;
}

