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

#ifndef EEPROM_FILE_CPP_H_
#define EEPROM_FILE_CPP_H_

#include "fs.hpp"
#include "mtd.hpp"

class EepromFile : public chibios_fs::BaseFileStreamInterface{
  friend class EepromFs;
public:
  EepromFile(void);
  void __test_ctor(Mtd *mtd, chibios_fs::fileoffset_t start, chibios_fs::fileoffset_t size);
  uint32_t getAndClearLastError(void);
  chibios_fs::fileoffset_t getSize(void);
  chibios_fs::fileoffset_t getPosition(void);
  uint32_t setPosition(chibios_fs::fileoffset_t pos);

  size_t write(const uint8_t *buf, size_t n);
  size_t read(uint8_t *buf, size_t n);
  msg_t put(uint8_t b);
  msg_t get(void);

  uint64_t getU64(void);
  uint32_t getU32(void);
  uint16_t getU16(void);
  size_t putU64(uint64_t data);
  size_t putU32(uint32_t data);
  size_t putU16(uint16_t data);

private:
  size_t clamp_size(size_t n);
  void close(void);
  Mtd *mtd;
  uint16_t start; /* file start in bytes relative to device start */
  uint16_t size;  /* size (bytes) */
  uint16_t tip;   /* current position in file (bytes) */
};

#endif /* EEPROM_FILE_CPP_H_ */
