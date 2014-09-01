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

#include <ctype.h>
#include <string.h>

#include "nvram_fs.hpp"

using namespace chibios_fs;

/*
 Структура суперблока:
 magic "EEPROM"
 uint8_t текущее количество файлов в системе
 оглавление[]
   uint8_t[8] name
   uint16_t size
   uint16_t start
 uint8_t seal контрольная сумма XOR

 файл можно создать, но нельзя удалить
 */

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
static const uint8_t magic[] = {'E','E','P','R','O','M'};
static const fileoffset_t FAT_OFFSET = sizeof(magic) + 1;  /* 1 is for storing file count */

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */
/**
 *
 */
void NvramFs::open_super(void) {
  super.mtd = &mtd;
  super.tip = 0;
  super.start = 0;
  super.size = FAT_OFFSET + sizeof(toc_item_t) * MAX_FILE_CNT + 1; /* 1 is seal byte */
}

/**
 *
 */
static uint8_t xorbuf(const uint8_t *buf, size_t len){
  uint8_t ret = 0;
  for (size_t i=0; i<len; i++)
    ret ^= buf[i];
  return ret;
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
bool NvramFs::mkfs(void) {

  uint8_t buf[sizeof(toc_item_t)];
  uint8_t checksum = xorbuf(magic, sizeof(magic));
  size_t written;

  osalDbgCheck((this->files_opened == 0) && (NULL == super.mtd));
  open_super();
  osalDbgAssert((super.start + super.size) < mtd.capacity(), "Overflow");

  memcpy(buf, magic, sizeof(magic));
  buf[sizeof(magic)] = 0; /* existing files count */
  if (FILE_OK != super.setPosition(0))
    goto FAILED;
  if (FAT_OFFSET != super.write(buf, FAT_OFFSET))
    goto FAILED;

  /* write empty FAT */
  memset(buf, 0, sizeof(buf));
  for (size_t i=0; i<MAX_FILE_CNT; i++){
    written = super.write(buf, sizeof(buf));
    checksum ^= xorbuf(buf, sizeof(buf));
    if (sizeof(buf) != written){
      goto FAILED;
    }
  }

  /* seal superblock */
  buf[0] = checksum;
  if (1 != super.write(buf, 1))
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
void NvramFs::get_magic(uint8_t *result){
  uint32_t status;

  status = super.setPosition(0);
  osalDbgCheck(FILE_OK == status);

  status = super.read(result, sizeof(magic));
  osalDbgCheck(sizeof(magic) == status);
}

/**
 *
 */
uint8_t NvramFs::get_checksum(void){
  uint8_t buf[1];
  uint32_t status;

  status = super.setPosition(FAT_OFFSET + sizeof(toc_item_t) * MAX_FILE_CNT);
  osalDbgCheck(FILE_OK == status);

  status = super.read(buf, 1);
  osalDbgCheck(1 == status);

  return buf[0];
}

/**
 * @brief   Recalculate and write checksum
 */
void NvramFs::seal(void){

  uint8_t buf[sizeof(toc_item_t)];
  uint8_t checksum;

  /* open file */
  osalDbgCheck((this->files_opened > 0) && (NULL != super.mtd));

  checksum = xorbuf(magic, sizeof(magic));
  checksum ^= get_file_cnt();

  super.setPosition(FAT_OFFSET);
  for (size_t i=0; i<MAX_FILE_CNT; i++){
    uint32_t status = super.read(buf, sizeof(buf));
    osalDbgCheck(sizeof(buf) == status);
    checksum ^= xorbuf(buf, sizeof(buf));
  }

  /* store calculated sum */
  uint32_t status = super.write(&checksum, 1);
  osalDbgCheck(1 == status);
}

/**
 *
 */
uint8_t NvramFs::get_file_cnt(void){
  uint8_t buf[1];
  uint32_t status;

  status = super.setPosition(sizeof(magic));
  osalDbgCheck(FILE_OK == status);

  status = super.read(buf, 1);
  osalDbgCheck(1 == status);

  return buf[0];
}

/**
 *
 */
void NvramFs::write_file_cnt(uint8_t cnt){

  uint32_t status;

  status = super.setPosition(sizeof(magic));
  osalDbgCheck(FILE_OK == status);

  status = super.write(&cnt, 1);
  osalDbgCheck(1 == status);
}

/**
 *
 */
void NvramFs::get_toc_item(toc_item_t *result, size_t num){
  osalDbgCheck(num < MAX_FILE_CNT);
  uint32_t status;
  const size_t blocklen = sizeof(toc_item_t);

  status = super.setPosition(FAT_OFFSET + num * blocklen);
  osalDbgCheck(FILE_OK == status);

  status = super.read((uint8_t *)result, blocklen);
  osalDbgCheck(blocklen == status);
}

/**
 *
 */
void NvramFs::write_toc_item(const toc_item_t *ti, uint8_t num){

  uint32_t status;
  const size_t blocklen = sizeof(toc_item_t);

  osalDbgCheck(num < MAX_FILE_CNT);

  status = super.setPosition(FAT_OFFSET + num * blocklen);
  osalDbgCheck(FILE_OK == status);
  status = super.write((uint8_t *)ti, blocklen);
  osalDbgCheck(blocklen == status);

  write_file_cnt(num + 1);
}

/**
 *
 */
bool NvramFs::fsck(void) {

  fileoffset_t first_empty_byte;
  uint8_t buf[sizeof(toc_item_t)];
  uint8_t checksum = xorbuf(magic, sizeof(magic));
  size_t exists;

  /* open file */
  osalDbgCheck((this->files_opened == 0) && (NULL == super.mtd));
  open_super();

  /* check magic */
  get_magic(buf);
  if (0 != memcmp(magic, buf, sizeof(magic)))
    goto FAILED;

  /* check existing files number */
  exists = get_file_cnt();
  checksum ^= exists;
  if (exists > MAX_FILE_CNT)
    goto FAILED;

  /* verify check sum */
  for (size_t i=0; i<MAX_FILE_CNT; i++){
    uint32_t status = super.read(buf, sizeof(buf));
    osalDbgCheck(sizeof(buf) == status);
    checksum ^= xorbuf(buf, sizeof(buf));
  }
  if (0 != (checksum ^ get_checksum()))
    goto FAILED;

  /* verify file names */
  first_empty_byte = super.start + super.size;
  super.setPosition(FAT_OFFSET);
  for (size_t i=0; i<exists; i++){
    toc_item_t ti;
    get_toc_item(&ti, i);

    if (OSAL_FAILED == check_name(ti.name, MAX_FILE_NAME_LEN))
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

/**
 * @brief   Delete reference to file from superblock
 */
void NvramFs::ulink(int id){
  (void)id;
  osalSysHalt("Unrealized");
}

/**
 * @brief   Consolidate free space after file deletion
 */
void NvramFs::gc(void){
  osalSysHalt("Unrealized");
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */
/**
 *
 */
NvramFs::NvramFs(Mtd &mtd) :
mtd(mtd),
files_opened(0)
{
  return;
}

/**
 *
 */
bool NvramFs::mount(void) {

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
bool NvramFs::umount(void) {

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
int NvramFs::find(const char *name, toc_item_t *ti){
  size_t i = 0;

  for (i=0; i<MAX_FILE_CNT; i++){
    get_toc_item(ti, i);
    if (0 == strcmp(name, ti->name)){
      return i;
    }
  }
  return -1;
}

/**
 *
 */
NvramFile * NvramFs::create(const char *name, chibios_fs::fileoffset_t size){
  toc_item_t ti;
  int id = -1;
  size_t file_cnt;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  /* zero size forbidden */
  if (0 == size)
    return NULL;

  file_cnt = get_file_cnt();

  /* check are we have spare slot for file */
  if (MAX_FILE_CNT == file_cnt)
    return NULL;

  id = find(name, &ti);
  if (-1 != id)
    return NULL; /* such file already exists */

  /* check for name length */
  if (strlen(name) >= MAX_FILE_NAME_LEN)
    return NULL;

  /* there is no such file. Lets create it*/
  strncpy(ti.name, name, MAX_FILE_NAME_LEN);
  ti.size = size;
  if (0 != file_cnt){
    toc_item_t tiprev;
    get_toc_item(&tiprev, file_cnt - 1);
    ti.start = tiprev.start + tiprev.size;
  }
  else
    ti.start = super.size + super.start;

  if ((ti.size + ti.start) > mtd.capacity())
    return NULL;

  /* */
  write_toc_item(&ti, file_cnt);
  seal();
  return open(name);
}

/**
 *
 */
NvramFile * NvramFs::open(const char *name){
  toc_item_t ti;
  int id = -1;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  id = find(name, &ti);

  if (-1 == id)
    return NULL; /* not found */

  if (&mtd == fat[id].mtd)
    return NULL; /* already opened */

  fat[id].size = ti.size;
  fat[id].start = ti.start;
  fat[id].mtd = &mtd;
  this->files_opened++;

  return &fat[id];
}

/**
 *
 */
void NvramFs::close(NvramFile *f) {
  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  f->close();
  this->files_opened--;
}

/**
 * @brief   Return free disk space
 */
fileoffset_t NvramFs::df(void){

  toc_item_t ti;
  size_t file_cnt;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  file_cnt = get_file_cnt();
  if (file_cnt > 0){
    get_toc_item(&ti, file_cnt - 1);
    return mtd.capacity() - (ti.size + ti.start);
  }
  else{
    return mtd.capacity() - (super.size + super.start);
  }
}

/**
 *
 */
bool NvramFs::rm(const char *name){
  toc_item_t ti;
  int id = -1;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  id = find(name, &ti);

  if (-1 == id)
    return OSAL_FAILED; /* not found */

  if (&mtd == fat[id].mtd)
    return OSAL_FAILED; /* file opened */

  osalSysHalt("Unfinished");
  ulink(id);
  gc();
  return OSAL_SUCCESS;
}

