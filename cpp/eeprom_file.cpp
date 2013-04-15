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
  this->ready = false;
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

  this->fs = fs;

  inodeid = fs->findInode(name);
  if (-1 != inodeid){
    tip = 0;
    ready = true;
  }

  return ready;
}

/**
 *
 */
void EepromFile::close(void){
  this->fs = NULL;
  ready = false;
}

/**
 *
 */
fileoffset_t EepromFile::getSize(void){
  chDbgCheck(true == ready, "File not opened");
  return fs->getSize(inodeid);
}

/**
 *
 */
fileoffset_t EepromFile::getPosition(void){
  chDbgCheck(true == ready, "File not opened");
  return tip;
}

/**
 *
 */
uint32_t EepromFile::setPosition(fileoffset_t offset){
  chDbgCheck(true == ready, "File not opened");

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

  chDbgCheck(true == ready, "File not opened");
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

  chDbgCheck(true == ready, "File not opened");
  chDbgCheck(NULL != bp, "");

  n = clamp_size(n);
  if (0 == n)
    return 0;

  transferred = fs->write(bp, n, inodeid, tip);
  tip += transferred;
  return transferred;
}
