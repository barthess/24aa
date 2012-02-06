/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#include "ch.h"
#include "hal.h"

#include "24aa.h"
#include "eeprom.h"

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 */

static fileoffset_t getsize(void *ip){
  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");
  return EEPROM_SIZE;
}

static fileoffset_t getposition(void *ip){
  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");
  return ((EepromFileStream*)ip)->position;
}

static fileoffset_t lseek(void *ip, fileoffset_t offset){
  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");
  if (offset > EEPROM_SIZE)
    offset = EEPROM_SIZE;
  ((EepromFileStream*)ip)->position = offset;
  return FILE_OK;
}

static uint32_t close(void *ip) {
  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");
  ((EepromFileStream*)ip)->errors = FILE_OK;
  ((EepromFileStream*)ip)->position = 0;
  ((EepromFileStream*)ip)->vmt = NULL;
  return FILE_OK;
}

static int geterror(void *ip){
  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");
  return ((EepromFileStream*)ip)->errors;
}

/**
 * @brief   Determines and returns size of data that can be processed
 */
static size_t __clamp_size(void *ip, size_t n){
  if (n > getsize(ip))
    return 0;
  else if ((getposition(ip) + n) > getsize(ip))
    return getsize(ip) - getposition(ip);
  else
    return n;
}

/**
 * @brief     Macro helper.
 */
#define __fitted_write(){                                                     \
  status  = eeprom_write(getposition(ip), bp, len);                           \
  if (status != RDY_OK)                                                       \
    return written;                                                           \
  else{                                                                       \
    written += len;                                                           \
    bp += len;                                                                \
    lseek(ip, getposition(ip) + len);                                         \
  }                                                                           \
}

/**
 * @brief     Write data to EEPROM.
 * @details   Only one EEPROM page can be written at once. So fucntion
 *            splits large data chunks in small EEPROM transactions if needed.
 * @note      To achieve the maximum effectivity use write operations
 *            aligned to EEPROM page boundaries.
 */
static size_t write(void *ip, const uint8_t *bp, size_t n){
  msg_t status = RDY_OK;

  size_t   len = 0;     /* bytes to be written at one trasaction */
  uint32_t written = 0; /* total bytes successfully written */
  uint32_t firstpage = getposition(ip) / EEPROM_PAGE_SIZE;
  uint32_t lastpage  = (getposition(ip) + n - 1) / EEPROM_PAGE_SIZE;

  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");

  if (n == 0)
    return 0;

  n = __clamp_size(ip, n);
  if (n == 0)
    return 0;

  /* data fitted in single page */
  if (firstpage == lastpage){
    len = n;
    __fitted_write();
    return written;
  }

  else{
    /* write first piece of data to first page boundary */
    len = ((firstpage + 1) * EEPROM_PAGE_SIZE) - getposition(ip);
    __fitted_write();

    /* now writes blocks at a size of pages (may be no one) */
    while ((n - written) > EEPROM_PAGE_SIZE){
      len = EEPROM_PAGE_SIZE;
      __fitted_write();
    }

    /* wrtie tail */
    len = n - written;
    if (len == 0)
      return written;
    else{
      __fitted_write();
    }
  }
  return written;
}

/**
 * Read some bytes from current position in file. After successful
 * read operation the position pointer will be increased by the number
 * of read bytes.
 */
static size_t read(void *ip, uint8_t *bp, size_t n){
  msg_t status = RDY_OK;

  chDbgCheck((ip != NULL) && (((EepromFileStream*)ip)->vmt != NULL), "");

  if (n == 0)
    return 0;

  n = __clamp_size(ip, n);
  if (n == 0)
    return 0;

  /* Stupid STM32 I2C cell does not allow to read less than 2 bytes.
     So we must read 2 bytes and return needed one. */
#if (defined(STM32F4XX) || defined(STM32F2XX) || defined(STM32F1XX) || \
                                                 defined(STM32L1XX))
  if (n == 1){
    uint8_t __buf[2];
    /* if NOT last byte of file requested */
    if ((getposition(ip) + 1) < getsize(ip)){
      if (read(ip, __buf, 2) == 2){
        lseek(ip, (getposition(ip) + 1));
        bp[0] = __buf[0];
        return 1;
      }
      else
        return 0;
    }
    else{
      lseek(ip, (getposition(ip) - 1));
      if (read(ip, __buf, 2) == 2){
        lseek(ip, (getposition(ip) + 2));
        bp[0] = __buf[1];
        return 1;
      }
      else
        return 0;
    }
  }
#endif

  /* call low level function */
  status  = eeprom_read(getposition(ip), bp, n);
  if (status != RDY_OK)
    return 0;
  else{
    lseek(ip, (getposition(ip) + n));
    return n;
  }
}

static const struct EepromFilelStreamVMT vmt = {
    write,
    read,
    close,
    geterror,
    getsize,
    getposition,
    lseek,
};

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

/**
 * Open EEPROM IC as file and return pointer to the file object
 * @note      Fucntion allways successfully open file. All checking makes
 *            in read/write functions.
 */
EepromFileStream* EepromOpen(EepromFileStream* efs){
  init_eepromio();
  chDbgCheck(efs->vmt != &vmt, "file allready opened");
  efs->vmt = &vmt;
  efs->errors = FILE_OK;
  efs->position = 0;
  return efs;
}

uint8_t EepromReadByte(EepromFileStream *EepromFile_p){
  uint8_t buf[1];
  size_t status = chFileStreamRead(EepromFile_p, buf, sizeof(buf));
  chDbgAssert(status == sizeof(buf), "EepromReadByte(), #1", "read failed");
  return buf[0];
}

uint16_t EepromReadHalfword(EepromFileStream *EepromFile_p){
  uint8_t buf[2];
  size_t status = chFileStreamRead(EepromFile_p, buf, sizeof(buf));
  chDbgAssert(status == sizeof(buf), "EepromReadByte(), #1", "read failed");
  return (buf[0] << 8) | buf[1];
}

uint32_t EepromReadWord(EepromFileStream *EepromFile_p){
  uint8_t buf[4];
  size_t status = chFileStreamRead(EepromFile_p, buf, sizeof(buf));
  chDbgAssert(status == sizeof(buf), "EepromReadByte(), #1", "read failed");
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

size_t EepromWriteByte(EepromFileStream *EepromFile_p, uint8_t data){
  uint8_t buf[1] = {data};
  return chFileStreamWrite(EepromFile_p, buf, sizeof(buf));
}

size_t EepromWriteHalfword(EepromFileStream *EepromFile_p, uint16_t data){
  uint8_t buf[2] = {
      (data >> 8) & 0xFF,
      data        & 0xFF
  };
  return chFileStreamWrite(EepromFile_p, buf, sizeof(buf));
}

size_t EepromWriteWord(EepromFileStream *EepromFile_p, uint32_t data){
  uint8_t buf[4] = {
      (data >> 24) & 0xFF,
      (data >> 16) & 0xFF,
      (data >> 8)  & 0xFF,
      data         & 0xFF
  };
  return chFileStreamWrite(EepromFile_p, buf, sizeof(buf));
}
