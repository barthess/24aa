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
#include "eeprom_fs_conf.h"

/**
 *
 */
typedef struct inode_t{
  /**
   * first page of file
   */
  uint16_t startpage;
  /**
   * first byte of file relative to page start
   */
  uint16_t pageoffset;
  /**
   * file size in bytes
   */
  uint16_t size;
}inode_t;

/**
 *
 */
typedef struct toc_record_t{
  char *name;
  inode_t inode;
}toc_record_t;

/**
 * type representing index in inode table
 */
typedef int32_t inodeid_t;

/**
 *
 */
class EepromFs{
public:
  /**
   * Constructor
   */
  EepromFs(EepromMtd *mtd, const toc_record_t *reftoc, size_t N);
  /**
   * Read inode table from EEPROM to RAM
   */
  void mount(void);
  /**
   *
   */
  bool umount(void);
  /**
   * Return size of file. Designed for use in higher level functions.
   */
  size_t getSize(inodeid_t inodeid);
  /**
   * Return inode index in table for given name
   * -1 if not found
   */
  inodeid_t open(const uint8_t *name);
  /**
   * Return inode index in table for given name
   * -1 if not found
   */
  bool close(inodeid_t inodeid);
  /**
   * Designed to be called from higher level
   */
  size_t write(const uint8_t *bp, size_t n, inodeid_t inodeid, fileoffset_t tip);
  /**
   * Designed to be called from higher level
   */
  size_t read(uint8_t *bp, size_t n, inodeid_t inodeid, fileoffset_t tip);


private:
   /**
   * Mass erase EEPROM and write TOC hardcoded in firmware
   */
  void mkfs(void);
  /**
   * Check correspondace between hadcoded TOC and EEPROM once.
   * If not equal that perform mkfs.
   * At the end raise "clean" flag
   */
  bool fsck(void);
  /**
   *
   */
  void fitted_write(const uint8_t *data, size_t absoffset, size_t len, uint32_t *written);
  /**
   *
   */
  void toc2buf(const toc_record_t *reftoc, uint8_t *b);
  /**
   *
   */
  void buf2toc(const uint8_t *b, toc_record_t *toc);
  /**
   *
   */
  size_t inode2absoffset(const inode_t *inode, fileoffset_t tip);
  /**
   *
   */
  bool overlap(const inode_t *inode0, const inode_t *inode1);
  /**
   *
   */
  void read_toc_record(uint32_t N, toc_record_t *rec);
  /**
   *
   */
  EepromMtd *mtd;
  /* reference table of content */
  const toc_record_t *reftoc;
  /**/
  inode_t inode_table[EEPROM_FS_MAX_FILE_COUNT];
  /* counter of opened files. In unmounted state this value must be 0.
   * After mounting it must be set to 1 denoting successful mount. Every
   * 'open' operation must increment it and every 'close' must decrement it. */
  uint8_t files_opened;
};

#endif /* EEPROM_FS_HPP_ */
