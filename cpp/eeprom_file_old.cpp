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
  chDbgPanic("Unsupported");
  return 0;
}

/**
 *
 */
msg_t EepromFile::get(void){
  chDbgPanic("Unsupported");
  return 0;
}

/**
 *
 */
uint32_t EepromFile::getAndClearLastError(void){
  chDbgPanic("Unsupported");
  return 0;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

EepromFile::EepromFile(void){
  this->fs = NULL;
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
bool EepromFile::open(EepromFs *fs, uint8_t *name){
  osalDbgCheck(NULL != fs);
  osalDbgCheck(NULL != name);

  if (NULL != this->fs) // Allready opened
    return CH_FAILED;

  inodeid = fs->open(name);
  if (-1 != inodeid){
    tip = 0;
    this->fs = fs;
    return CH_SUCCESS;
  }
  else
    return CH_FAILED;
}

/**
 *
 */
void EepromFile::close(void){
  fs->close(this->inodeid);
  this->fs = NULL;
}

/**
 *
 */
fileoffset_t EepromFile::getSize(void){
  osalDbgAssert(NULL != this->fs, "File not opened");
  return fs->getSize(inodeid);
}

/**
 *
 */
fileoffset_t EepromFile::getPosition(void){
  osalDbgAssert(NULL != this->fs, "File not opened");
  return tip;
}

/**
 *
 */
uint32_t EepromFile::setPosition(fileoffset_t offset){
  osalDbgAssert(NULL != this->fs, "File not opened");

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
size_t EepromFile::read(uint8_t *bp, size_t n){
  size_t transferred;

  osalDbgAssert(NULL != this->fs, "File not opened");
  chDbgCheck(NULL != bp);

  n = clamp_size(n);
  if (0 == n)
    return 0;

  /*
   * Ugly workaround.
   * Stupid I2C cell in STM32F1x does not allow to read single byte.
   * So we must read 2 bytes and return needed one.
   */
#if defined(STM32F1XX_I2C)
  if (n == 1){
    uint8_t buf[2];
    /* if NOT last byte of file requested */
    if ((getPosition() + 1) < getSize()){
      if (2 == read(buf, 2)){
        setPosition(getPosition() + 1);
        bp[0] = buf[0];
        return 1;
      }
      else
        return 0;
    }
    else{
      setPosition(getPosition() - 1);
      if (2 == read(buf, 2)){
        setPosition(getPosition() + 2);
        bp[0] = buf[1];
        return 1;
      }
      else
        return 0;
    }
  }
#endif /* defined(STM32F1XX_I2C) */

  transferred = fs->read(bp, n, inodeid, tip);
  tip += transferred;
  return transferred;
}

/**
 *
 */
size_t EepromFile::write(const uint8_t *bp, size_t n){
  size_t transferred;

  osalDbgAssert(NULL != this->fs, "File not opened");
  osalDbgCheck(NULL != bp);

  n = clamp_size(n);
  if (0 == n)
    return 0;

  transferred = fs->write(bp, n, inodeid, tip);
  tip += transferred;
  return transferred;
}

/**
 *
 */
uint32_t EepromFile::readWord(void){
  uint8_t buf[4];
  read(buf, sizeof(buf));
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

/**
 *
 */
size_t EepromFile::writeWord(uint32_t data){
  uint8_t buf[4];
  buf[0] = (data >> 24) & 0xFF;
  buf[1] = (data >> 16) & 0xFF;
  buf[2] = (data >> 8)  & 0xFF;
  buf[3] =  data        & 0xFF;
  return write(buf, sizeof(buf));
}

/**
 *
 */
uint16_t EepromFile::readHalfWord(void){
  uint8_t buf[2];
  read(buf, sizeof(buf));
  return (buf[0] << 8) | buf[1];
}

/**
 *
 */
size_t EepromFile::writeHalfWord(uint16_t data){
  uint8_t buf[2];
  buf[0] = (data >> 8)  & 0xFF;
  buf[1] =  data        & 0xFF;
  return write(buf, sizeof(buf));
}
