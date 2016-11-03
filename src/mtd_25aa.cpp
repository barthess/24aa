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

#include <cstring>
#include <cstdlib>

#include "ch.hpp"

#include "mtd_25aa.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

#define CMD_25AA_WRSR     0x01  // Write status register
#define CMD_25AA_WRITE    0x02
#define CMD_25AA_READ     0x03
#define CMD_25AA_WRDI     0x04  // Write Disable
#define CMD_25AA_RDSR     0x05  // Read Status Register
#define CMD_25AA_WREN     0x06  // Write Enable

#define STATUS_25AA_WEL   0b00000010  // write enable latch (1 == write enable)
#define STATUS_25AA_WIP   0b00000001  // write in progress

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
/**
 *
 */
msg_t Mtd25aa::spi_read(uint8_t *rxbuf, size_t len,
                        uint8_t *writebuf, size_t preamble_len) {

#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(this->spip);
#endif

  osalDbgCheck((nullptr != rxbuf) && (0 != len));

  spiSelect(spip);
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
msg_t Mtd25aa::spi_write(const uint8_t *txdata, size_t len,
                         uint8_t *writebuf, size_t preamble_len) {

  if ((nullptr != txdata) && (len > 0)) {
    memcpy(&writebuf[preamble_len], txdata, len);
  }

#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(this->spip);
#endif

  spiSelect(spip);
  spiPolledExchange(spip, CMD_25AA_WREN);
  spiUnselect(spip);

  spiSelect(spip);
  spiSend(spip, preamble_len + len, writebuf);
  spiUnselect(spip);

#if SPI_USE_MUTUAL_EXCLUSION
  spiReleaseBus(this->spip);
#endif

  if (OSAL_SUCCESS == wait_op_complete())
    return MSG_OK;
  else
    return MSG_RESET;
}

/*
 *
 */
bool Mtd25aa::wait_op_complete(void) {
  uint8_t tmp;
  bool ret;

  if (0 == cfg.programtime) {
    return OSAL_SUCCESS;
  }
  else {
    osalThreadSleep(cfg.programtime);

#if SPI_USE_MUTUAL_EXCLUSION
    spiAcquireBus(this->spip);
#endif

    spiSelect(spip);
    spiPolledExchange(spip, CMD_25AA_RDSR);
    tmp = spiPolledExchange(spip, 0);
    spiUnselect(spip);

    if (tmp & STATUS_25AA_WIP)
      ret = OSAL_FAILED;
    else
      ret = OSAL_SUCCESS;

#if SPI_USE_MUTUAL_EXCLUSION
    spiReleaseBus(this->spip);
#endif
  }

  return ret;
}

/**
 * @brief   Accepts data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd25aa::bus_write(const uint8_t *txdata, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgCheck(this->writebuf_size >= cfg.pagesize + cfg.addr_len + 1);
  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  this->acquire();

  /* fill preamble */
  writebuf[0] = CMD_25AA_WRITE;
  addr2buf(&writebuf[1], offset, cfg.addr_len);
  status = spi_write(txdata, len, writebuf, 1+cfg.addr_len);

  this->release();

  if (MSG_OK == status)
    return len;
  else
    return 0;
}

/**
 *
 */
size_t Mtd25aa::bus_read(uint8_t *rxbuf, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
  osalDbgCheck(this->writebuf_size >= cfg.addr_len + 1);
  osalDbgCheck((nullptr != rxbuf) && (0 != len));

  this->acquire();

  /* fill preamble */
  writebuf[0] = CMD_25AA_READ;
  addr2buf(&writebuf[1], offset, cfg.addr_len);

  status = spi_read(rxbuf, len, writebuf, 1+cfg.addr_len);

  this->release();

  if (MSG_OK == status)
    return len;
  else
    return 0;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd25aa::Mtd25aa(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size, SPIDriver *spip) :
MtdBase(cfg, writebuf, writebuf_size),
spip(spip)
{
  return;
}

} /* namespace */
