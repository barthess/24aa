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

#ifndef MTD_24AA_HPP_
#define MTD_24AA_HPP_

#include "ch.hpp"
#include "hal.h"

#include "mtd_conf.h"
#include "mtd_base.hpp"

namespace nvram {

/**
 *
 */
class Mtd24aa : public MtdBase {
public:
  Mtd24aa(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size,
                                      I2CDriver *i2cp, i2caddr_t addr);
protected:
  size_t bus_write(const uint8_t *txdata, size_t len, uint32_t offset);
  size_t bus_read(uint8_t *rxbuf, size_t len, uint32_t offset);
private:
  bool wait_op_complete(void);
  msg_t i2c_read(uint8_t *rxbuf, size_t len,
                 uint8_t *writebuf, size_t preamble_len);
  msg_t i2c_write(const uint8_t *txdata, size_t len,
                  uint8_t *writebuf, size_t preamble_len);
  msg_t stm32_f1x_read_single_byte(const uint8_t *txbuf, size_t txbytes,
                                   uint8_t *rxbuf, systime_t tmo);
  I2CDriver *i2cp;
  i2caddr_t addr;
  i2cflags_t i2cflags = 0;
  uint32_t bus_clk;
};

} /* namespace */

#endif /* MTD_24AA_HPP_ */
