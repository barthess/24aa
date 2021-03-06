/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

    This file is part of 24AA lib.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.hpp"
#include "hal.h"

#include "nvram_file.hpp"

using namespace chibios_fs;
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
msg_t File::put(uint8_t b){
  (void)b;
  osalSysHalt("Unrealized");
  return 0;
}

/**
 *
 */
msg_t File::get(void){
  osalSysHalt("Unrealized");
  return 0;
}

/**
 *
 */
uint32_t File::getAndClearLastError(void){
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
File::File(void) :
mtd(nullptr), start(0), size(0), tip(0)
{
  return;
}

/**
 * @brief     Special constructor for testing. Do not use it.
 */
void File::__test_ctor(MtdBase *mtd, fileoffset_t start, fileoffset_t size){
  this->mtd = mtd;
  this->tip = 0;
  this->start = start;
  this->size = size;
}

/**
 *
 */
size_t File::clamp_size(size_t n){
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
void File::close(void){
  this->mtd = nullptr;
  this->tip = 0;
  this->start = 0;
  this->size = 0;
}

/**
 *
 */
fileoffset_t File::getSize(void){
  osalDbgAssert(nullptr != this->mtd, "File not opened");
  return size;
}

/**
 *
 */
fileoffset_t File::getPosition(void){
  osalDbgAssert(nullptr != this->mtd, "File not opened");
  return tip;
}

/**
 *
 */
uint32_t File::setPosition(uint32_t offset){
  osalDbgAssert(nullptr != this->mtd, "File not opened");

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
size_t File::read(uint8_t *buf, size_t n) {
  size_t acquired;

  osalDbgAssert(nullptr != this->mtd, "File not opened");
  osalDbgCheck(nullptr != buf);

  n = clamp_size(n);
  if (0 == n)
    return 0;

  acquired = mtd->read(buf, n, tip + start);
  tip += acquired;
  return acquired;
}

/**
 *
 */
size_t File::write(const uint8_t *buf, size_t n) {

  size_t written;

  osalDbgAssert(nullptr != this->mtd, "File not opened");
  osalDbgCheck(nullptr != buf);

  n = clamp_size(n);
  if (0 == n)
    return 0;

  written = mtd->write(buf, n, tip + start);
  tip += written;
  return written;
}

} /* namespace */
