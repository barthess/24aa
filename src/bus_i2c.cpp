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
#include <cstdlib>

#include "ch.hpp"

#include "bus_i2c.hpp"

namespace nvram {

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#if defined(SAM7_PLATFORM)
#define EEPROM_I2C_CLOCK (MCK / (((this->i2cp->config->cwgr & 0xFF) + ((this->cfg->i2cp->config->cwgr >> 8) & 0xFF)) * (1 << ((this->cfg->i2cp->config->cwgr >> 16) & 7)) + 6))
#else
#define EEPROM_I2C_CLOCK (this->i2cp->config->clock_speed)
#endif

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
 * @brief     Calculates requred timeout.
 */
systime_t BusI2C::calc_timeout(size_t bytes) {
  const uint32_t bitsinbyte = 10;
  uint32_t tmo;
  tmo = ((bytes + 1) * bitsinbyte * 1000);
  tmo /= EEPROM_I2C_CLOCK;
  tmo += 10; /* some additional milliseconds to be safer */
  return MS2ST(tmo);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
BusI2C::BusI2C(I2CDriver *i2cp, i2caddr_t addr) :
i2cp(i2cp),
addr(addr),
i2cflags(I2C_NO_ERROR)
{
  return;
}

/**
 *
 */
msg_t BusI2C::exchange(const uint8_t *txbuf, size_t txbytes,
                             uint8_t *rxbuf, size_t rxbytes) {

#if defined(STM32F1XX_I2C)
#error "Ugly workaround for single byte reading is not implemented yet"
  if (1 == len)
    return stm32_f1x_read_single_byte(data, offset);
#endif /* defined(STM32F1XX_I2C) */

  msg_t status;
  systime_t tmo = calc_timeout(txbytes + rxbytes);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cAcquireBus(this->i2cp);
#endif

  status = i2cMasterTransmitTimeout(this->i2cp, this->addr,
                                    txbuf, txbytes, rxbuf, rxbytes, tmo);
  if (MSG_OK != status)
    i2cflags = i2cGetErrors(this->i2cp);

#if I2C_USE_MUTUAL_EXCLUSION
  i2cReleaseBus(this->i2cp);
#endif

  return status;
}

/*
 * Ugly workaround.
 * Stupid I2C cell in STM32F1x does not allow to read single byte.
 * So we must read 2 bytes and return needed one.
 */
msg_t BusI2C::stm32_f1x_read_single_byte(const uint8_t *txbuf, size_t txbytes,
                                         uint8_t *rxbuf, systime_t tmo) {
  uint8_t tmp[2];
  msg_t status;

  status = i2cMasterTransmitTimeout(this->i2cp, this->addr,
                                    txbuf, txbytes, tmp, 2, tmo);
  rxbuf[0] = tmp[0];
  return status;
}

} /* namespace */
