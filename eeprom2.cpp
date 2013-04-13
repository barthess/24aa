/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#include "ch.hpp"
#include "hal.h"

#include "eeprom2.hpp"

myEepromFile::myEepromFile(const I2CEepromFileConfig_2 *cfg){
  this->cfg = cfg;
  return;
}

uint32_t myEepromFile::getAndClearLastError(void){
  return 0;
}

fileoffset_t myEepromFile::getSize(void){
  return 0;
}

fileoffset_t myEepromFile::getPosition(void){
  return 0;
}

uint32_t myEepromFile::setPosition(fileoffset_t offset){
  this->offset = offset;
  return 0;
}

size_t myEepromFile::write(const uint8_t *bp, size_t n){
  (void)n;
  (void)bp;
  return 0;
}

size_t myEepromFile::read(uint8_t *bp, size_t n){
  (void)n;
  (void)bp;
  return 0;
}

msg_t myEepromFile::put(uint8_t b){
  (void)b;
  return 0;
}

msg_t myEepromFile::get(void){
  return 0;
}
