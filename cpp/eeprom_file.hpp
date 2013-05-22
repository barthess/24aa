/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_FILE_CPP_H_
#define EEPROM_FILE_CPP_H_

#include "fs.hpp"
#include "eeprom_fs.hpp"

class EepromFile : public chibios_fs::BaseFileStreamInterface{
public:
  EepromFile(void);
  uint32_t getAndClearLastError(void);
  fileoffset_t getSize(void);
  fileoffset_t getPosition(void);
  uint32_t setPosition(fileoffset_t pos);

  size_t write(const uint8_t *bp, size_t n);
  size_t read(uint8_t *bp, size_t n);
  msg_t put(uint8_t b);
  msg_t get(void);

  uint32_t readWord(void);
  uint16_t readHalfWord(void);
  size_t writeWord(uint32_t data);
  size_t writeHalfWord(uint16_t data);
  bool open(EepromFs *fs, uint8_t *name);
  void close(void);

private:
  size_t clamp_size(size_t n);
  EepromFs *fs;
  /**
   * Current position in bytes relative to file start
   */
  fileoffset_t tip;
  inodeid_t inodeid;
};

#endif /* EEPROM_FILE_CPP_H_ */
