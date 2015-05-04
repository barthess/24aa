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

#ifndef NVRAM_BUS_SPI_HPP_
#define NVRAM_BUS_SPI_HPP_

#include "ch.hpp"
#include "hal.h"

#include "bus.hpp"

namespace nvram {

/**
 *
 */
class BusSPI : public Bus {
public:
  BusSPI(SPIDriver *spip);
  msg_t read(uint8_t *rxbuf, size_t len,
             uint8_t *writebuf, size_t preamble_len);
  msg_t write(const uint8_t *txdata, size_t len,
              uint8_t *writebuf, size_t preamble_len);
private:
  SPIDriver *spip;
};

} /* namespace */

#endif /* NVRAM_BUS_SPI_HPP_ */
