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

#ifndef NVRAM_FILE_HPP_
#define NVRAM_FILE_HPP_

#include "fs.hpp"
#include "mtd_base.hpp"

namespace nvram {

/**
 *
 */
class File : public chibios_fs::BaseFileStreamInterface {
  friend class Fs;
public:
  File(void);
  void __test_ctor(MtdBase *mtd, uint32_t start, uint32_t size);
  uint32_t getAndClearLastError(void);
  uint32_t getSize(void);
  uint32_t getPosition(void);
  uint32_t setPosition(uint32_t pos);

  size_t write(const uint8_t *buf, size_t n);
  size_t read(uint8_t *buf, size_t n);
  msg_t put(uint8_t b);
  msg_t get(void);

private:
  size_t clamp_size(size_t n);
  void close(void);
  MtdBase *mtd;
  uint16_t start; /* file start in bytes relative to device start */
  uint16_t size;  /* size (bytes) */
  uint16_t tip;   /* current position in file (bytes) */
};

} /* namespace */

#endif /* NVRAM_FILE_HPP_ */
