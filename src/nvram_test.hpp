/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

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

#ifndef NVRAM_TEST_HPP_
#define NVRAM_TEST_HPP_

#include "mtd.hpp"

namespace nvram {

/**
 *
 */
struct TestContext {
  nvram::Mtd            *mtd;
  uint8_t               *mtdbuf;
  uint8_t               *refbuf;
  uint8_t               *filebuf;
  size_t                len;      // length of buffers
  BaseSequentialStream  *chn;     // debug output. Set to nullptr if unused
};

bool TestSuite(TestContext *ctx);

} // namespace

#endif /* NVRAM_TEST_HPP_ */
