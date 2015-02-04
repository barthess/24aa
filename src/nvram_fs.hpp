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

#ifndef NVRAM_FS_HPP_
#define NVRAM_FS_HPP_

#include "mtd.hpp"
#include "nvram_file.hpp"

#if !defined(NVRAM_FS_MAX_FILE_NAME_LEN)
#define NVRAM_FS_MAX_FILE_NAME_LEN        8
#endif

#if !defined(NVRAM_FS_MAX_FILE_CNT)
#define NVRAM_FS_MAX_FILE_CNT             3
#endif

#if NVRAM_FS_USE_DELETE_AND_RESIZE
#warning "Experimental untested feature enabled"
#endif

namespace nvram {

/**
 *
 */
typedef struct {
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
} toc_item_t;

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
  Fs(Mtd &mtd);
  File *open(const char *name);
  void close(File *f);
  File *create(const char *name, chibios_fs::fileoffset_t size);
  bool mount(void);
  bool umount(void);
  bool mkfs(void);
  bool fsck(void);
  chibios_fs::fileoffset_t df(void);
#if NVRAM_FS_USE_DELETE_AND_RESIZE
  bool rm(const char *name);
  bool resize(const char *name, size_t newsize);
#endif
private:
  uint8_t get_checksum(void);
  uint8_t get_file_cnt(void);
  void write_file_cnt(uint8_t cnt);
  void get_magic(uint8_t *result);
  void read_toc_item(toc_item_t *result, size_t num);
  void write_toc_item(const toc_item_t *result, uint8_t num);
  void open_super(void);
  void seal(void);
#if NVRAM_FS_USE_DELETE_AND_RESIZE
  void lock(void);
  void unlock(void);
#endif
  int find(const char *name, toc_item_t *ti);
  Mtd &mtd;
  File super;
  File fat[NVRAM_FS_MAX_FILE_CNT];
  /* counter of opened files. In unmounted state this value must be 0.
   * After mounting it must be set to 1 denoting successful mount. Every
   * 'open' operation must increment it and every 'close' must decrement it. */
  filecount_t files_opened;
};

} /* namespace */

#endif /* NVRAM_FS_HPP_ */
