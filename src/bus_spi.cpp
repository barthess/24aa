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

#include <cstring>

#include "ch.hpp"

#include "bus_spi.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * EXTERNS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * PROTOTYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
BusSPI::BusSPI(SPIDriver *spip) :
    spip(spip)
{
  return;
}

/**
 *
 */
msg_t BusSPI::read(uint8_t *rxbuf, size_t len,
                   uint8_t *writebuf, size_t preamble_len) {

#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(this->spip);
#endif

  osalDbgCheck((nullptr != rxbuf) && (0 != len));

  spiSelect(spip);
  //spiPolledExchange(spip, frame);
  spiSend(spip, preamble_len, writebuf);
  spiReceive(spip, len, rxbuf);
  spiUnselect(spip);

#if SPI_USE_MUTUAL_EXCLUSION
  spiReleaseBus(this->spip);
#endif

  return MSG_OK;
}

/**
 *
 */
msg_t BusSPI::write(const uint8_t *txdata, size_t len,
                    uint8_t *writebuf, size_t preamble_len) {

#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(this->spip);
#endif

  if ((nullptr != txdata) && (0 != len))
    memcpy(&writebuf[preamble_len], txdata, len);

  spiSelect(spip);
  //spiPolledExchange(spip, frame);
  spiSend(spip, preamble_len + len, writebuf);
  spiUnselect(spip);

#if SPI_USE_MUTUAL_EXCLUSION
  spiReleaseBus(this->spip);
#endif

  return MSG_OK;
}
} /* namespace */
