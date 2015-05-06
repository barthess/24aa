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
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "ch.hpp"
#include "hal.h"

#include "nvram_file.hpp"

using namespace nvram;

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define TEST_BUF_LEN    (1024 * 8)

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
static uint8_t mtdbuf[TEST_BUF_LEN];
static uint8_t refbuf[TEST_BUF_LEN];
static uint8_t filebuf[TEST_BUF_LEN];

static time_measurement_t tm_mass_erase;
static time_measurement_t tm_full_write;
static time_measurement_t tm_full_read;

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */
/*
 *
 */
static void __eeprom_write_align_check(Mtd &mtd, uint8_t pattern, size_t pagenum) {
  size_t offset = mtd.pagesize() * pagenum;
  msg_t status = MSG_RESET;

  memset(refbuf, pattern, sizeof(refbuf));

  status = mtd.write(refbuf, mtd.pagesize(), offset);
  osalDbgCheck(MSG_OK == status);
  status = mtd.read(mtdbuf, mtd.pagesize(), offset);
  osalDbgCheck(MSG_OK == status);

  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, mtd.pagesize()));
}

/*
 *
 */
static void write_align_check(Mtd &mtd) {

  __eeprom_write_align_check(mtd, 0x00, 0);
  __eeprom_write_align_check(mtd, 0x55, 0);
  __eeprom_write_align_check(mtd, 0xAA, 0);
  __eeprom_write_align_check(mtd, 0xFF, 0);

  __eeprom_write_align_check(mtd, 0x00, 1);
  __eeprom_write_align_check(mtd, 0x55, 1);
  __eeprom_write_align_check(mtd, 0xAA, 1);
  __eeprom_write_align_check(mtd, 0xFF, 1);

  __eeprom_write_align_check(mtd, 0x00, mtd.pagecount() - 1);
  __eeprom_write_align_check(mtd, 0x55, mtd.pagecount() - 1);
  __eeprom_write_align_check(mtd, 0xAA, mtd.pagecount() - 1);
  __eeprom_write_align_check(mtd, 0xFF, mtd.pagecount() - 1);

  __eeprom_write_align_check(mtd, 0x00, mtd.pagecount() - 2);
  __eeprom_write_align_check(mtd, 0x55, mtd.pagecount() - 2);
  __eeprom_write_align_check(mtd, 0xAA, mtd.pagecount() - 2);
  __eeprom_write_align_check(mtd, 0xFF, mtd.pagecount() - 2);
}

/*
 *
 */
static void __eeprom_write_check(Mtd &mtd, uint8_t pattern, size_t offset, size_t len) {
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

/*
 *
 */
static void eeprom_write_misalign_check(Mtd &mtd) {
  size_t len;
  size_t offset;

  len = mtd.pagesize() + 1;
  offset = 0;
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  offset = mtd.pagesize();
  __eeprom_write_check(mtd, 0x17, offset - 2, len);
  __eeprom_write_check(mtd, 0x17, offset - 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  offset = (mtd.pagecount() - 1) * mtd.pagesize();
  //__eeprom_write_check(mtd, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_check(mtd, 0x17, offset - 1, len);
  __eeprom_write_check(mtd, 0x17, offset - 2, len);

  len = mtd.pagesize() + 2;
  offset = 0;
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  offset = mtd.pagesize();
  __eeprom_write_check(mtd, 0x17, offset - 3, len);
  __eeprom_write_check(mtd, 0x17, offset - 2, len);
  __eeprom_write_check(mtd, 0x17, offset - 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  __eeprom_write_check(mtd, 0x17, offset + 3, len);
  offset = (mtd.pagecount() - 1) * mtd.pagesize();
  //__eeprom_write_check(mtd, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  //__eeprom_write_check(mtd, 0x17, offset - 1, len);
  __eeprom_write_check(mtd, 0x17, offset - 2, len);
  __eeprom_write_check(mtd, 0x17, offset - 3, len);

  len = mtd.pagesize() * 2 + 1;
  offset = 0;
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  offset = (mtd.pagecount() - 2) * mtd.pagesize();
  //__eeprom_write_check(mtd, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_check(mtd, 0x17, offset - 1, len);
  __eeprom_write_check(mtd, 0x17, offset - 2, len);

  len = mtd.pagesize() - 1;
  offset = 0;
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  offset = (mtd.pagecount() - 1) * mtd.pagesize();
  //__eeprom_write_check(mtd, 0x17, offset + 2, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset - 1, len);

  len = mtd.pagesize() * 2 - 1;
  offset = 0;
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 2, len);
  offset = (mtd.pagecount() - 2) * mtd.pagesize();
  //__eeprom_write_check(mtd, 0x17, offset + 2, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_check(mtd, 0x17, offset + 1, len);
  __eeprom_write_check(mtd, 0x17, offset + 0, len);
  __eeprom_write_check(mtd, 0x17, offset - 1, len);
}

/*
 *
 */
static void file_test(File *eef) {

  const uint16_t u16 = 0x0102;
  const uint32_t u32 = 0x03040506;
  const uint64_t u64 = 0x0708090A0B0C0D0E;
  uint16_t ret_u16;
  uint32_t ret_u32;
  uint64_t ret_u64;

  osalDbgCheck(NULL != eef);

  eef->setPosition(0);
  osalDbgCheck(2 == eef->put(u16));
  osalDbgCheck(4 == eef->put(u32));
  osalDbgCheck(8 == eef->put(u64));

  eef->setPosition(0);
  osalDbgCheck(2 == eef->get(&ret_u16));
  osalDbgCheck(ret_u16 == u16);
  osalDbgCheck(4 == eef->get(&ret_u32));
  osalDbgCheck(ret_u32 == u32);
  osalDbgCheck(8 == eef->get(&ret_u64));
  osalDbgCheck(ret_u64 == u64);

  eef->setPosition(0);
  osalDbgCheck(8 == eef->put(u64));
  osalDbgCheck(4 == eef->put(u32));
  osalDbgCheck(2 == eef->put(u16));

  eef->setPosition(0);
  osalDbgCheck(8 == eef->get(&ret_u64));
  osalDbgCheck(ret_u64 == u64);
  osalDbgCheck(4 == eef->get(&ret_u32));
  osalDbgCheck(ret_u32 == u32);
  osalDbgCheck(2 == eef->get(&ret_u16));
  osalDbgCheck(ret_u16 == u16);
}

/*
 *
 */
static void __file_addres_translate_test(Mtd &mtd, size_t writesize) {
  const uint8_t watermark = 0xFF;
  const uint8_t databyte = 'U';
  const size_t testoffset = 125;

  memset(mtdbuf, watermark, TEST_BUF_LEN);
  mtd.write(mtdbuf, TEST_BUF_LEN, 0);

  File f;
  f.__test_ctor(&mtd, testoffset, writesize * 2);
  memset(mtdbuf, databyte, TEST_BUF_LEN);
  f.write(mtdbuf, writesize);

  /* check */
  memset(refbuf, watermark, TEST_BUF_LEN);
  memset(refbuf+testoffset, databyte, writesize);
  mtd.read(filebuf, TEST_BUF_LEN, 0);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, TEST_BUF_LEN));
}

/*
 *
 */
static void file_addres_translate_test(Mtd &mtd) {
  __file_addres_translate_test(mtd, mtd.pagesize());
  __file_addres_translate_test(mtd, mtd.pagesize() - 1);
  __file_addres_translate_test(mtd, mtd.pagesize() + 1);
}

/*
 *
 */
static void erase_test(Mtd &mtd) {
  const uint8_t watermark = 0xFF;
  msg_t status;
  size_t read_bytes;

  status = mtd.erase();
  osalDbgCheck(MSG_OK == status);

  read_bytes = mtd.read(filebuf, TEST_BUF_LEN, 0);
  osalDbgCheck(read_bytes == TEST_BUF_LEN);

  memset(refbuf, watermark, TEST_BUF_LEN);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, TEST_BUF_LEN));
}









static void fill_random(uint8_t *buf, size_t len) {
  size_t steps = len / sizeof(int);
  int *b = (int*)buf;

  osalDbgCheck(0 ==  len % sizeof(int));

  for (size_t i=0; i<steps; i++) {
    b[i] = rand();
  }
}

static void check_erased(Mtd &mtd) {
  size_t offset = 0;
  size_t write_steps = mtd.capacity() / TEST_BUF_LEN;
  uint32_t bytes;

  for (size_t i=0; i<write_steps; i++) {
    memset(mtdbuf, 0x55, TEST_BUF_LEN);
    memset(refbuf, 0xFF, TEST_BUF_LEN);

    bytes = mtd.read(mtdbuf, TEST_BUF_LEN, offset);
    offset += bytes;

    osalDbgCheck(TEST_BUF_LEN == bytes);
    osalDbgCheck(0 == memcmp(refbuf, mtdbuf, TEST_BUF_LEN));
  }
}

static void fullwrite_erase(Mtd &mtd) {
  int seed = chVTGetSystemTimeX();
  msg_t status;
  size_t write_steps = mtd.capacity() / TEST_BUF_LEN;
  uint32_t offset, bytes;

  osalDbgCheck(0 == mtd.capacity() % TEST_BUF_LEN);
  osalDbgCheck(write_steps > 0);

  chTMObjectInit(&tm_mass_erase);
  chTMObjectInit(&tm_full_write);
  chTMObjectInit(&tm_full_read);

  /* erase all */
  chTMStartMeasurementX(&tm_mass_erase);
  status = mtd.erase();
  chTMStopMeasurementX(&tm_mass_erase);
  osalDbgCheck(MSG_OK == status);
  check_erased(mtd);

  /* fill with random data */
  srand(seed);
  offset = 0;
  bytes = 0;
  chTMStartMeasurementX(&tm_full_write);
  for (size_t i=0; i<write_steps; i++) {
    fill_random(mtdbuf, TEST_BUF_LEN);
    bytes = mtd.write(mtdbuf, TEST_BUF_LEN, offset);
    osalDbgCheck(TEST_BUF_LEN == bytes);
    offset += bytes;
  }
  chTMStopMeasurementX(&tm_full_write);

  /* read back and compare */
  srand(seed);
  offset = 0;
  bytes = 0;
  memset(mtdbuf, 0x55, TEST_BUF_LEN);
  chTMStartMeasurementX(&tm_full_read);
  for (size_t i=0; i<write_steps; i++) {
    fill_random(refbuf, TEST_BUF_LEN);
    bytes = mtd.read(mtdbuf, TEST_BUF_LEN, offset);
    osalDbgCheck(TEST_BUF_LEN == bytes);
    osalDbgCheck(0 == memcmp(refbuf, mtdbuf, TEST_BUF_LEN));
    offset += bytes;
  }
  chTMStopMeasurementX(&tm_full_read);

  /* make clean */
  chTMStartMeasurementX(&tm_mass_erase);
  status = mtd.erase();
  chTMStopMeasurementX(&tm_mass_erase);
  osalDbgCheck(MSG_OK == status);
  check_erased(mtd);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 *
 */
void nvramTest(Mtd &mtd) {
  fullwrite_erase(mtd);
}



