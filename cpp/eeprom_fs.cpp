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
static void __close(EepromFile *f) {
  f->mtd = NULL;
  f->tip = 0;
  f->start = 0;
  f->size = 0;
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

  __close(&super);
  return OSAL_SUCCESS;

FAILED:
  __close(&super);
  return OSAL_FAILED;
}

/**
 *
 */
void EepromFs::get_magic(uint8_t *result){
  osalDbgCheck(FILE_OK == super.setPosition(0));
  osalDbgCheck(sizeof(magic) == super.read(result, sizeof(magic)));
}

/**
 *
 */
uint8_t EepromFs::get_checksum(void){
  uint8_t buf[1];
  osalDbgCheck(FILE_OK == super.setPosition(FAT_OFFSET + sizeof(toc_item_t) * MAX_FILE_CNT));
  osalDbgCheck(1 == super.read(buf, 1));
  return buf[0];
}

/**
 *
 */
uint8_t EepromFs::get_file_cnt(void){
  uint8_t buf[1];
  osalDbgCheck(FILE_OK == super.setPosition(sizeof(magic)));
  osalDbgCheck(1 == super.read(buf, 1));
  return buf[0];
}

/**
 *
 */
void EepromFs::get_toc_item(toc_item_t *result, size_t num){
  osalDbgCheck(num < MAX_FILE_CNT);

  osalDbgCheck(FILE_OK == super.setPosition(FAT_OFFSET + num * sizeof(toc_item_t)));
  osalDbgCheck(MAX_FILE_NAME_LEN == super.read(result->name, MAX_FILE_NAME_LEN));
  result->start = super.getU16();
  result->size = super.getU16();
}

/**
 *
 */
void EepromFs::write_toc_item(const toc_item_t *result, size_t num){
  osalDbgCheck(num < MAX_FILE_CNT);

  osalDbgCheck(FILE_OK == super.setPosition(FAT_OFFSET + num * sizeof(toc_item_t)));
  osalDbgCheck(MAX_FILE_NAME_LEN == super.write(result->name, MAX_FILE_NAME_LEN));
  osalDbgCheck(2 == super.putU16(result->start));
  osalDbgCheck(2 == super.putU16(result->size));
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
    osalDbgCheck(sizeof(buf) == super.read(buf, sizeof(buf)));
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

  __close(&super);
  return OSAL_SUCCESS;

FAILED:
  __close(&super);
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
  __close(&super);
  return OSAL_FAILED;
}

/**
 *
 */
void EepromFs::umount(void) {

  osalDbgAssert((this->files_opened == 1), "FS has some opened files");
  __close(&super);
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

  osalDbgAssert((this->files_opened > 0), "FS not mounted");

  if (-1 == find(name, &ti)){
    #error "write me!"
    srcpy(ti.name, name);
    ti.size = size;
  }
  else{
    return NULL;
  }
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
 *
 */
void EepromFs::close(EepromFile *f) {
  osalDbgAssert((this->files_opened > 0), "FS not mounted");
  __close(f);
}

/**
 * @brief   Return free disk space
 */
fileoffset_t EepromFs::df(void){
  osalSysHalt("unrealized");
  return 0;
}
