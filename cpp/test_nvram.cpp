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

#include <string.h>

#include "ch.hpp"
#include "hal.h"

#include "nvram_fs.hpp"

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
#include "nvram_fs.hpp"

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
static NvramFs eefs(eemtd);

#define TEST_BUF_LEN          (EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT)
static uint8_t mtdbuf[TEST_BUF_LEN];
static uint8_t refbuf[TEST_BUF_LEN];
static uint8_t filebuf[TEST_BUF_LEN];

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

static void file_test(NvramFile *eef){

  uint16_t u16 = 0x0102;
  uint32_t u32 = 0x03040506;
  uint64_t u64 = 0x0708090A0B0C0D0E;
  osalDbgCheck(NULL != eef);

  eef->setPosition(0);
  osalDbgCheck(2 == eef->putU16(u16));
  osalDbgCheck(4 == eef->putU32(u32));
  osalDbgCheck(8 == eef->putU64(u64));

  eef->setPosition(0);
  osalDbgCheck(u16 == eef->getU16());
  osalDbgCheck(u32 == eef->getU32());
  osalDbgCheck(u64 == eef->getU64());

  eef->setPosition(0);
  osalDbgCheck(8 == eef->putU64(u64));
  osalDbgCheck(4 == eef->putU32(u32));
  osalDbgCheck(2 == eef->putU16(u16));

  eef->setPosition(0);
  osalDbgCheck(u64 == eef->getU64());
  osalDbgCheck(u32 == eef->getU32());
  osalDbgCheck(u16 == eef->getU16());
}

static void __addres_translate_test(size_t writesize){
  const uint8_t watermark = 0xFF;
  const uint8_t databyte = 'U';
  const size_t testoffset = 125;

  memset(mtdbuf, watermark, TEST_BUF_LEN);
  eemtd.write(mtdbuf, TEST_BUF_LEN, 0);

  NvramFile eef;
  eef.__test_ctor(&eemtd, testoffset, writesize * 2);
  memset(mtdbuf, databyte, TEST_BUF_LEN);
  eef.write(mtdbuf, writesize);

  /* check */
  memset(refbuf, watermark, TEST_BUF_LEN);
  memset(refbuf+testoffset, databyte, writesize);
  eemtd.read(filebuf, TEST_BUF_LEN, 0);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, TEST_BUF_LEN));
}

static void addres_translate_test(void){
  __addres_translate_test(EEPROM_PAGE_SIZE);
  __addres_translate_test(EEPROM_PAGE_SIZE - 1);
  __addres_translate_test(EEPROM_PAGE_SIZE - 1);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

void testNvram(void){
  size_t df, df2;
  NvramFile *test0, *test1, *test2, *test3;

  get_size_check(eemtd);

  addres_translate_test();

  write_align_check(eemtd);
  write_misalign_check(eemtd);

  {
    NvramFile eef;
    eef.__test_ctor(&eemtd, EEPROM_PAGE_SIZE * 2, EEPROM_PAGE_SIZE);
    file_test(&eef);
  }

  eemtd.shred(0xFF);
  osalDbgCheck(OSAL_FAILED  == eefs.mount());
  osalDbgCheck(OSAL_FAILED  == eefs.fsck());
  osalDbgCheck(OSAL_SUCCESS == eefs.mkfs());
  osalDbgCheck(OSAL_SUCCESS == eefs.fsck());
  osalDbgCheck(OSAL_SUCCESS == eefs.mount());

  df = eefs.df();
  test0 = eefs.create("test0", 1024);
  osalDbgCheck(NULL != test0);
  df2 = eefs.df();
  osalDbgCheck(df2 == (df - 1024));
  memset(mtdbuf, 0x55, sizeof(mtdbuf));
  osalDbgCheck(MSG_OK == eemtd.read(mtdbuf, sizeof(mtdbuf), 0));
  file_test(test0);
  memset(mtdbuf, 0x55, sizeof(mtdbuf));
  osalDbgCheck(MSG_OK == eemtd.read(mtdbuf, sizeof(mtdbuf), 0));
  eefs.close(test0);
  test0 = eefs.open("test0");
  osalDbgCheck(NULL != test0);
  eefs.close(test0);
  test0 = eefs.create("test0", 1024);
  osalDbgCheck(NULL == test0);

  test1 = eefs.create("test1", 32);
  osalDbgCheck(NULL != test1);
  df2 = eefs.df();
  osalDbgCheck(df2 == (df - 1024 - 32));

  test2 = eefs.create("test2", 32);
  osalDbgCheck(NULL != test2);
  df2 = eefs.df();
  osalDbgCheck(df2 == (df - 1024 - 32 - 32));

  test3 = eefs.create("test3", 32);
  osalDbgCheck(NULL == test3);

  file_test(test1);
  file_test(test2);

  eefs.close(test1);
  eefs.close(test2);

  osalDbgCheck(OSAL_SUCCESS == eefs.umount());
  osalDbgCheck(OSAL_SUCCESS == eefs.fsck());

  memset(mtdbuf, 0x55, sizeof(mtdbuf));
  osalDbgCheck(MSG_OK == eemtd.read(mtdbuf, sizeof(mtdbuf), 0));
}



