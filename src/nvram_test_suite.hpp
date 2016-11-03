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

#ifndef NVRAM_TEST_SUITE_HPP_
#define NVRAM_TEST_SUITE_HPP_

#include "mtd_base.hpp"

namespace nvram {

/**
 *
 */
struct TestContext {
  nvram::MtdBase        *mtd;
  uint8_t               *mtdbuf;
  uint8_t               *refbuf;
  uint8_t               *filebuf;
  size_t                len;      // length of buffers
  BaseSequentialStream  *chn;     // debug output. Set to nullptr if unused
};

bool TestSuite(TestContext *ctx);

} // namespace

#endif /* NVRAM_TEST_SUITE_HPP_ */
