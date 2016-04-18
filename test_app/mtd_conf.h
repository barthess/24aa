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

#ifndef MTD_CONF_H_
#define MTD_CONF_H_

#include "pads.h"

#define eeprom_led_on()           red_led_on()
#define eeprom_led_off()          red_led_off()

#define MTD_USE_MUTUAL_EXCLUSION  TRUE
#define MTD_WRITE_BUF_SIZE        (32 + 2)

#endif /* MTD_CONF_H_ */
