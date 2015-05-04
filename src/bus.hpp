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

#ifndef NVRAM_BUS_HPP_
#define NVRAM_BUS_HPP_

#include "ch.hpp"
#include "hal.h"

namespace nvram {

struct BusRequest {
  /**
   * @brief Convenience function.
   */
  void fill(const uint8_t *txdata, size_t txbytes, uint8_t *rxdata,
                    size_t rxbytes, uint8_t *writebuf, size_t preamble_len) {
    this->txdata        = txdata;
    this->txbytes       = txbytes;
    this->rxdata        = rxdata;
    this->rxbytes       = rxbytes;
    this->writebuf      = writebuf;
    this->preamble_len  = preamble_len;
  }

  const uint8_t   *txdata = nullptr;
  size_t          txbytes = 0;
  uint8_t         *rxdata = nullptr;
  size_t          rxbytes = 0;
  /**
   * @brief   Pointer to working area allocated in higher level.
   * @details Size of buffer must be enough to store preamble + txdata.
   * @details Start of buffer my contain some CMD or ADDR data called preamble.
   */
  uint8_t         *writebuf = nullptr;
  /**
   * @brief   Preamble bytes count written in higher leve. May be 0.
   */
  size_t          preamble_len = 0;
};

/**
 *
 */
class Bus {
public:
  virtual msg_t exchange(const BusRequest &request) = 0;
};

} /* namespace */

#endif /* NVRAM_BUS_HPP_ */
