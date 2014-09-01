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

#ifndef EEPROM_MTD_HPP_
#define EEPROM_MTD_HPP_

#include "mtd.hpp"

/**
 *
 */
typedef struct {
  /**
   * Time needed by IC for single page writing.
   * Consult datasheet!!!
   */
  systime_t     writetime;
  /**
   * Size of memory array in pages.
   * Consult datasheet!!!
   */
  size_t        pages;
  /**
   * Size of single page in bytes.
   * Consult datasheet!!!
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
