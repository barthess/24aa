/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#include <string.h>

#include "ch.hpp"
#include "hal.h"

#include "eeprom_fs.hpp"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

#define EEPROM_I2CD             I2CD1
#define EEPROM_I2C_ADDR         0b1010000   /* EEPROM address on bus */
#define EEPROM_PAGE_SIZE        32
#define EEPROM_PAGE_COUNT       128

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

#include "eeprom_mtd.hpp"
#include "eeprom_fs.hpp"

static const EepromConfig eeprom_cfg = {
  OSAL_MS2ST(5),
  EEPROM_PAGE_COUNT,
  EEPROM_PAGE_SIZE,
};

static const MtdConfig mtd_cfg = {
  &EEPROM_I2CD,
  EEPROM_I2C_ADDR,
};

static EepromMtd eemtd(&mtd_cfg, &eeprom_cfg);
static EepromFs eefs(eemtd);

static uint8_t mtdbuf[EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT];
static uint8_t refbuf[EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT];
static uint8_t filebuf[EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT];

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */

static void __write_align_check(EepromMtd &mtd, uint8_t pattern, size_t pagenum){
  size_t offset = EEPROM_PAGE_SIZE * pagenum;
  msg_t status = MSG_RESET;

  memset(refbuf, pattern, sizeof(refbuf));

  status = mtd.write(refbuf, EEPROM_PAGE_SIZE, offset);
  osalDbgCheck(MSG_OK == status);
  status = mtd.read(mtdbuf, EEPROM_PAGE_SIZE, offset);
  osalDbgCheck(MSG_OK == status);

  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, EEPROM_PAGE_SIZE));
}

static void write_align_check(EepromMtd &mtd){
  __write_align_check(mtd, 0x00, 0);
  __write_align_check(mtd, 0x55, 0);
  __write_align_check(mtd, 0xAA, 0);
  __write_align_check(mtd, 0xFF, 0);

  __write_align_check(mtd, 0x00, 1);
  __write_align_check(mtd, 0x55, 1);
  __write_align_check(mtd, 0xAA, 1);
  __write_align_check(mtd, 0xFF, 1);

  __write_align_check(mtd, 0x00, EEPROM_PAGE_COUNT - 1);
  __write_align_check(mtd, 0x55, EEPROM_PAGE_COUNT - 1);
  __write_align_check(mtd, 0xAA, EEPROM_PAGE_COUNT - 1);
  __write_align_check(mtd, 0xFF, EEPROM_PAGE_COUNT - 1);

  __write_align_check(mtd, 0x00, EEPROM_PAGE_COUNT - 2);
  __write_align_check(mtd, 0x55, EEPROM_PAGE_COUNT - 2);
  __write_align_check(mtd, 0xAA, EEPROM_PAGE_COUNT - 2);
  __write_align_check(mtd, 0xFF, EEPROM_PAGE_COUNT - 2);
}

static void __write_check(EepromMtd &mtd, uint8_t pattern, size_t offset, size_t len){
  msg_t status = MSG_RESET;

  memset(refbuf, ~pattern, sizeof(refbuf));
  memset(mtdbuf, ~pattern, sizeof(mtdbuf));
  memset(filebuf, pattern, len);

  /* write watermark */
  status = mtd.write(refbuf, sizeof(refbuf), 0);
  osalDbgCheck(MSG_OK == status);
  /* write data */
  status = mtd.write(filebuf, len, offset);
  osalDbgCheck(MSG_OK == status);
  /* read back */
  status = mtd.read(mtdbuf, sizeof(mtdbuf), 0);
  osalDbgCheck(MSG_OK == status);
  /* prepare reference buffer */
  memset(&refbuf[offset], pattern, len);
  /* compare */
  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, sizeof(mtdbuf)));
}

static void write_misalign_check(EepromMtd &mtd) {
  size_t len;
  size_t offset;

  len = EEPROM_PAGE_SIZE + 1;
  offset = 0;
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  offset = EEPROM_PAGE_SIZE;
  __write_check(mtd, 0x17, offset - 2, len);
  __write_check(mtd, 0x17, offset - 1, len);
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  offset = (EEPROM_PAGE_COUNT - 1) * EEPROM_PAGE_SIZE;
  //__write_check(mtd, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  __write_check(mtd, 0x17, offset - 1, len);
  __write_check(mtd, 0x17, offset - 2, len);

  len = EEPROM_PAGE_SIZE + 2;
  offset = 0;
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  offset = EEPROM_PAGE_SIZE;
  __write_check(mtd, 0x17, offset - 3, len);
  __write_check(mtd, 0x17, offset - 2, len);
  __write_check(mtd, 0x17, offset - 1, len);
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  __write_check(mtd, 0x17, offset + 3, len);
  offset = (EEPROM_PAGE_COUNT - 1) * EEPROM_PAGE_SIZE;
  //__write_check(mtd, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  //__write_check(mtd, 0x17, offset - 1, len);
  __write_check(mtd, 0x17, offset - 2, len);
  __write_check(mtd, 0x17, offset - 3, len);

  len = EEPROM_PAGE_SIZE * 2 + 1;
  offset = 0;
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  offset = (EEPROM_PAGE_COUNT - 2) * EEPROM_PAGE_SIZE;
  //__write_check(mtd, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  __write_check(mtd, 0x17, offset - 1, len);
  __write_check(mtd, 0x17, offset - 2, len);

  len = EEPROM_PAGE_SIZE - 1;
  offset = 0;
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  offset = (EEPROM_PAGE_COUNT - 1) * EEPROM_PAGE_SIZE;
  //__write_check(mtd, 0x17, offset + 2, len); /* here mtd MUST crash because of overflow */
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset - 1, len);

  len = EEPROM_PAGE_SIZE * 2 - 1;
  offset = 0;
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 2, len);
  offset = (EEPROM_PAGE_COUNT - 2) * EEPROM_PAGE_SIZE;
  //__write_check(mtd, 0x17, offset + 2, len); /* here mtd MUST crash because of overflow */
  __write_check(mtd, 0x17, offset + 1, len);
  __write_check(mtd, 0x17, offset + 0, len);
  __write_check(mtd, 0x17, offset - 1, len);
}

static void get_size_check(EepromMtd &mtd){
  osalDbgCheck((EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) == mtd.capacity());
}

static void file_test(void){
  EepromFile eef;
  eef.__test_ctor(&eemtd, EEPROM_PAGE_SIZE * 2, EEPROM_PAGE_SIZE);

  uint16_t u16 = 0x0102;
  uint32_t u32 = 0x03040506;
  uint64_t u64 = 0x0708090A0B0C0D0E;

  eef.setPosition(0);
  osalDbgCheck(2 == eef.putU16(u16));
  osalDbgCheck(4 == eef.putU32(u32));
  osalDbgCheck(8 == eef.putU64(u64));

  eef.setPosition(0);
  osalDbgCheck(u16 == eef.getU16());
  osalDbgCheck(u32 == eef.getU32());
  osalDbgCheck(u64 == eef.getU64());

  eef.setPosition(0);
  osalDbgCheck(8 == eef.putU64(u64));
  osalDbgCheck(4 == eef.putU32(u32));
  osalDbgCheck(2 == eef.putU16(u16));

  eef.setPosition(0);
  osalDbgCheck(u64 == eef.getU64());
  osalDbgCheck(u32 == eef.getU32());
  osalDbgCheck(u16 == eef.getU16());
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

void testEepromMtd(void){
  get_size_check(eemtd);

  file_test();

  write_align_check(eemtd);
  osalDbgCheck(OSAL_FAILED  == eefs.mount());
  osalDbgCheck(OSAL_FAILED  == eefs.fsck());
  osalDbgCheck(OSAL_SUCCESS == eefs.mkfs());
  osalDbgCheck(OSAL_SUCCESS == eefs.fsck());
  osalDbgCheck(OSAL_SUCCESS == eefs.mount());

  write_misalign_check(eemtd);
}



