/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#ifndef EEPROM_TESTSUIT_H_
#define EEPROM_TESTSUIT_H_

#include "eeprom.h"

Thread* eepromtest_clicmd(int argc, const char * const * argv, SerialDriver *sdp);

#endif /* EEPROM_TESTSUIT_H_ */
