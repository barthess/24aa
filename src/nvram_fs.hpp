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

#ifndef NVRAM_FS_HPP_
#define NVRAM_FS_HPP_

#include <cstring>

#include "mtd_base.hpp"
#include "nvram_file.hpp"

#include "nvram_fs_conf.h"

namespace nvram {

/**
 *
 */
struct __attribute__((packed)) toc_item_t {
  /**
   * Default constructor.
   */
  toc_item_t(void) {
    memset(this, 0, sizeof(*this));
  }
  /**
   * NULL terminated string representing human readable name.
   */
  char name[NVRAM_FS_MAX_FILE_NAME_LEN];
  /**
   * Start of file
   */
  uint16_t start;
  /**
   * Size of file
   */
  uint16_t size;
};

/**
 *
 */
typedef uint8_t filecount_t;

/**
 *
 */
typedef uint8_t checksum_t;

/**
 *
 */
class Fs {
public:
  Fs(MtdBase &mtd);
  File *open(const char *name);
  void close(File *file);
  File *create(const char *name, uint32_t size);
  bool mount(void);
  bool is_mounted(void);
  bool umount(void);
  bool mkfs(void);
  bool fsck(void);
  uint32_t df(void);
private:
  checksum_t get_checksum(void);
  filecount_t get_file_cnt(void);
  void write_file_cnt(filecount_t cnt);
  void get_magic(uint8_t *result);
  void read_toc_item(toc_item_t *result, size_t num);
  void write_toc_item(const toc_item_t *result, size_t num);
  void open_super(void);
  void seal(void);
  int find(const char *name, toc_item_t *ti);
  MtdBase &mtd;
  File super;
  File fat[NVRAM_FS_MAX_FILE_CNT];
  uint8_t toc_buf[sizeof(toc_item_t)];
  /* Counter for opened files. In unmounted state this value must be 0.
   * After mounting it must be set to 1 denoting successful mount. Every
   * 'open' must increment it and every 'close' must decrement it. */
  filecount_t files_opened;
};

} /* namespace */

#endif /* NVRAM_FS_HPP_ */
