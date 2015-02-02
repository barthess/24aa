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

#ifndef FRAM_MTD_HPP_
#define FRAM_MTD_HPP_

#include "mtd.hpp"

namespace nvram {

/**
 *
 */
typedef struct {
  /**
   * Size of memory array in bytes.
   * Consult datasheet!!!
   */
  size_t        size;
}FramConfig;

/**
 *
 */
class FramMtd : public Mtd {
public:
  FramMtd(const MtdConfig *mtd_cfg, const FramConfig *fram_cfg);
  msg_t write(const uint8_t *data, size_t len, size_t offset);
  msg_t shred(uint8_t pattern);
  size_t capacity(void);
  msg_t datamove(size_t blklen, size_t blkoffset, int32_t shift);
private:
  void wait_for_sync(void);
  void fitted_write(const uint8_t *data, size_t len, size_t offset, uint32_t *written);
  const FramConfig *fram_cfg;
};

} /* namespace */

#endif /* FRAM_MTD_HPP_ */
