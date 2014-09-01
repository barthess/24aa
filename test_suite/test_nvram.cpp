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

/* use this switch to build FRAM test instead of EEPROM */
#define USE_FRAM_TEST     FALSE

#if USE_FRAM_TEST
  #include "fram_mtd.hpp"
#else
  #include "eeprom_mtd.hpp"
#endif

#include "nvram_fs.hpp"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

#define NVRAM_I2CD                I2CD3
#if USE_FRAM_TEST
  #define NVRAM_I2C_ADDR          0b1010000   /* FRAM address on bus */
  #define EEPROM_PAGE_SIZE        32          /* fake value for testing */
  #define EEPROM_PAGE_COUNT       1024        /* fake value for testing */
  #define FRAM_SIZE               (EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT)
#else
  #define NVRAM_I2C_ADDR          0b1010001   /* EEPROM address on bus */
  #define EEPROM_PAGE_SIZE        32
  #define EEPROM_PAGE_COUNT       256
#endif
#define TEST_BUF_LEN            (EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT)

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
#if USE_FRAM_TEST
static const FramConfig fram_cfg = {
  FRAM_SIZE
};
#else
static const EepromConfig eeprom_cfg = {
  OSAL_MS2ST(5),
  EEPROM_PAGE_COUNT,
  EEPROM_PAGE_SIZE,
};
#endif

static const MtdConfig mtd_cfg = {
  &NVRAM_I2CD,
  NVRAM_I2C_ADDR,
};

#if USE_FRAM_TEST
static FramMtd nvmtd(&mtd_cfg, &fram_cfg);
#else
static EepromMtd nvmtd(&mtd_cfg, &eeprom_cfg);
#endif

static NvramFs nvfs(nvmtd);

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

static void __write_align_check(Mtd &mtd, uint8_t pattern, size_t pagenum){
  size_t offset = EEPROM_PAGE_SIZE * pagenum;
  msg_t status = MSG_RESET;

  memset(refbuf, pattern, sizeof(refbuf));

  status = mtd.write(refbuf, EEPROM_PAGE_SIZE, offset);
  osalDbgCheck(MSG_OK == status);
  status = mtd.read(mtdbuf, EEPROM_PAGE_SIZE, offset);
  osalDbgCheck(MSG_OK == status);

  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, EEPROM_PAGE_SIZE));
}

static void write_align_check(Mtd &mtd){
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

static void __write_check(Mtd &mtd, uint8_t pattern, size_t offset, size_t len){
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

static void write_misalign_check(Mtd &mtd) {
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

static void get_size_check(Mtd &mtd){
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
  nvmtd.write(mtdbuf, TEST_BUF_LEN, 0);

  NvramFile f;
  f.__test_ctor(&nvmtd, testoffset, writesize * 2);
  memset(mtdbuf, databyte, TEST_BUF_LEN);
  f.write(mtdbuf, writesize);

  /* check */
  memset(refbuf, watermark, TEST_BUF_LEN);
  memset(refbuf+testoffset, databyte, writesize);
  nvmtd.read(filebuf, TEST_BUF_LEN, 0);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, TEST_BUF_LEN));
}

static void addres_translate_test(void){
  __addres_translate_test(EEPROM_PAGE_SIZE);
  __addres_translate_test(EEPROM_PAGE_SIZE - 1);
  __addres_translate_test(EEPROM_PAGE_SIZE - 1);
}

static void shred_test(void){
  const uint8_t watermark = 0xFF;
  const uint8_t databyte = 'U';

  memset(refbuf, watermark, TEST_BUF_LEN);
  nvmtd.shred(watermark);
  nvmtd.read(filebuf, TEST_BUF_LEN, 0);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, TEST_BUF_LEN));

  memset(refbuf, databyte, TEST_BUF_LEN);
  nvmtd.shred(databyte);
  nvmtd.read(filebuf, TEST_BUF_LEN, 0);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, TEST_BUF_LEN));
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

void testNvram(void){
  size_t df, df2;
  NvramFile *test0, *test1, *test2, *test3;

  get_size_check(nvmtd);

  shred_test();

  addres_translate_test();

  write_align_check(nvmtd);
  write_misalign_check(nvmtd);

  {
    NvramFile f;
    f.__test_ctor(&nvmtd, EEPROM_PAGE_SIZE * 2, EEPROM_PAGE_SIZE);
    file_test(&f);
  }

  nvmtd.shred(0xFF);
  osalDbgCheck(OSAL_FAILED  == nvfs.mount());
  osalDbgCheck(OSAL_FAILED  == nvfs.fsck());
  osalDbgCheck(OSAL_SUCCESS == nvfs.mkfs());
  osalDbgCheck(OSAL_SUCCESS == nvfs.fsck());
  osalDbgCheck(OSAL_SUCCESS == nvfs.mount());

  df = nvfs.df();
  test0 = nvfs.create("test0", 1024);
  osalDbgCheck(NULL != test0);
  df2 = nvfs.df();
  osalDbgCheck(df2 == (df - 1024));
  memset(mtdbuf, 0x55, sizeof(mtdbuf));
  osalDbgCheck(MSG_OK == nvmtd.read(mtdbuf, sizeof(mtdbuf), 0));
  file_test(test0);
  memset(mtdbuf, 0x55, sizeof(mtdbuf));
  osalDbgCheck(MSG_OK == nvmtd.read(mtdbuf, sizeof(mtdbuf), 0));
  nvfs.close(test0);
  test0 = nvfs.open("test0");
  osalDbgCheck(NULL != test0);
  nvfs.close(test0);
  test0 = nvfs.create("test0", 1024);
  osalDbgCheck(NULL == test0);

  test1 = nvfs.create("test1", 32);
  osalDbgCheck(NULL != test1);
  df2 = nvfs.df();
  osalDbgCheck(df2 == (df - 1024 - 32));

  test2 = nvfs.create("test2", 32);
  osalDbgCheck(NULL != test2);
  df2 = nvfs.df();
  osalDbgCheck(df2 == (df - 1024 - 32 - 32));

  test3 = nvfs.create("test3", 32);
  osalDbgCheck(NULL == test3);

  file_test(test1);
  file_test(test2);

  nvfs.close(test1);
  nvfs.close(test2);

  osalDbgCheck(OSAL_SUCCESS == nvfs.umount());
  osalDbgCheck(OSAL_SUCCESS == nvfs.fsck());

  /*  now just read all data from NVRAM for eye check */
  memset(mtdbuf, 0x55, sizeof(mtdbuf));
  osalDbgCheck(MSG_OK == nvmtd.read(mtdbuf, sizeof(mtdbuf), 0));
}



