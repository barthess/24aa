/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
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
  chDbgCheck(NULL != fs, "");
  chDbgCheck(NULL != name, "");

  if (NULL != this->fs) // Allready opened
    return CH_FAILED;

  inodeid = fs->findInode(name);
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
  this->fs = NULL;
}

/**
 *
 */
fileoffset_t EepromFile::getSize(void){
  chDbgCheck(NULL != this->fs, "File not opened");
  return fs->getSize(inodeid);
}

/**
 *
 */
fileoffset_t EepromFile::getPosition(void){
  chDbgCheck(NULL != this->fs, "File not opened");
  return tip;
}

/**
 *
 */
uint32_t EepromFile::setPosition(fileoffset_t offset){
  chDbgCheck(NULL != this->fs, "File not opened");

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

  chDbgCheck(NULL != this->fs, "File not opened");
  chDbgCheck(NULL != bp, "");

  n = clamp_size(n);
  if (0 == n)
    return 0;

  transferred = fs->read(bp, n, inodeid, tip);
  tip += transferred;
  return transferred;
}

/**
 *
 */
size_t EepromFile::write(const uint8_t *bp, size_t n){
  size_t transferred;

  chDbgCheck(NULL != this->fs, "File not opened");
  chDbgCheck(NULL != bp, "");

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





