/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#include <ctype.h>
#include <string.h>

#include "eeprom_fs.hpp"

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
void EepromFs::open_super(void) {
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
static bool check_name(const uint8_t *buf, size_t len) {

  int str_len = -1;

  /* check string length */
  for (size_t i=0; i<len; i++){
    if (0 == buf[i])
      str_len = i;
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
bool EepromFs::mkfs(void) {

  uint8_t buf[sizeof(toc_item_t)];
  uint8_t checksum = xorbuf(magic, sizeof(magic));
  size_t written;
  checksum ^= MAX_FILE_CNT;

  osalDbgAssert((0 == files_opened) && (NULL == super.mtd),
                        "running mkfs on mounted FS forbidden");
  open_super();
  osalDbgAssert((super.start + super.size) < mtd.capacity(), "Overflow");

  memcpy(buf, magic, sizeof(magic));
  buf[sizeof(magic)] = 0; /* exixting files count */
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
void EepromFs::get_magic(uint8_t *result){
  uint32_t status;

  status = super.setPosition(0);
  osalDbgCheck(FILE_OK == status);

  status = super.read(result, sizeof(magic));
  osalDbgCheck(sizeof(magic) == status);
}

/**
 *
 */
uint8_t EepromFs::get_checksum(void){
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
void EepromFs::seal(void){

  uint8_t buf[sizeof(toc_item_t)];
  uint8_t checksum;

  /* open file */
  osalDbgAssert((0 != files_opened) && (NULL != super.mtd), "FS must be mounted");

  checksum = xorbuf(magic, sizeof(magic));
  checksum ^= get_file_cnt();

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
uint8_t EepromFs::get_file_cnt(void){
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
void EepromFs::get_toc_item(toc_item_t *result, size_t num){
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
void EepromFs::write_toc_item(const toc_item_t *result, size_t num){
  osalDbgCheck(num < MAX_FILE_CNT);
  uint32_t status;
  const size_t blocklen = sizeof(toc_item_t);

  status = super.setPosition(FAT_OFFSET + num * blocklen);
  osalDbgCheck(FILE_OK == status);

  status = super.write((uint8_t *)result, blocklen);
  osalDbgCheck(blocklen == status);
}

/**
 *
 */
bool EepromFs::fsck(void) {

  fileoffset_t first_empty_byte;
  uint8_t buf[sizeof(toc_item_t)];
  uint8_t checksum = xorbuf(magic, sizeof(magic));
  size_t exists;

  /* open file */
  osalDbgAssert((0 == files_opened) && (NULL == super.mtd),
                        "running fsck on mounted FS forbidden");
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
void EepromFs::ulink(int id){
  (void)id;
  osalSysHalt("Unrealized");
}

/**
 * @brief   Consolidate free space after file deletion
 */
void EepromFs::gc(void){
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
EepromFs::EepromFs(Mtd &mtd) :
mtd(mtd)
{
  return;
}

/**
 *
 */
bool EepromFs::mount(void) {

  osalDbgAssert((0 == this->files_opened), "FS already mounted");

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
void EepromFs::umount(void) {

  osalDbgAssert((this->files_opened == 1), "FS has some opened files");
  super.close();
  this->files_opened = 0;
}

/**
 *
 */
int EepromFs::find(const uint8_t *name, toc_item_t *ti){
  size_t i = 0;

  for (i=0; i<MAX_FILE_CNT; i++){
    get_toc_item(ti, i);
    if (0 == strcmp((const char*)name, (const char*)ti->name)){
      return i;
    }
  }
  return -1;
}

/**
 *
 */
EepromFile * EepromFs::create(const uint8_t *name, chibios_fs::fileoffset_t size){
  toc_item_t ti;
  int id = -1;
  size_t file_cnt;

  osalDbgAssert((this->files_opened > 0), "FS not mounted");

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
  if (strlen((char *)name) < MAX_FILE_NAME_LEN)
    return NULL;

  /* there is no such file. Lets create it*/
  strncpy((char *)ti.name, (const char *)name, MAX_FILE_NAME_LEN);
  ti.size = size;
  ti.start = super.size + super.start;
  if (0 != file_cnt)
    ti.start += fat[file_cnt].start + fat[file_cnt].size;
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
EepromFile *EepromFs::open(const uint8_t *name){
  toc_item_t ti;
  int id = -1;

  osalDbgAssert((this->files_opened > 0), "FS not mounted");

  id = find(name, &ti);

  if (-1 == id)
    return NULL; /* not found */

  if (&mtd == fat[id].mtd)
    return NULL; /* already opened */

  fat[id].size = ti.size;
  fat[id].start = ti.start;
  fat[id].mtd = &mtd;

  return &fat[id];
}

/**
 * @brief   Return free disk space
 */
fileoffset_t EepromFs::df(void){

  toc_item_t ti;
  size_t file_cnt;
  size_t free;

  osalDbgAssert((this->files_opened > 0), "FS not mounted");

  free = mtd.capacity() - (super.size + super.start);
  file_cnt = get_file_cnt();
  if (file_cnt > 0){
    get_toc_item(&ti, file_cnt);
    free -= ti.size + ti.start;
  }

  return free;
}

/**
 *
 */
bool EepromFs::rm(const uint8_t *name){
  toc_item_t ti;
  int id = -1;

  osalDbgAssert((this->files_opened > 0), "FS not mounted");

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

