/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "eeprom_testsuit.h"


/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define TEST_AREA_SIZE    4096

/*
 ******************************************************************************
 * EXTERNS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */

/* buffer containing control data */
static uint8_t referencebuf[TEST_AREA_SIZE];

/* buffer for read data */
static uint8_t checkbuf[TEST_AREA_SIZE];

/* buffer for stashing data from test area */
static uint8_t backupbuf[TEST_AREA_SIZE];

/* variables for run test in different areas */
static uint32_t TestAreaStart;
static uint32_t TestAreaFinish;

/*
 *******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************
 */

static void test_wtapper_write_byte(EepromFileStream *EfsTest, uint8_t pattern, uint32_t pos){
  uint8_t u8result;
  uint8_t u8 = pattern;

  chFileStreamSeek(EfsTest, pos);
  if (EepromWriteByte(EfsTest, u8) != sizeof(u8))
    chDbgPanic("write failed");
  chFileStreamSeek(EfsTest, chFileStreamGetPosition(EfsTest) - sizeof(u8));
  u8result = EepromReadByte(EfsTest);
  if (u8 != u8result)
    chDbgPanic("veryfication failed");
}

static void test_wtapper_write_halfword(EepromFileStream *EfsTest, uint8_t pattern, uint32_t pos){
  uint16_t u16result;
  uint16_t u16 = 0;
  u16 |= pattern << 8 | pattern;

  chFileStreamSeek(EfsTest, pos);
  if (EepromWriteHalfword(EfsTest, u16) != sizeof(u16))
    chDbgPanic("write failed");
  chFileStreamSeek(EfsTest, chFileStreamGetPosition(EfsTest) - sizeof(u16));
  u16result = EepromReadHalfword(EfsTest);
  if (u16 != u16result)
    chDbgPanic("veryfication failed");
}

static void test_wtapper_write_word(EepromFileStream *EfsTest, uint8_t pattern, uint32_t pos){
  uint32_t u32result;
  uint32_t u32 = 0;
  u32 |= pattern << 24 | pattern << 16 | pattern << 8 | pattern;

  chFileStreamSeek(EfsTest, pos);
  if (EepromWriteWord(EfsTest, u32) != sizeof(u32))
    chDbgPanic("write failed");
  chFileStreamSeek(EfsTest, chFileStreamGetPosition(EfsTest) - sizeof(u32));
  u32result = EepromReadWord(EfsTest);
  if (u32 != u32result)
    chDbgPanic("veryfication failed");
}

static void test_wrapper_write_all(EepromFileStream *EfsTest, uint8_t pattern, uint32_t pos){
  test_wtapper_write_byte(EfsTest, pattern, pos);
  test_wtapper_write_halfword(EfsTest, pattern, pos);
  test_wtapper_write_word(EfsTest, pattern, pos);
  palTogglePad(GPIOB, GPIOB_LED_B);
}

static void test_wrapper_write_all_patterns(EepromFileStream *EfsTest, uint32_t pos){
  uint8_t r;
  chSysLock();
  r = rand() & 0xFF;
  chSysUnlock();
  test_wrapper_write_all(EfsTest, 0x55, pos);
  test_wrapper_write_all(EfsTest, 0xAA, pos);
  test_wrapper_write_all(EfsTest, 0x00, pos);
  test_wrapper_write_all(EfsTest, 0xFF, pos);
  test_wrapper_write_all(EfsTest, r,    pos);
}

/**
 * Fills eeprom area with pattern, than read it back and compare
 */
static void __block_api_write(EepromFileStream *EfsTest, uint8_t pattern, uint32_t len, uint8_t misaligment){
  uint32_t i = 0;
  uint32_t status = 0;
  uint32_t pos = 0;

  /* fill buffer with pattern */
  for (i = 0; i < len; i++)
    referencebuf[i] = pattern;

  /* move to begin of test area */
  chFileStreamSeek(EfsTest, TestAreaStart + misaligment);
  pos = chFileStreamGetPosition(EfsTest);
  if (pos != TestAreaStart + misaligment)
    chDbgPanic("file seek error");

  /* write */
  status = chFileStreamWrite(EfsTest, referencebuf, len);
  if (status < len)
    chDbgPanic("write failed");

  /* check */
  pos = chFileStreamGetPosition(EfsTest);
  if (pos != len + TestAreaStart + misaligment)
    chDbgPanic("file seek error");
  chFileStreamSeek(EfsTest, pos - len);
  status = chFileStreamRead(EfsTest, checkbuf, len);
  if (status < len)
    chDbgPanic("veryfication failed");
  if (memcmp(referencebuf, checkbuf, len) != 0)
    chDbgPanic("veryfication failed");
}

static void test_api_all(EepromFileStream *EfsTest, uint32_t len, uint8_t misaligment){
  uint8_t r;
  chSysLock();
  r = rand() & 0xFF;
  chSysUnlock();

  __block_api_write(EfsTest, 0x55, len, misaligment);
  __block_api_write(EfsTest, 0xAA, len, misaligment);
  __block_api_write(EfsTest, 0x00, len, misaligment);
  __block_api_write(EfsTest, 0xFF, len, misaligment);
  __block_api_write(EfsTest, r,    len, misaligment);

  palTogglePad(GPIOB, GPIOB_LED_B);
}

void eeprom_testsuit(EepromFileStream *EfsTest){
  int8_t i = 0;
  int8_t j = 0;
  int32_t n = 0;
  int32_t pagesize = EfsTest->cfg->pagesize;

  /* backup data from test area */
  chFileStreamSeek(EfsTest, TestAreaStart);
  if (chFileStreamRead(EfsTest, backupbuf, TEST_AREA_SIZE) < TEST_AREA_SIZE)
    chDbgPanic("backuping failed");

  /* first check the whole test area */
  test_api_all(EfsTest, TEST_AREA_SIZE, 0);

  for (i = -2; i < 3; i++){
    for (j = 0; j < 3; j++){
      /* large block tests */
      n = pagesize;
      while (n < (TEST_AREA_SIZE - 3 * pagesize)){
        test_api_all(EfsTest, n + i,  j);
        test_api_all(EfsTest, n + i,  pagesize - 1 - j);
        n *= pagesize;
      }
      /* small block tests */
      for (n = 2; n < 7; n++){
        test_api_all(EfsTest, n + i,     j);
        test_api_all(EfsTest, n + i,     pagesize - 1 - j);
        test_api_all(EfsTest, n*10 + i,  j);
        test_api_all(EfsTest, n*10 + i,  pagesize - 1 - j);
      }
    }
  }

  /* wrapper fucntions test */
  test_wrapper_write_all_patterns(EfsTest, TestAreaStart);
  test_wrapper_write_all_patterns(EfsTest, TestAreaStart + 1);
  for (i = -4; i < 5; i++)
    test_wrapper_write_all_patterns(EfsTest, TestAreaStart + pagesize + i);
  test_wrapper_write_all_patterns(EfsTest, TestAreaFinish - 5);

  /* personally check end of test area */
  test_wtapper_write_byte    (EfsTest, 0x55, TestAreaFinish - 1);
  test_wtapper_write_halfword(EfsTest, 0xAA, TestAreaFinish - 2);
  test_wtapper_write_word    (EfsTest, 0xA5, TestAreaFinish - 4);

  /* roll back data from backup */
  chFileStreamSeek(EfsTest, TestAreaStart);
  if (chFileStreamWrite(EfsTest, backupbuf, TEST_AREA_SIZE) < TEST_AREA_SIZE)
    chDbgPanic("rolling back failed");
}

/*
 *******************************************************************************
 * EXPORTED FUNCTIONS
 *******************************************************************************
 */

void eeprom_testsuit_run(void){
#define EEPROM_PAGE_SIZE  128
#define EEPROM_SIZE       65536
#define EEPROM_I2C_ADDR   0b1010000
#define EEPROM_WRITE_TIME 20
#define EEPROM_TX_DEPTH   (EEPROM_PAGE_SIZE + 2)

  uint8_t eeprom_buf[EEPROM_TX_DEPTH];
  const I2CEepromFileConfig eeprom_cfg = {
    &I2CD2,
    0,
    EEPROM_SIZE,
    EEPROM_SIZE,
    EEPROM_PAGE_SIZE,
    EEPROM_I2C_ADDR,
    MS2ST(EEPROM_WRITE_TIME),
    FALSE,
    eeprom_buf,
  };
  EepromFileStream EfsTest;

  EepromFileOpen(&EfsTest, &eeprom_cfg);

  /* run from zero address */
  TestAreaStart = 0;
  TestAreaFinish = TestAreaStart + TEST_AREA_SIZE;
  eeprom_testsuit(&EfsTest);

  /* run somwhere in middle */
  TestAreaStart = (EEPROM_SIZE / 2) - (TEST_AREA_SIZE / 2) - EEPROM_PAGE_SIZE;
  TestAreaFinish = TestAreaStart + TEST_AREA_SIZE;
  eeprom_testsuit(&EfsTest);

  /* run at the end */
  TestAreaStart = EEPROM_SIZE - TEST_AREA_SIZE;
  TestAreaFinish = TestAreaStart + TEST_AREA_SIZE;
  eeprom_testsuit(&EfsTest);

  chFileStreamClose(&EfsTest);

  /* switch off led */
  palSetPad(GPIOB, GPIOB_LED_B);
}




