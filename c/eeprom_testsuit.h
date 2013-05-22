/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_TESTSUIT_H_
#define EEPROM_TESTSUIT_H_

#include "eeprom.h"

Thread* eepromtest_clicmd(int argc, const char * const * argv, SerialDriver *sdp);

#endif /* EEPROM_TESTSUIT_H_ */
