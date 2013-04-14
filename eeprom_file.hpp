/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#ifndef EEPROM_FILE_CPP_H_
#define EEPROM_FILE_CPP_H_

#include "ch.hpp"
#include "fs.hpp"
#include "hal.h"
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

  bool open(EepromFs *fs, uint8_t *name);
  void close(void);

private:
  size_t clamp_size(size_t n);
  bool ready;
  EepromFs *fs;
  /**
   * Current position in bytes relative to file start
   */
  fileoffset_t tip;
  inodeid_t inodeid;
};

#endif /* EEPROM_FILE_CPP_H_ */
