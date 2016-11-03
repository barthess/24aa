/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

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
#include "chprintf.h"

#include "nvram_file.hpp"
#include "nvram_fs.hpp"
#include "nvram_test.hpp"

using namespace nvram;

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

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
static void dbgprint(TestContext *ctx, const char *msg) {
  if (nullptr != ctx->chn) {
    chprintf(ctx->chn, msg);
  }
}

/*
 *
 */
static void __eeprom_write_misalign_check(TestContext *ctx, uint8_t pattern,
                                          size_t offset, size_t len) {
  size_t status = MSG_RESET;
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  uint8_t *refbuf = ctx->refbuf;
  uint8_t *filebuf = ctx->filebuf;

  memset(refbuf, ~pattern, ctx->len);
  memset(mtdbuf, ~pattern, ctx->len);
  memset(filebuf, pattern, len);

  /* write watermark */
  status = mtd->write(refbuf, ctx->len, 0);
  osalDbgCheck(ctx->len == status);

  /* write data */
  status = mtd->write(filebuf, len, offset);
  osalDbgCheck(len == status);

  /* read back */
  status = mtd->read(mtdbuf, ctx->len, 0);
  osalDbgCheck(ctx->len == status);

  /* prepare reference buffer */
  memset(&refbuf[offset], pattern, len);

  /* compare */
  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, ctx->len));
}

/*
 *
 */
static void eeprom_write_misalign_check(TestContext *ctx) {

  const size_t pagesize = ctx->mtd->pagesize();
  const size_t pagecnt = ctx->mtd->pagecount();
  size_t offset = 0;
  size_t len = ctx->mtd->pagesize() + 1;

  dbgprint(ctx, "write misalign test ... ");

  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  offset = pagesize;
  __eeprom_write_misalign_check(ctx, 0x17, offset - 2, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  offset = (pagecnt - 1) * pagesize;
  //__eeprom_write_misalign_check(ctx, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 2, len);

  len = pagesize + 2;
  offset = 0;
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  offset = pagesize;
  __eeprom_write_misalign_check(ctx, 0x17, offset - 3, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 2, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 3, len);
  offset = (pagecnt - 1) * pagesize;
  //__eeprom_write_misalign_check(ctx, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  //__eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 2, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 3, len);

  len = pagesize * 2 + 1;
  offset = 0;
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  offset = (pagecnt - 2) * pagesize;
  //__eeprom_write_misalign_check(ctx, 0x17, offset - 0, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 2, len);

  len = pagesize - 1;
  offset = 0;
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  offset = (pagecnt - 1) * pagesize;
  //__eeprom_write_misalign_check(ctx, 0x17, offset + 2, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);

  len = pagesize * 2 - 1;
  offset = 0;
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 2, len);
  offset = (pagecnt - 2) * pagesize;
  //__eeprom_write_misalign_check(ctx, 0x17, offset + 2, len); /* here mtd MUST crash because of overflow */
  __eeprom_write_misalign_check(ctx, 0x17, offset + 1, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset + 0, len);
  __eeprom_write_misalign_check(ctx, 0x17, offset - 1, len);

  dbgprint(ctx, "OK\r\n");
}

/*
 *
 */
template <typename T>
static size_t _get(File *eef, T *ret) {
  return eef->read(reinterpret_cast<uint8_t *>(ret), sizeof(T));
}

/*
 *
 */
template <typename T>
static size_t _put(File *eef, const T data) {
  return eef->write(reinterpret_cast<const uint8_t *>(&data), sizeof(T));
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
  osalDbgCheck(2 == _put(eef, u16));
  osalDbgCheck(4 == _put(eef, u32));
  osalDbgCheck(8 == _put(eef, u64));

  eef->setPosition(0);
  osalDbgCheck(2 == _get(eef, &ret_u16));
  osalDbgCheck(ret_u16 == u16);
  osalDbgCheck(4 == _get(eef, &ret_u32));
  osalDbgCheck(ret_u32 == u32);
  osalDbgCheck(8 == _get(eef, &ret_u64));
  osalDbgCheck(ret_u64 == u64);

  eef->setPosition(0);
  osalDbgCheck(8 == _put(eef, u64));
  osalDbgCheck(4 == _put(eef, u32));
  osalDbgCheck(2 == _put(eef, u16));

  eef->setPosition(0);
  osalDbgCheck(8 == _get(eef, &ret_u64));
  osalDbgCheck(ret_u64 == u64);
  osalDbgCheck(4 == _get(eef, &ret_u32));
  osalDbgCheck(ret_u32 == u32);
  osalDbgCheck(2 == _get(eef, &ret_u16));
  osalDbgCheck(ret_u16 == u16);
}

/*
 *
 */
static void __file_addres_translate_test(TestContext *ctx, size_t writesize) {
  const uint8_t watermark = 0xFF;
  const uint8_t databyte = 'U';
  const size_t testoffset = 125;
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  uint8_t *refbuf = ctx->refbuf;
  uint8_t *filebuf = ctx->filebuf;

  memset(mtdbuf, watermark, ctx->len);
  mtd->write(mtdbuf, ctx->len, 0);

  File f;
  f.__test_ctor(mtd, testoffset, writesize * 2);
  memset(mtdbuf, databyte, ctx->len);
  f.write(mtdbuf, writesize);

  /* check */
  memset(refbuf, watermark, ctx->len);
  memset(refbuf + testoffset, databyte, writesize);
  mtd->read(filebuf, ctx->len, 0);
  osalDbgCheck(0 == memcmp(refbuf, filebuf, ctx->len));
}

/*
 *
 */
static void addres_translate_test(TestContext *ctx) {
  uint32_t writesize;

  dbgprint(ctx, "address translate test ... ");

  if (ctx->mtd->pagecount() > 1)
    writesize = ctx->mtd->pagesize();
  else
    writesize = 64;

  __file_addres_translate_test(ctx, writesize);
  __file_addres_translate_test(ctx, writesize - 1);
  __file_addres_translate_test(ctx, writesize + 1);

  dbgprint(ctx, "OK\r\n");
}

/*
 *
 */
static void __eeprom_write_align_check(TestContext *ctx, uint8_t pattern, size_t pagenum) {

  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  uint8_t *refbuf = ctx->refbuf;
  const size_t offset = mtd->pagesize() * pagenum;
  const size_t N = mtd->pagesize();
  size_t status;

  memset(refbuf, pattern, ctx->len);

  status = mtd->write(refbuf, N, offset);
  osalDbgCheck(N == status);

  memset(mtdbuf, ~pattern, ctx->len);
  status = mtd->read(mtdbuf, N, offset);
  osalDbgCheck(N == status);

  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, N));
}

/*
 *
 */
static void __eeprom_write_align_overlap_check(TestContext *ctx,
                      uint8_t pattern1, uint8_t pattern2, size_t firstpage) {
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  uint8_t *refbuf = ctx->refbuf;

  const size_t offset = mtd->pagesize() * firstpage;
  const size_t N = mtd->pagesize();
  size_t status;

  memset(refbuf, pattern1, N);
  memset(refbuf + N, pattern2, N);

  status = mtd->read(mtdbuf, 2*N, offset);
  osalDbgCheck(2*N == status);

  osalDbgCheck(0 == memcmp(refbuf, mtdbuf, N));
}

/*
 *
 */
static void eeprom_write_align_check(nvram::TestContext *ctx) {
  MtdBase *mtd = ctx->mtd;
  dbgprint(ctx, "write align test ... ");

  __eeprom_write_align_check(ctx, 0xAA, 0);
  __eeprom_write_align_check(ctx, 0x00, 0);
  __eeprom_write_align_check(ctx, 0x55, 0);
  __eeprom_write_align_check(ctx, 0xFF, 0);
  __eeprom_write_align_check(ctx, 0x00, 1);
  __eeprom_write_align_check(ctx, 0x55, 1);
  __eeprom_write_align_check(ctx, 0xAA, 1);
  __eeprom_write_align_check(ctx, 0xFF, 1);

  __eeprom_write_align_check(ctx, 0x00, 0);
  __eeprom_write_align_check(ctx, 0x55, 1);
  __eeprom_write_align_overlap_check(ctx, 0x00, 0x55, 0);
  __eeprom_write_align_check(ctx, 0xAA, 1);
  __eeprom_write_align_check(ctx, 0x55, 0);
  __eeprom_write_align_overlap_check(ctx, 0x55, 0xAA, 0);

  __eeprom_write_align_check(ctx, 0x00, mtd->pagecount() - 1);
  __eeprom_write_align_check(ctx, 0x55, mtd->pagecount() - 1);
  __eeprom_write_align_check(ctx, 0xAA, mtd->pagecount() - 1);
  __eeprom_write_align_check(ctx, 0xFF, mtd->pagecount() - 1);
  __eeprom_write_align_check(ctx, 0x00, mtd->pagecount() - 2);
  __eeprom_write_align_check(ctx, 0x55, mtd->pagecount() - 2);
  __eeprom_write_align_check(ctx, 0xAA, mtd->pagecount() - 2);
  __eeprom_write_align_check(ctx, 0xFF, mtd->pagecount() - 2);

  __eeprom_write_align_check(ctx, 0x55, mtd->pagecount() - 2);
  __eeprom_write_align_check(ctx, 0x00, mtd->pagecount() - 1);
  __eeprom_write_align_overlap_check(ctx, 0x55, 0x00, mtd->pagecount() - 2);
  __eeprom_write_align_check(ctx, 0xAA, mtd->pagecount() - 2);
  __eeprom_write_align_check(ctx, 0x55, mtd->pagecount() - 1);
  __eeprom_write_align_overlap_check(ctx, 0xAA, 0x55, mtd->pagecount() - 2);

  dbgprint(ctx, "OK\r\n");
}

/*
 *
 */
static void fill_random(uint8_t *buf, size_t len) {
  size_t steps = len / sizeof(int);
  int *b = (int*)buf;

  osalDbgCheck(0 ==  len % sizeof(int));

  for (size_t i=0; i<steps; i++) {
    b[i] = rand();
  }
}

/*
 *
 */
static void check_erased(nvram::TestContext *ctx) {
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  uint8_t *refbuf = ctx->refbuf;
  size_t offset = 0;
  size_t write_steps = mtd->capacity() / ctx->len;
  uint32_t bytes;

  for (size_t i=0; i<write_steps; i++) {
    memset(mtdbuf, 0x55, ctx->len);
    memset(refbuf, 0xFF, ctx->len);

    bytes = mtd->read(mtdbuf, ctx->len, offset);
    offset += bytes;

    osalDbgCheck(ctx->len == bytes);
    osalDbgCheck(0 == memcmp(refbuf, mtdbuf, ctx->len));
  }
}

/*
 *
 */
static msg_t nvramset(nvram::TestContext *ctx, int pattern) {

  size_t psize;
  size_t pages;

  if (! ctx->mtd->is_fram()) {
    psize = ctx->mtd->pagesize();
    pages = ctx->mtd->pagecount();
  }
  else {
    psize = 32;
    pages = ctx->mtd->pagesize() / psize;
  }

  memset(ctx->mtdbuf, pattern, psize);

  for (size_t i=0; i<pages; i++) {
    if (psize != ctx->mtd->write(ctx->mtdbuf, psize, i*psize)) {
      return MSG_RESET;
    }
  }

  return MSG_OK;
}

/*
 *
 */
static void full_write_erase(nvram::TestContext *ctx) {
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  uint8_t *refbuf = ctx->refbuf;
  int seed = chVTGetSystemTimeX();
  msg_t status;
  size_t write_steps = mtd->capacity() / ctx->len;
  uint32_t offset, bytes;

  dbgprint(ctx, "full write test ... ");

  osalDbgCheck(0 == mtd->capacity() % ctx->len);
  osalDbgCheck(write_steps > 0);

  /* fill with random data */
  srand(seed);
  offset = 0;
  bytes = 0;
  for (size_t i=0; i<write_steps; i++) {
    fill_random(mtdbuf, ctx->len);
    bytes = mtd->write(mtdbuf, ctx->len, offset);
    osalDbgCheck(ctx->len == bytes);
    offset += bytes;
  }

  /* read back and compare */
  srand(seed);
  offset = 0;
  bytes = 0;
  memset(mtdbuf, 0x55, ctx->len);
  for (size_t i=0; i<write_steps; i++) {
    fill_random(refbuf, ctx->len);
    bytes = mtd->read(mtdbuf, ctx->len, offset);
    osalDbgCheck(ctx->len == bytes);
    osalDbgCheck(0 == memcmp(refbuf, mtdbuf, ctx->len));
    offset += bytes;
  }

  /* make clean */
  status = nvramset(ctx, 0xFF);
  osalDbgCheck(MSG_OK == status);
  check_erased(ctx);

  dbgprint(ctx, "OK\r\n");
}

/*
 *
 */
static void file_put_test(nvram::TestContext *ctx) {

  nvram::File f;

  dbgprint(ctx, "file put test ... ");

  f.__test_ctor(ctx->mtd, 0, ctx->mtd->capacity());
  file_test(&f);

  dbgprint(ctx, "OK\r\n");
}

/*
 *
 */
void mkfs_and_mount_test(nvram::TestContext *ctx) {

  MtdBase *mtd = ctx->mtd;
  Fs nvfs(*mtd);

  dbgprint(ctx, "mkfs and mount test ... ");
  nvramset(ctx, 0xFF);
  osalDbgCheck(OSAL_FAILED  == nvfs.mount());
  osalDbgCheck(OSAL_FAILED  == nvfs.fsck());
  osalDbgCheck(OSAL_SUCCESS == nvfs.mkfs());
  osalDbgCheck(OSAL_SUCCESS == nvfs.fsck());
  osalDbgCheck(OSAL_SUCCESS == nvfs.mount());
  dbgprint(ctx, "OK\r\n");
}

/*
 *
 */
static void file_creation_test(nvram::TestContext *ctx) {

  size_t df, df2;
  size_t status;
  File *test0, *test1, *test2, *test3;
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  Fs nvfs(*mtd);

  dbgprint(ctx, "file creation test ... ");

  nvfs.mount();

  df = nvfs.df();
  test0 = nvfs.create("test0", 1024);
  osalDbgCheck(NULL != test0);
  df2 = nvfs.df();
  osalDbgCheck(df2 == (df - 1024));
  memset(mtdbuf, 0x55, ctx->len);
  status = mtd->read(mtdbuf, ctx->len, 0);
  osalDbgCheck(ctx->len == status);
  file_test(test0);
  memset(mtdbuf, 0x55, ctx->len);
  status = mtd->read(mtdbuf, ctx->len, 0);
  osalDbgCheck(ctx->len == status);
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

  dbgprint(ctx, "OK\r\n");
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/*
 *
 */
bool nvram::TestSuite(TestContext *ctx) {

  /* This test strictly depends on checks provided by ChibiOS. It is pointless
     to run it without enabled debug checks. */
#if CH_DBG_ENABLE_CHECKS != TRUE
  return OSAL_FAILED;
#endif

  osalDbgCheck((nullptr != ctx->mtd) && (nullptr != ctx->mtdbuf)
            && (nullptr != ctx->filebuf) && (nullptr != ctx->refbuf)
            && (0 != ctx->len));

  size_t status;
  MtdBase *mtd = ctx->mtd;
  uint8_t *mtdbuf = ctx->mtdbuf;
  Fs nvfs(*mtd);

  full_write_erase(ctx);
  if (mtd->pagecount() > 1) {
    eeprom_write_align_check(ctx);
    eeprom_write_misalign_check(ctx);
  }
  addres_translate_test(ctx);
  file_put_test(ctx);
  mkfs_and_mount_test(ctx);
  file_creation_test(ctx);

  dbgprint(ctx, "umount and fsck ... ");
  osalDbgCheck(OSAL_SUCCESS == nvfs.umount());
  osalDbgCheck(OSAL_SUCCESS == nvfs.fsck());
  dbgprint(ctx, "OK\r\n");

  /*  now just read all data from NVRAM for eye check */
  memset(mtdbuf, 0x55, ctx->len);
  status = mtd->read(mtdbuf, ctx->len, 0);
  osalDbgCheck(ctx->len == status);

  /* make clean */
  status = nvramset(ctx, 0xFF);
  osalDbgCheck(MSG_OK == status);
  return OSAL_SUCCESS;
}






