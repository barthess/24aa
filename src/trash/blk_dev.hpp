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

#ifndef NVRAM_BLK_DEV_HPP_
#define NVRAM_BLK_DEV_HPP_

#include "mtd.hpp"

namespace nvram {

/**
 *
 */
class BlkDev {
public:
  BlkDev(Mtd &mtd);
  msg_t write(const uint8_t *data, size_t len, size_t offset);

  msg_t read(uint8_t *data, size_t len, size_t offset) {
    return mtd.read(data, len, offset);
  }

  msg_t erase(void){
    return mtd.erase();
  }

  size_t capacity(void) {
    return mtd.capacity();
  }
private:
  Mtd &mtd;
};

} /* namespace */

#endif /* NVRAM_BLK_DEV_HPP_ */
