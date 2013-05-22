/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_MTD_CONF_H_
#define EEPROM_MTD_CONF_H_

#define EEPROM_PAGE_SIZE        128    /* page size in bytes. Consult datasheet. */
#define EEPROM_SIZE             65536  /* total amount of memory in bytes */
#define EEPROM_WRITE_TIME_MS    20     /* time to write one page in mS. Consult datasheet! */

#define eeprom_led_on()         {palClearPad(GPIOB, GPIOB_LED_R);}
#define eeprom_led_off()        {palSetPad(GPIOB, GPIOB_LED_R);}

#endif /* EEPROM_MTD_CONF_H_ */
