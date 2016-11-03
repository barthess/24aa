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

#ifndef MTD_CONF_H_
#define MTD_CONF_H_

#include "pads.h"

#define eeprom_led_on()           red_led_on()
#define eeprom_led_off()          red_led_off()

#define MTD_USE_MUTUAL_EXCLUSION  TRUE
#define MTD_WRITE_BUF_SIZE        (32 + 2)

#endif /* MTD_CONF_H_ */
