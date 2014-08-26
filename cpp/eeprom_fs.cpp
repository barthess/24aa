/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

#include <ctype.h>
#include "eeprom_fs.hpp"

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
static const uint8_t magic = {0x55, 0xAA};

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
void EepromFs::mkfs(void) {
//  msg_t status = MSG_RESET;
//
//  memset(file_table, 0, sizeof(file_table));
//  status = mtd.shred(0);
//  osalDbgCheck(MSG_OK == status);
//  status = mtd.write(magic, 0, sizeof(magic));
//  osalDbgCheck(MSG_OK == status);
}

/**
 *
 */
bool EepromFs::fsck(void) {

//  msg_t status = MSG_RESET;
//  size_t i = 0;
//  size_t offset = 0;
//
//  /* get whole table in ram */
//  status = mtd.read(file_table, sizeof(magic), sizeof(file_table));
//  osalDbgCheck(MSG_OK == status);
//
//  for (i=0; i<EEPROM_FS_MAX_FILES; i++) {
//    /* now check all toc items */
//    osalSysHalt("unrealized");
//    for (size_t c=0; c<sizeof(file_table[0].name); c++){
//      if (0 == isalnum(file_table[i].name[c]))
//        osalSysHalt("unrealized");
//    }
//  }
//  return OSAL_FAILED;
  return OSAL_SUCCESS;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

EepromFs::EepromFs(Mtd &mtd)
mtd(mtd),
files_opened(0),
files_total(0)
{
  return;
}

/**
 *
 */
void EepromFs::mount(void) {

  uint8_t buf[sizeof(toc_item_t)];
  msg_t status = MSG_RESET;

  osalDbgAssert((0 == this->files_opened), "FS already mounted");

  status = mtd.read(buf, 0, sizeof(magic));
  osalDbgCheck(MSG_OK == status);

  if (0 != memcmp(magic, buf, sizeof(magic)))
    mkfs();

  if (OSAL_FAILED == fsck()){
    mkfs();
    osalDbgCheck(OSAL_SUCCESS == fsck());
  }

  this->files_opened = 1;
}

/**
 * @brief     Write data to EEPROM.
 * @details   Only one EEPROM page can be written at once. So fucntion
 *            splits large data chunks in small EEPROM transactions if needed.
 * @note      To achieve the maximum effectivity use write operations
 *            aligned to EEPROM page boundaries.
 * @return    number of processed bytes.
 */
size_t EepromFs::_write(const uint8_t *buf, size_t n,
                      inodeid_t inode, fileoffset_t tip){
  msg_t status = MSG_RESET;
  size_t offset;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");
  osalDbgAssert(((tip + n) <= file_table[inode].inode.size), "Overflow error");

  offset = file_table[inode].inode.offset + tip;

  /* call low level function */
  status = mtd.write(buf, offset, n);
  if (status != MSG_OK)
    return 0;
  else
    return n;
}

/**
 * @brief     Read some bytes from current position in file.
 * @return    number of processed bytes.
 */
size_t EepromFs::_read(uint8_t *buf, size_t n, inodeid_t inode, fileoffset_t tip){
  msg_t status = MSG_RESET;
  size_t offset;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");
  osalDbgAssert(((tip + n) <= file_table[inode].inode.size), "Overflow error");

  offset = file_table[inode].inode.offset + tip;

  /* call low level function */
  status = mtd.read(buf, offset, n);
  if (status != MSG_OK)
    return 0;
  else
    return n;
}

