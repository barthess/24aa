/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012 Uladzimir Pylinski aka barthess

    Licensed under the 3-Clause BSD license, (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://directory.fsf.org/wiki/License:BSD_3Clause
*/

/*****************************************************************************
 * DATASHEET NOTES
 *****************************************************************************
Write cycle time (byte or page) — 5 ms

Note:
Page write operations are limited to writing
bytes within a single physical page,
regardless of the number of bytes
actually being written. Physical page
boundaries start at addresses that are
integer multiples of the page buffer size (or
‘page size’) and end at addresses that are
integer multiples of [page size – 1]. If a
Page Write command attempts to write
across a physical page boundary, the
result is that the data wraps around to the
beginning of the current page (overwriting
data previously stored there), instead of
being written to the next page as might be
expected.
*********************************************************************/

#include <string.h>

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
void EepromFs::toc2buf(const toc_record_t *toc, uint8_t *b){
  uint32_t n = 0;

  memset(b, 0, EEPROM_FS_TOC_RECORD_SIZE);
  strncpy((char*)b, toc->name, EEPROM_FS_TOC_NAME_SIZE);

  n = EEPROM_FS_TOC_NAME_SIZE;
  b[n++] = (toc->inode.startpage >> 8)  & 0xFF;
  b[n++] =  toc->inode.startpage & 0xFF;
  b[n++] = (toc->inode.pageoffset >> 8) & 0xFF;
  b[n++] =  toc->inode.pageoffset & 0xFF;
  b[n++] = (toc->inode.size >> 8) & 0xFF;
  b[n++] =  toc->inode.size & 0xFF;
}

/**
 *
 */
void EepromFs::buf2toc(const uint8_t *b, toc_record_t *toc){
  uint16_t tmp;
  uint32_t n;

  strncpy(toc->name, (const char*)b, EEPROM_FS_TOC_NAME_SIZE);

  n = EEPROM_FS_TOC_NAME_SIZE;
  tmp =  b[n++] << 8;
  tmp |= b[n++];
  toc->inode.startpage = tmp;
  tmp =  b[n++] << 8;
  tmp |= b[n++];
  toc->inode.pageoffset = tmp;
  tmp =  b[n++] << 8;
  tmp |= b[n++];
  toc->inode.size = tmp;
}

/**
 *
 */
size_t EepromFs::inode2absoffset(const inode_t *inode, fileoffset_t tip){
  size_t absoffset;
  absoffset =  inode->startpage * mtd->getPageSize();
  absoffset += inode->pageoffset + tip;
  return absoffset;
}

/**
 * @brief   Write data that can be fitted in single page boundary
 */
void EepromFs::fitted_write(const uint8_t *data, size_t absoffset,
                            size_t len, uint32_t *written){
  msg_t status = RDY_RESET;

  osalDbgAssert(len != 0, "something broken in hi level part");

  status = mtd->write(data, absoffset, len);
  if (status == RDY_OK){
    *written += len;
  }
}

/**
 *
 */
void EepromFs::mkfs(void){
  uint8_t bp[EEPROM_FS_TOC_RECORD_SIZE] = {};
  uint32_t i = 0;
  uint32_t n = 0;
  size_t absoffset = 0;
  msg_t status = RDY_RESET;

  /* erase whole IC */
  this->mtd->massErase();

  /* write all data in single byte transactions to workaround page organized problem */
  for (i=0; i<EEPROM_FS_MAX_FILE_COUNT; i++){
    toc2buf(&(reftoc[i]), bp);
    for (n=0; n<sizeof(bp); n++){
      status = mtd->write(&(bp[n]), absoffset, 1);
      absoffset++;
      osalDbgAssert(RDY_OK == status, "MTD broken");
    }
  }
}

/**
 *
 */
void EepromFs::read_toc_record(uint32_t N, toc_record_t *rec){
  msg_t status = RDY_RESET;
  uint8_t bp[EEPROM_FS_TOC_RECORD_SIZE] = {};
  size_t absoffset;

  absoffset = EEPROM_FS_TOC_RECORD_SIZE * N;
  status = mtd->read(bp, absoffset, EEPROM_FS_TOC_RECORD_SIZE);
  osalDbgAssert(RDY_OK == status, "MTD broken");

  buf2toc(bp, rec);
}

/**
 *
 */
bool EepromFs::fsck(void){
  int32_t i = 0, n = 0;
  char namebuf[EEPROM_FS_TOC_NAME_SIZE];
  toc_record_t rec;
  rec.name = namebuf;

  /* check name collisions in reference TOC */
  for (n=0; n<EEPROM_FS_MAX_FILE_COUNT; n++){
    for (i=n+1; i<EEPROM_FS_MAX_FILE_COUNT; i++){
      if (0 == strcmp(reftoc[n].name, reftoc[i].name))
        osalSysHalt("name collision in reference TOC");
    }
  }

  /* check name lengths */
  for (n=0; n<EEPROM_FS_MAX_FILE_COUNT; n++){
    if (EEPROM_FS_TOC_NAME_SIZE < strlen(reftoc[n].name))
      osalSysHalt("name too long");
  }

  /* check correspondace of names in reference TOC and last actual TOC in EEPROM */
  for (i=0; i<EEPROM_FS_MAX_FILE_COUNT; i++){
    read_toc_record(i, &rec);
    if (0 != strcmp(reftoc[i].name, rec.name))
      return CH_FAILED;
    if (reftoc[i].inode.pageoffset != rec.inode.pageoffset)
      return CH_FAILED;
    if (reftoc[i].inode.size != rec.inode.size)
      return CH_FAILED;
    if (reftoc[i].inode.startpage != rec.inode.startpage)
      return CH_FAILED;
  }
  return CH_SUCCESS;
}

/**
 *
 */
bool EepromFs::overlap(const inode_t *inode0, const inode_t *inode1){
  size_t absoffset0 = inode2absoffset(inode0, 0);
  size_t absoffset1 = inode2absoffset(inode1, 0);

  if ((absoffset0 + inode0->size) > absoffset1)
    return true;
  else
    return false;
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

EepromFs::EepromFs(EepromMtd *mtd, const toc_record_t *reftoc, size_t N){
  osalDbgCheck(((NULL != mtd) && (NULL != reftoc) && (0 != N)));
  osalDbgCheck(EEPROM_FS_MAX_FILE_COUNT >= N);

  this->files_opened = 0;
  this->mtd = mtd;
  this->reftoc = reftoc;

  /* check inode overlapping in reference TOC */
  uint32_t i = 0;
  inode_t inode_toc; /* inode storing TOC */
  inode_toc.startpage = 0;
  inode_toc.pageoffset = 0;
  inode_toc.size = EEPROM_FS_TOC_SIZE;

  const inode_t *inode0 = &inode_toc;
  const inode_t *inode1 = &(reftoc[0].inode);

  for (i=1; i<N; i++){
    if (overlap(inode0, inode1))
      osalSysHalt("Inodes can not ovelap");
    inode0 = inode1;
    inode1 = &(reftoc[i].inode);
  }
}

/**
 *
 */
void EepromFs::mount(void){
  uint32_t i = 0;
  char namebuf[EEPROM_FS_TOC_NAME_SIZE];
  toc_record_t rec;
  rec.name = namebuf;

  osalDbgAssert((0 == this->files_opened), "FS already mounted");

  if (CH_FAILED == this->fsck()){
    this->mkfs();
    if (CH_FAILED == this->fsck())
      osalSysHalt("mkfs created FS with errors");
  }

  for (i=0; i<EEPROM_FS_MAX_FILE_COUNT; i++){
    read_toc_record(i, &rec);
    inode_table[i].size       = rec.inode.size;
    inode_table[i].pageoffset = rec.inode.pageoffset;
    inode_table[i].startpage  = rec.inode.startpage;
  }

  this->files_opened = 1;
}

/**
 * Use it with care
 */
bool EepromFs::umount(void){
  if (0 == this->files_opened){ // not mounted yet
    return CH_SUCCESS;
  }
  else if (1 == this->files_opened){
    chSysLock();
    this->files_opened = 0;
    chSysUnlock();
    return CH_SUCCESS;
  }
  else{
    osalSysHalt("FS has opened files");
    return CH_FAILED; //warning supressor
  }
}

/**
 *
 */
size_t EepromFs::getSize(inodeid_t inodeid){
  osalDbgAssert(this->files_opened > 0, "Not mounted");
  osalDbgAssert(EEPROM_FS_MAX_FILE_COUNT > inodeid, "Overflow error");
  return this->inode_table[inodeid].size;
}

/**
 *
 */
inodeid_t EepromFs::open(const uint8_t *name){
  uint32_t i = 0;
  char namebuf[EEPROM_FS_TOC_NAME_SIZE];
  toc_record_t rec;
  rec.name = namebuf;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");

  for (i=0; i<EEPROM_FS_MAX_FILE_COUNT; i++){
    read_toc_record(i, &rec);
    if (0 == strcmp((const char*)name, rec.name)){
      chSysLock();
      this->files_opened++;
      chSysUnlock();
      return i;
    }
  }

  return -1; // nothing was found
}

/**
 *
 */
bool EepromFs::close(inodeid_t inodeid){
  (void)inodeid;
  chSysLock();
  this->files_opened--;
  chSysUnlock();
  return CH_SUCCESS;
}

/**
 * @brief     Write data to EEPROM.
 * @details   Only one EEPROM page can be written at once. So fucntion
 *            splits large data chunks in small EEPROM transactions if needed.
 * @note      To achieve the maximum effectivity use write operations
 *            aligned to EEPROM page boundaries.
 * @return    number of processed bytes.
 */
size_t EepromFs::write(const uint8_t *bp, size_t n,
                      inodeid_t inodeid, fileoffset_t tip){

  size_t   len = 0;   /* bytes to be written at one trasaction */
  uint32_t written;   /* total bytes successfully written */
  uint16_t pagesize;
  uint32_t firstpage; /* first page to be affected during transaction */
  uint32_t lastpage;  /* last page to be affected during transaction */
  size_t   absoffset; /* absoulute offset in bytes relative to eeprom start */

  osalDbgAssert(this->files_opened > 0, "FS not mounted");
  osalDbgAssert(((tip + n) <= inode_table[inodeid].size), "Overflow error");

  absoffset = inode2absoffset(&(inode_table[inodeid]), tip);

  pagesize  =  mtd->getPageSize();
  firstpage =  absoffset / pagesize;
  lastpage  =  (absoffset + n - 1) / pagesize;

  written = 0;
  /* data can be fitted in single page */
  if (firstpage == lastpage){
    len = n;
    fitted_write(bp, absoffset, len, &written);
    bp += len;
    absoffset += len;
    return written;
  }

  else{
    /* write first piece of data to first page boundary */
    len =  ((firstpage + 1) * pagesize) - tip;
    len -= inode_table[inodeid].startpage * mtd->getPageSize() + inode_table[inodeid].pageoffset;
    fitted_write(bp, absoffset, len, &written);
    bp += len;
    absoffset += len;

    /* now writes blocks at a size of pages (may be no one) */
    while ((n - written) > pagesize){
      len = pagesize;
      fitted_write(bp, absoffset, len, &written);
      bp += len;
      absoffset += len;
    }

    /* wrtie tail */
    len = n - written;
    if (0 == len)
      return written;
    else{
      fitted_write(bp, absoffset, len, &written);
    }
  }

  return written;
}

/**
 * @brief     Read some bytes from current position in file.
 * @return    number of processed bytes.
 */
size_t EepromFs::read(uint8_t *bp, size_t n, inodeid_t inodeid, fileoffset_t tip){
  msg_t status = RDY_OK;
  size_t absoffset;

  osalDbgAssert(this->files_opened > 0, "FS not mounted");
  osalDbgAssert(((tip + n) <= inode_table[inodeid].size), "Overflow error");

  absoffset = inode2absoffset(&(inode_table[inodeid]), tip);

  /* call low level function */
  status = mtd->read(bp, absoffset, n);
  if (status != RDY_OK)
    return 0;
  else
    return n;
}

