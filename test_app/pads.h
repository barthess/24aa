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

#ifndef PADS_H_
#define PADS_H_

static inline void nand_wp_assert(void)   {palClearPad(GPIOB, GPIOB_NAND_WP);}
static inline void nand_wp_release(void)  {palSetPad(GPIOB, GPIOB_NAND_WP);}
static inline void red_led_on(void)       {palSetPad(GPIOI, GPIOI_LED_R);}
static inline void red_led_off(void)      {palClearPad(GPIOI, GPIOI_LED_R);}
static inline void green_led_on(void)     {palSetPad(GPIOI, GPIOI_LED_G);}
static inline void green_led_off(void)    {palClearPad(GPIOI, GPIOI_LED_G);}
static inline void nvram_power_on(void)   {palClearPad(GPIOB, GPIOB_NVRAM_PWR);}
static inline void nvram_power_off(void)  {palSetPad(GPIOB, GPIOB_NVRAM_PWR);}

#endif /* PADS_H_ */
