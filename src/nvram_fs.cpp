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

#include <cctype>
#include <cstring>

#include "nvram_fs.hpp"

using namespace chibios_fs;

namespace nvram {

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

/**
 *
 */
static const uint8_t magic[] = {'2','4','a','a'};

/**
 *
 */
static const fileoffset_t FAT_OFFSET = sizeof(magic) + sizeof(filecount_t);

/**
 *
 */
static const size_t SUPERBLOCK_SIZE =
    FAT_OFFSET
    + sizeof(toc_item_t) * NVRAM_FS_MAX_FILE_CNT
    + sizeof(checksum_t);

static uint8_t dbg_super[SUPERBLOCK_SIZE];

/*
  Name  : CRC-8
  Poly  : 0x31    x^8 + x^5 + x^4 + 1
  Init  : 0xFF
  Revert: false
  XorOut: 0x00
  Check : 0xF7 ("123456789")
*/
static const uint8_t Crc8Table[256] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
    0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
    0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
    0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52,
    0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
    0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
    0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
    0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
    0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
    0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
    0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
    0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
    0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
    0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
    0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
    0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
    0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
    0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
    0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
    0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
    0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
    0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
    0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
    0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
    0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
    0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
    0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
    0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
};

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */

/**
 * param[in] crc    initialization vector. For the first data block it
 *                  is generally 0xFF.  For subsequent blocks it is result
 *                  of calculation of previous block(s).
 */
static uint8_t nvramcrc(const uint8_t *buf, size_t len, uint8_t crc) {
  while (len--)
    crc = Crc8Table[crc ^ *buf++];
  return crc;
}

/**
 *
 */
void Fs::open_super(void) {
  super.mtd = &mtd;
  super.tip = 0;
  super.start = 0;
  super.size = SUPERBLOCK_SIZE;
}

/**
 *
 */
static bool check_name(const char *buf, size_t len) {
  int str_len = -1;

  /* check string length */
  for (size_t i=0; i<len; i++){
    if (0 == buf[i]){
      str_len = i;
      break;
    }
  }
  if (-1 == str_len)
    return OSAL_FAILED;

  /* check symbol set in string */
  for (int i=0; i<str_len; i++){
    if (0 == isalnum(buf[i]))
      return OSAL_FAILED;
  }

  return OSAL_SUCCESS;
}

/**
 *
 */
bool Fs::is_mounted(void) {
  return (0 != this->files_opened);
}

/**
 *
 */
bool Fs::mkfs(void) {
  checksum_t sum = 0xFF; /* initial CRC vector */
  size_t written;

  osalDbgCheck((this->files_opened == 0) && (nullptr == super.mtd));
  open_super();
  osalDbgAssert((super.start + super.size) < mtd.capacity(), "Overflow");

  /* write magic and zero file count */
  memcpy(toc_buf, magic, sizeof(magic));
  toc_buf[sizeof(magic)] = 0; /* existing files count */
  if (FILE_OK != super.setPosition(0))
    goto FAILED;
  if (FAT_OFFSET != super.write(toc_buf, FAT_OFFSET))
    goto FAILED;
  sum = nvramcrc(toc_buf, FAT_OFFSET, sum);

  /* write empty FAT */
  memset(toc_buf, 0, sizeof(toc_buf));
  for (size_t i=0; i<NVRAM_FS_MAX_FILE_CNT; i++){
    written = super.write(toc_buf, sizeof(toc_buf));
    if (sizeof(toc_buf) != written)
      goto FAILED;
    sum = nvramcrc(toc_buf, sizeof(toc_buf), sum);
  }

  /* seal superblock */
  memcpy(toc_buf, &sum, sizeof(checksum_t));
  if (sizeof(checksum_t) != super.write(toc_buf, sizeof(checksum_t)))
    goto FAILED;

  super.close();
  return OSAL_SUCCESS;

FAILED:
  super.close();
  return OSAL_FAILED;
}

/**
 *
 */
void Fs::get_magic(uint8_t *result){
  size_t status;

  status = super.setPosition(0);
  osalDbgCheck(FILE_OK == status);

  status = super.read(result, sizeof(magic));
  osalDbgCheck(sizeof(magic) == status);
}

/**
 *
 */
checksum_t Fs::get_checksum(void){
  checksum_t ret;
  size_t status;

  status = super.setPosition(SUPERBLOCK_SIZE - sizeof(checksum_t));
  osalDbgCheck(FILE_OK == status);

  status = super.read(&ret, sizeof(checksum_t));
  osalDbgCheck(sizeof(checksum_t)== status);

  return ret;
}

/**
 * @brief   Recalculate and write checksum
 */
void Fs::seal(void){
  uint8_t sum = 0xFF;
  size_t status;

  osalDbgCheck((this->files_opened > 0) && (nullptr != super.mtd));

  super.setPosition(0);
  status = super.read(toc_buf, FAT_OFFSET);
  osalDbgCheck(FAT_OFFSET == status);
  sum = nvramcrc(toc_buf, FAT_OFFSET, sum);

  for (size_t i=0; i<NVRAM_FS_MAX_FILE_CNT; i++){
    status = super.read(toc_buf, sizeof(toc_buf));
    osalDbgCheck(sizeof(toc_buf) == status);
    sum = nvramcrc(toc_buf, sizeof(toc_buf), sum);
  }

  /* store calculated sum */
  status = super.write(&sum, sizeof(checksum_t));
  osalDbgCheck(sizeof(checksum_t) == status);
}

/**
 *
 */
filecount_t Fs::get_file_cnt(void){
  filecount_t cnt;
  size_t status;

  status = super.setPosition(sizeof(magic));
  osalDbgCheck(FILE_OK == status);

  status = super.read(&cnt, sizeof(filecount_t));
  osalDbgCheck(sizeof(filecount_t) == status);

  return cnt;
}

/**
 *
 */
void Fs::write_file_cnt(filecount_t N){
  size_t status;

  status = super.setPosition(sizeof(magic));
  osalDbgCheck(FILE_OK == status);

  status = super.write(&N, 1);
  osalDbgCheck(1 == status);
}

/**
 *
 */
void Fs::read_toc_item(toc_item_t *result, size_t N){
  size_t status;
  const size_t blocklen = sizeof(toc_item_t);

  osalDbgCheck(N < NVRAM_FS_MAX_FILE_CNT);

  status = super.setPosition(FAT_OFFSET + N * blocklen);
  osalDbgCheck(FILE_OK == status);

  status = super.read((uint8_t *)result, blocklen);
  osalDbgCheck(blocklen == status);
}

/**
 *
 */
void Fs::write_toc_item(const toc_item_t *ti, size_t N){
  size_t status;
  const size_t blocklen = sizeof(toc_item_t);

  osalDbgCheck(N < NVRAM_FS_MAX_FILE_CNT);

  status = super.setPosition(FAT_OFFSET + N * blocklen);
  osalDbgCheck(FILE_OK == status);
  status = super.write((uint8_t *)ti, blocklen);
  osalDbgCheck(blocklen == status);

  write_file_cnt(N + 1);
}

/**
 *
 */
bool Fs::fsck(void) {
  fileoffset_t first_empty_byte;
  uint8_t sum = 0xFF;
  filecount_t exists;
  size_t status;

  mtd.read(dbg_super, sizeof(dbg_super), 0);

  /* open superblock */
  osalDbgCheck((this->files_opened == 0) && (nullptr == super.mtd));
  open_super();

  /* check magic */
  get_magic(toc_buf);
  sum = nvramcrc(toc_buf, sizeof(magic), sum);
  if (0 != memcmp(magic, toc_buf, sizeof(magic)))
    goto FAILED;

  /* check existing files number */
  exists = get_file_cnt();
  sum = nvramcrc(&exists, sizeof(exists), sum);
  if (exists > NVRAM_FS_MAX_FILE_CNT)
    goto FAILED;

  /* verify check sum */
  for (size_t i=0; i<NVRAM_FS_MAX_FILE_CNT; i++){
    status = super.read(toc_buf, sizeof(toc_buf));
    osalDbgCheck(sizeof(toc_buf) == status);
    sum = nvramcrc(toc_buf, sizeof(toc_buf), sum);
  }
  if (sum != get_checksum())
    goto FAILED;

  /* verify file names */
  first_empty_byte = super.start + super.size;
  super.setPosition(FAT_OFFSET);
  for (size_t i=0; i<exists; i++){
    toc_item_t ti;
    read_toc_item(&ti, i);

    if (OSAL_FAILED == check_name(ti.name, NVRAM_FS_MAX_FILE_NAME_LEN))
      goto FAILED;
    if ((ti.start + ti.size) >= mtd.capacity())
      goto FAILED;
    if (ti.start < first_empty_byte)
      goto FAILED;
  }

  super.close();
  return OSAL_SUCCESS;

FAILED:
  super.close();
  return OSAL_FAILED;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */
/**
 *
 */
Fs::Fs(MtdBase &mtd) :
mtd(mtd),
files_opened(0)
{
  return;
}

/**
 *
 */
bool Fs::mount(void) {

  /* already mounted */
  if (this->files_opened > 1)
    return OSAL_SUCCESS;

  /* check file table correctness*/
  if (OSAL_FAILED == fsck())
    goto FAILED;

  open_super();

  this->files_opened = 1;
  return OSAL_SUCCESS;

FAILED:
  this->files_opened = 0;
  super.close();
  return OSAL_FAILED;
}

/**
 *
 */
bool Fs::umount(void) {

  /* FS has some opened files */
  if (this->files_opened > 1)
    return OSAL_FAILED;
  else{
    super.close();
    this->files_opened = 0;
    return OSAL_SUCCESS;
  }
}

/**
 *
 */
int Fs::find(const char *name, toc_item_t *ti){
  size_t i = 0;

  for (i=0; i<NVRAM_FS_MAX_FILE_CNT; i++){
    read_toc_item(ti, i);
    if (0 == strncmp(name, ti->name, NVRAM_FS_MAX_FILE_NAME_LEN)){
      return i;
    }
  }
  return -1;
}

/**
 *
 */
File* Fs::create(const char *name, uint32_t size){
  toc_item_t ti;
  int id = -1;
  size_t file_cnt;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  /* zero size forbidden */
  if (0 == size)
    return nullptr;

  file_cnt = get_file_cnt();

  /* check are we have spare slot for file */
  if (NVRAM_FS_MAX_FILE_CNT == file_cnt)
    return nullptr;

  id = find(name, &ti);
  if (-1 != id)
    return nullptr; /* such file already exists */

  /* check for name length */
  if (strlen(name) >= NVRAM_FS_MAX_FILE_NAME_LEN)
    return nullptr;

  /* there is no such file. Lets create it*/
  strncpy(ti.name, name, NVRAM_FS_MAX_FILE_NAME_LEN);
  ti.size = size;
  if (0 != file_cnt){
    toc_item_t tiprev;
    read_toc_item(&tiprev, file_cnt - 1);
    ti.start = tiprev.start + tiprev.size;
  }
  else
    ti.start = super.size + super.start;

  if ((ti.size + ti.start) > mtd.capacity())
    return nullptr;

  /* */
  write_toc_item(&ti, file_cnt);
  seal();
  return open(name);
}

/**
 *
 */
File* Fs::open(const char *name){
  toc_item_t ti;
  int id = -1;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  id = find(name, &ti);

  if (-1 == id)
    return nullptr; /* not found */

  if (&mtd == fat[id].mtd)
    return nullptr; /* already opened */

  fat[id].size = ti.size;
  fat[id].start = ti.start;
  fat[id].mtd = &mtd;
  this->files_opened++;

  return &fat[id];
}

/**
 *
 */
void Fs::close(File *file) {

  osalDbgAssert(file != nullptr, "null pointer reference");
  if (!file)
    return;
  osalDbgAssert(this->files_opened > 0, "FS not mounted");
  /* there is no opened files. return.*/
  if (this->files_opened <= 1)
    return;
  /* return if the file is already closed.*/
  if (!file->mtd)
    return;
  file->close();
  this->files_opened--;
}

/**
 * @brief   Return free disk space
 */
uint32_t Fs::df(void) {
  toc_item_t ti;
  size_t file_cnt;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  file_cnt = get_file_cnt();
  if (file_cnt > 0) {
    read_toc_item(&ti, file_cnt - 1);
    return mtd.capacity() - (ti.size + ti.start);
  }
  else {
    return mtd.capacity() - (super.size + super.start);
  }
}

} /* namespace */






