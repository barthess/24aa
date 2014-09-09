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
#include "hal.h"

#include "nvram_file.hpp"

using namespace chibios_fs;

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
msg_t NvramFile::put(uint8_t b){
  (void)b;
  osalSysHalt("Unrealized");
  return 0;
}

/**
 *
 */
msg_t NvramFile::get(void){
  osalSysHalt("Unrealized");
  return 0;
}

/**
 *
 */
uint32_t NvramFile::getAndClearLastError(void){
  osalSysHalt("Unrealized");
  return 0;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */
/**
 *
 */
NvramFile::NvramFile(void) {
  return;
}

/**
 * @brief     Special constructor for testing. Do not use it.
 */
void NvramFile::__test_ctor(Mtd *mtd, fileoffset_t start, fileoffset_t size){
  this->mtd = mtd;
  this->tip = 0;
  this->start = start;
  this->size = size;
}

/**
 *
 */
size_t NvramFile::clamp_size(size_t n){
  size_t p = getPosition();
  size_t s = getSize();
  if ((p + n) > s)
    return s - p;
  else
    return n;
}

/**
 *
 */
void NvramFile::close(void){
  this->mtd = NULL;
  this->tip = 0;
  this->start = 0;
  this->size = 0;
}

/**
 *
 */
fileoffset_t NvramFile::getSize(void){
  osalDbgAssert(NULL != this->mtd, "File not opened");
  return size;
}

/**
 *
 */
fileoffset_t NvramFile::getPosition(void){
  osalDbgAssert(NULL != this->mtd, "File not opened");
  return tip;
}

/**
 *
 */
uint32_t NvramFile::setPosition(fileoffset_t offset){
  osalDbgAssert(NULL != this->mtd, "File not opened");

  uint32_t size = getSize();
  if (offset >= size)
    return FILE_ERROR;
  else{
    tip = offset;
    return FILE_OK;
  }
}

/**
 *
 */
size_t NvramFile::read(uint8_t *buf, size_t n){

  osalDbgAssert(NULL != this->mtd, "File not opened");
  osalDbgCheck(NULL != buf);

  n = clamp_size(n);
  if (0 == n)
    return 0;

  if (MSG_OK == mtd->read(buf, n, tip + start)){
    tip += n;
    return n;
  }
  else
    return 0;
}

/**
 *
 */
size_t NvramFile::write(const uint8_t *buf, size_t n){

  osalDbgAssert(NULL != this->mtd, "File not opened");
  osalDbgCheck(NULL != buf);

  n = clamp_size(n);
  if (0 == n)
    return 0;

  if (MSG_OK == mtd->write(buf, n, tip + start)){
    tip += n;
    return n;
  }
  else
    return 0;
}

/**
 *
 */
uint64_t NvramFile::getU64(void){
  uint64_t ret;
  read((uint8_t *)&ret, sizeof(ret));
  return ret;
}

/**
 *
 */
uint32_t NvramFile::getU32(void){
  uint32_t ret;
  read((uint8_t *)&ret, sizeof(ret));
  return ret;
}

/**
 *
 */
uint16_t NvramFile::getU16(void){
  uint16_t ret;
  read((uint8_t *)&ret, sizeof(ret));
  return ret;
}

/**
 *
 */
size_t NvramFile::putU64(uint64_t data){
  return write((uint8_t *)&data, sizeof(data));
}

/**
 *
 */
size_t NvramFile::putU32(uint32_t data){
  return write((uint8_t *)&data, sizeof(data));
}

/**
 *
 */
size_t NvramFile::putU16(uint16_t data){
  return write((uint8_t *)&data, sizeof(data));
}
