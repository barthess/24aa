/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#ifndef EEPROM_FS_CONF_H_
#define EEPROM_FS_CONF_H_

/* Size reserved in storage for TOC. Bytes. */
#define EEPROM_FS_TOC_SIZE          256

/* Size of one record in TOC. Bytes. */
#define EEPROM_FS_TOC_RECORD_SIZE   16

/* Size of file name. Bytes. */
#define EEPROM_FS_TOC_NAME_SIZE     10

/* Maximum allowed file count in TOC */
#define EEPROM_FS_MAX_FILE_COUNT    2


#endif /* EEPROM_FS_CONF_H_ */
