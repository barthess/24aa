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

#include "mtd25.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define     S25_CMD_READ    0x03
#define     S25_CMD_4READ   0x13
#define     S25_CMD_PP      0x02  // page program
#define     S25_CMD_4PP     0x12
#define     S25_CMD_BE      0x60  // bulk erase
#define     S25_CMD_RDSR1   0x05  // Read Status Register-1
#define     S25_CMD_RDSR2   0x07  // Read Status Register-2
#define     S25_CMD_RDCR    0x35  // Read Configuration Register-1
#define     S25_CMD_RDID    0x9F  // Read ID
#define     S25_CMD_WRR     0x01  // Write Register (Status-1, Configuration-1)
#define     S25_CMD_WRDI    0x04  // Write Disable
#define     S25_CMD_WREN    0x06  // Write Enable
#define     S25_CMD_CLSR    0x30  // Clear Status Register-1 - Erase/Prog. Fail Reset

#define     S25_SR1_WEL     0b00000010  // write enable latch (1 == write enable)
#define     S25_SR1_PERR    0b01000000  // program error (0 - ok, 1 - error)
#define     S25_SR1_EERR    0b00100000  // erase error (0 - ok, 1 - error)
#define     S25_SR1_WIP     0b00000001  // write in progress

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
msg_t Mtd25::spi_read(uint8_t *rxbuf, size_t len,
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
msg_t Mtd25::spi_write_enable(void) {
  msg_t ret = MSG_RESET;
  uint8_t tmp;

#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(this->spip);
#endif

  /* try to enable write access */
  spiSelect(spip);
  spiPolledExchange(spip, S25_CMD_WREN);
  spiUnselect(spip);
  chSysPolledDelayX(10);

  /* check result */
  spiSelect(spip);
  spiPolledExchange(spip, S25_CMD_RDSR1);
  tmp = spiPolledExchange(spip, 0);
  spiUnselect(spip);
  if (tmp & S25_SR1_WEL)
    ret = MSG_OK;
  else
    ret = MSG_RESET;

#if SPI_USE_MUTUAL_EXCLUSION
  spiReleaseBus(this->spip);
#endif

  return ret;
}

/**
 *
 */
msg_t Mtd25::spi_write(const uint8_t *txdata, size_t len,
                       uint8_t *writebuf, size_t preamble_len) {

  memcpy(&writebuf[preamble_len], txdata, len);

  if (MSG_OK != spi_write_enable())
    return MSG_RESET;

#if SPI_USE_MUTUAL_EXCLUSION
  spiAcquireBus(this->spip);
#endif

  spiSelect(spip);
  spiSend(spip, preamble_len + len, writebuf);
  spiUnselect(spip);

#if SPI_USE_MUTUAL_EXCLUSION
  spiReleaseBus(this->spip);
#endif

  return wait_op_complete(cfg.programtime);
}

/**
 *
 */
msg_t Mtd25::wait_op_complete(systime_t timeout) {

  systime_t start = chVTGetSystemTimeX();
  systime_t end = start + timeout;

  osalThreadSleepMilliseconds(2);

  while (chVTIsSystemTimeWithinX(start, end)) {
    uint8_t tmp;

#if SPI_USE_MUTUAL_EXCLUSION
    spiAcquireBus(this->spip);
#endif

    spiSelect(spip);
    spiPolledExchange(spip, S25_CMD_RDSR1);
    tmp = spiPolledExchange(spip, 0);
    spiUnselect(spip);

#if SPI_USE_MUTUAL_EXCLUSION
    spiReleaseBus(this->spip);
#endif

    if (tmp & S25_SR1_WIP) {
      continue;
    }
    else {
      if (tmp & (S25_SR1_PERR | S25_SR1_EERR))
        return MSG_RESET;
      else
        return MSG_OK;
    }

    osalThreadSleepMilliseconds(10);
  }

  /* time is out */
  return MSG_RESET;
}

/**
 * @brief   Accepts data that can be fitted in single page boundary (for EEPROM)
 *          or can be placed in write buffer (for FRAM)
 */
size_t Mtd25::bus_write(const uint8_t *txdata, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgCheck(this->writebuf_size >= cfg.pagesize + cfg.addr_len + 1);
  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");

  this->acquire();

  /* fill preamble */
  if (4 == cfg.addr_len)
    writebuf[0] = S25_CMD_4PP;
  else
    writebuf[0] = S25_CMD_PP;
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
size_t Mtd25::bus_read(uint8_t *rxbuf, size_t len, uint32_t offset) {
  msg_t status;

  osalDbgAssert((offset + len) <= capacity(), "Transaction out of device bounds");
  osalDbgCheck(this->writebuf_size >= cfg.addr_len + 1);

  this->acquire();

  /* fill preamble */
  if (4 == cfg.addr_len)
    writebuf[0] = S25_CMD_4READ;
  else
    writebuf[0] = S25_CMD_READ;
  addr2buf(&writebuf[1], offset, cfg.addr_len);

  status = spi_read(rxbuf, len, writebuf, 1+cfg.addr_len);

  this->release();

  if (MSG_OK == status)
    return len;
  else
    return 0;
}

/**
 *
 */
msg_t Mtd25::bus_erase(void) {
  msg_t ret;

  this->acquire();

  writebuf[0] = S25_CMD_BE;
  spi_write(nullptr, 0, writebuf, 1);
  ret = wait_op_complete(cfg.erasetime);

  this->release();

  return ret;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
Mtd25::Mtd25(const MtdConfig &cfg, uint8_t *writebuf, size_t writebuf_size, SPIDriver *spip) :
Mtd(cfg, writebuf, writebuf_size),
spip(spip)
{
  return;
}

} /* namespace */
