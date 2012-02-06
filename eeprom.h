/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#ifndef EEPROM_FILE_H_
#define EEPROM_FILE_H_

#include "ch.h"
#include "hal.h"

#include "24aa.h"

/**
 * @brief   @p EepromFileStream specific data.
 */
#define _eeprom_file_stream_data                                            \
  _base_file_stream_data                                                    \
  uint32_t    errors;                                                       \
  uint32_t    position;

/**
 * @brief   @p EepromFileStream virtual methods table.
 */
struct EepromFilelStreamVMT {
  _base_file_stream_methods
};

/**
 * @brief   EEPROM file stream driver class.
 * @details This class extends @p BaseFileStream by adding some fields.
 */
typedef struct EepromFileStream EepromFileStream;
struct EepromFileStream {
  /** @brief Virtual Methods Table.*/
  const struct EepromFilelStreamVMT *vmt;
  _eeprom_file_stream_data
};

/**
 * @brief   File Stream read.
 * @details The function reads data from a file into a buffer.
 *
 * @param[in] ip        pointer to a @p BaseSequentialStream or derived class
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred
 * @return              The number of bytes transferred. The return value can
 *                      be less than the specified number of bytes if the
 *                      stream reaches the end of the available data.
 *
 * @api
 */
#define chFileStreamRead(ip, bp, n)  (chSequentialStreamRead(ip, bp, n))

/**
 * @brief   File Stream write.
 * @details The function writes data from a buffer to a file.
 *
 * @param[in] ip        pointer to a @p BaseSequentialStream or derived class
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred
 * @return              The number of bytes transferred. The return value can
 *                      be less than the specified number of bytes if the
 *                      stream reaches a physical end of file and cannot be
 *                      extended.
 *
 * @api
 */
#define chFileStreamWrite(ip, bp, n) (chSequentialStreamWrite(ip, bp, n))


EepromFileStream* EepromOpen(EepromFileStream* efs);

uint8_t  EepromReadByte(EepromFileStream *EepromFile_p);
uint16_t EepromReadHalfword(EepromFileStream *EepromFile_p);
uint32_t EepromReadWord(EepromFileStream *EepromFile_p);
size_t EepromWriteByte(EepromFileStream *EepromFile_p, uint8_t data);
size_t EepromWriteHalfword(EepromFileStream *EepromFile_p, uint16_t data);
size_t EepromWriteWord(EepromFileStream *EepromFile_p, uint32_t data);

#endif /* EEPROM_FILE_H_ */
