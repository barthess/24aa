/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_FS_HPP_
#define EEPROM_FS_HPP_

#include "eeprom_mtd.hpp"

#define EEPROM_FS_MAX_FILES     4

/**
 *
 */
typedef struct inode_t {
  /**
   * first byte of file relative to memory start
   */
  uint16_t offset;
  /**
   * file size in bytes
   */
  uint16_t size;
}inode_t;

/**
 *
 */
typedef struct toc_item_t {
  /**
   * NULL terminated string representing human readable name.
   */
  char name[8];
  /**
   * Number of file
   */
  inode_t inode;
}toc_item_t;

/**
 * type representing index in inode table
 */
typedef int32_t inodeid_t;

/**
 *
 */
class EepromFs {
public:
  /*
   * Constructor
   */
  EepromFs(Mtd &mtd);
  /*
   * Designed to be called from higher level
   */
  size_t _write(const uint8_t *bp, size_t n, inodeid_t inodeid, fileoffset_t tip);
  /*
   * Designed to be called from higher level
   */
  size_t _read(uint8_t *buf, size_t n, inodeid_t inode, fileoffset_t tip);
  /*
   *
   */
  void mount(void);
private:
  /*
   *
   */
  void fsck();
  /*
   *
   */
  void mkfs();
  /*
   *
   */
  Mtd &mtd;
  /*
   *
   */
  toc_item_t file_table[EEPROM_FS_MAX_FILES];
  /* counter of opened files. In unmounted state this value must be 0.
   * After mounting it must be set to 1 denoting successful mount. Every
   * 'open' operation must increment it and every 'close' must decrement it. */
  uint8_t files_opened;
};

#endif /* EEPROM_FS_HPP_ */
