/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_MTD_HPP_
#define EEPROM_MTD_HPP_

#include "mtd.hpp"

/**
 *
 */
typedef struct {
  /**
   * Time needed by IC for single page writing.
   * Check datasheet!!!
   */
  systime_t     writetime;
  /**
   * Size of memory array in pages.
   * Check datasheet!!!
   */
  size_t        pages;
  /**
   * Size of single page in bytes.
   * Check datasheet!!!
   */
  size_t        pagesize;
}EepromConfig;

/**
 *
 */
class EepromMtd : public Mtd {
public:
  EepromMtd(const MtdConfig *mtd_cfg, const EepromConfig *eeprom_cfg);
  msg_t write(const uint8_t *data, size_t len, size_t offset);
  msg_t shred(uint8_t pattern);
  size_t capacity(void);
  size_t page_size(void);
  msg_t move(size_t blklen, size_t blkoffset, int32_t shift);
private:
  void fitted_write(const uint8_t *data, size_t len, size_t offset, uint32_t *written);
  const EepromConfig *eeprom_cfg;
};

#endif /* EEPROM_MTD_HPP_ */
