/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#include "ch.hpp"
#include "hal.h"

#include "eeprom_file.hpp"

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
msg_t EepromFile::put(uint8_t b){
  (void)b;
  osalSysHalt("Unrealized");
  return 0;
}

/**
 *
 */
msg_t EepromFile::get(void){
  osalSysHalt("Unrealized");
  return 0;
}

/**
 *
 */
uint32_t EepromFile::getAndClearLastError(void){
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
EepromFile::EepromFile(void) {
  return;
}

/**
 *
 */
void EepromFile::__test_ctor(Mtd *mtd, fileoffset_t start, fileoffset_t size){
  this->mtd = mtd;
  this->tip = 0;
  this->start = start;
  this->size = size;
}

/**
 *
 */
size_t EepromFile::clamp_size(size_t n){
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
void EepromFile::close(void){
  this->mtd = NULL;
  this->tip = 0;
  this->start = 0;
  this->size = 0;
}

/**
 *
 */
fileoffset_t EepromFile::getSize(void){
  osalDbgAssert(NULL != this->mtd, "File not opened");
  return size;
}

/**
 *
 */
fileoffset_t EepromFile::getPosition(void){
  osalDbgAssert(NULL != this->mtd, "File not opened");
  return tip;
}

/**
 *
 */
uint32_t EepromFile::setPosition(fileoffset_t offset){
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
size_t EepromFile::read(uint8_t *buf, size_t n){

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
size_t EepromFile::write(const uint8_t *buf, size_t n){

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
uint64_t EepromFile::getU64(void){
  uint64_t ret;
  read((uint8_t *)&ret, sizeof(ret));
  return ret;
}

/**
 *
 */
uint32_t EepromFile::getU32(void){
  uint32_t ret;
  read((uint8_t *)&ret, sizeof(ret));
  return ret;
}

/**
 *
 */
uint16_t EepromFile::getU16(void){
  uint16_t ret;
  read((uint8_t *)&ret, sizeof(ret));
  return ret;
}

/**
 *
 */
size_t EepromFile::putU64(uint64_t data){
  return write((uint8_t *)&data, sizeof(data));
}

/**
 *
 */
size_t EepromFile::putU32(uint32_t data){
  return write((uint8_t *)&data, sizeof(data));
}

/**
 *
 */
size_t EepromFile::putU16(uint16_t data){
  return write((uint8_t *)&data, sizeof(data));
}
