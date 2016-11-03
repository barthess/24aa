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

#ifndef MTD25_HPP_
#define MTD25_HPP_

#include "ch.hpp"
#include "hal.h"

#include "mtd_conf.h"
#include "mtd.hpp"

namespace nvram {

/**
 *
 */
class Mtd25 : public Mtd {
public:
  Mtd25(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size, SPIDriver *spip);
protected:
  size_t bus_write(const uint8_t *txdata, size_t len, uint32_t offset);
  size_t bus_read(uint8_t *rxbuf, size_t len, uint32_t offset);
  msg_t bus_erase(void);
private:
  msg_t spi_write_enable(void);
  msg_t wait_op_complete(systime_t timeout);
  msg_t spi_read(uint8_t *rxbuf, size_t len,
                 uint8_t *writebuf, size_t preamble_len);
  msg_t spi_write(const uint8_t *txdata, size_t len,
                  uint8_t *writebuf, size_t preamble_len);
  SPIDriver *spip;
};

} /* namespace */

#endif /* MTD24_HPP_ */
