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

#ifndef NVRAM_BUS_I2C_HPP_
#define NVRAM_BUS_I2C_HPP_

#include "ch.hpp"
#include "hal.h"

#include "bus.hpp"

namespace nvram {

/**
 *
 */
class BusI2C : public Bus {
public:
  BusI2C(I2CDriver *i2cp, i2caddr_t addr);
  msg_t exchange(const uint8_t *txbuf, size_t txbytes,
                       uint8_t *rxbuf, size_t rxbytes);
private:
  msg_t stm32_f1x_read_single_byte(const uint8_t *txbuf, size_t txbytes,
                                         uint8_t *rxbuf, systime_t tmo);
  systime_t calc_timeout(size_t bytes);
  I2CDriver *i2cp;
  i2caddr_t addr;
  i2cflags_t i2cflags;
};

} /* namespace */

#endif /* NVRAM_BUS_HPP_ */
