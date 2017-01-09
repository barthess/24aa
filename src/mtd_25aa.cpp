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

  this->cfg.spi_select();
  spiSend(spip, preamble_len, writebuf);
  spiReceive(spip, len, rxbuf);
  this->cfg.spi_unselect();

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

  this->cfg.spi_select();
  spiPolledExchange(spip, CMD_25AA_WREN);
  this->cfg.spi_unselect();

  this->cfg.spi_select();
  spiSend(spip, preamble_len + len, writebuf);
  this->cfg.spi_unselect();

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

    this->cfg.spi_select();
    spiPolledExchange(spip, CMD_25AA_RDSR);
    tmp = spiPolledExchange(spip, 0);
    this->cfg.spi_unselect();

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
