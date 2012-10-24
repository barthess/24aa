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
#include "cli.h"
#include "chprintf.h"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define TEST_AREA_START   0
#define TEST_AREA_SIZE    1024
#define TEST_AREA_END     (TEST_AREA_START + TEST_AREA_SIZE)

/*
 ******************************************************************************
 * EXTERNS
 ******************************************************************************
 */
extern MemoryHeap ThdHeap;

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
  test_wrapper_write_all(EfsTest, 0x55, pos);
  test_wrapper_write_all(EfsTest, 0xAA, pos);
  test_wrapper_write_all(EfsTest, 0x00, pos);
  test_wrapper_write_all(EfsTest, 0xFF, pos);
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
  chFileStreamSeek(EfsTest, TEST_AREA_START + misaligment);
  pos = chFileStreamGetPosition(EfsTest);
  if (pos != TEST_AREA_START + misaligment)
    chDbgPanic("file seek error");

  /* write */
  status = chFileStreamWrite(EfsTest, referencebuf, len);
  if (status < len)
    chDbgPanic("write failed");

  /* check */
  pos = chFileStreamGetPosition(EfsTest);
  if (pos != len + TEST_AREA_START + misaligment)
    chDbgPanic("file seek error");
  chFileStreamSeek(EfsTest, pos - len);
  status = chFileStreamRead(EfsTest, checkbuf, len);
  if (status < len)
    chDbgPanic("veryfication failed");
  if (memcmp(referencebuf, checkbuf, len) != 0)
    chDbgPanic("veryfication failed");
}

static void test_api_all(EepromFileStream *EfsTest, uint32_t len, uint8_t misaligment){
  __block_api_write(EfsTest, 0x55, len, misaligment);
  __block_api_write(EfsTest, 0xAA, len, misaligment);
  __block_api_write(EfsTest, 0x00, len, misaligment);
  __block_api_write(EfsTest, 0xFF, len, misaligment);

  palTogglePad(GPIOB, GPIOB_LED_B);
}

void eeprom_testsuit(EepromFileStream *EfsTest){
  int8_t i = 0;
  int8_t j = 0;
  int32_t n = 0;
  int32_t pagesize = EfsTest->cfg->pagesize;

  /* backup data from test area */
  chFileStreamSeek(EfsTest, TEST_AREA_START);
  if (chFileStreamRead(EfsTest, backupbuf, TEST_AREA_SIZE) < TEST_AREA_SIZE)
    chDbgPanic("backing up failed");

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
  test_wrapper_write_all_patterns(EfsTest, TEST_AREA_START);
  test_wrapper_write_all_patterns(EfsTest, TEST_AREA_START + 1);
  for (i = -4; i < 5; i++)
    test_wrapper_write_all_patterns(EfsTest, TEST_AREA_START + pagesize + i);
  test_wrapper_write_all_patterns(EfsTest, TEST_AREA_END - 5);

  /* personally check end of test area */
  test_wtapper_write_byte    (EfsTest, 0x55, TEST_AREA_END - 1);
  test_wtapper_write_halfword(EfsTest, 0xAA, TEST_AREA_END - 2);
  test_wtapper_write_word    (EfsTest, 0xA5, TEST_AREA_END - 4);

  /* roll back data from backup */
  chFileStreamSeek(EfsTest, TEST_AREA_START);
  if (chFileStreamWrite(EfsTest, backupbuf, TEST_AREA_SIZE) < TEST_AREA_SIZE)
    chDbgPanic("rolling back failed");
}

/**
 *
 */
//static WORKING_AREA(EepromTestThreadWA, 512);
//static msg_t EepromTestThread(void *sdp){
//  chRegSetThreadName("EepromTst");
//
//  uint8_t eeprom_buf[EEPROM_TX_DEPTH];
//  I2CEepromFileConfig eeprom_cfg = {
//    &I2CD2,
//    0,
//    EEPROM_SIZE,
//    EEPROM_SIZE,
//    EEPROM_PAGE_SIZE,
//    EEPROM_I2C_ADDR,
//    MS2ST(EEPROM_WRITE_TIME),
//    FALSE,
//    eeprom_buf,
//  };
//  EepromFileStream EfsTest;
//
//  EepromFileOpen(&EfsTest, &eeprom_cfg);
//
//  /* run from zero address */
//  TEST_AREA_START = 0;
//  TEST_AREA_END = TEST_AREA_START + TEST_AREA_SIZE;
//  eeprom_testsuit(&EfsTest);
//
//  /* run somwhere in middle */
//  TEST_AREA_START = (EEPROM_SIZE / 2) - (TEST_AREA_SIZE / 2) - EEPROM_PAGE_SIZE;
//  TEST_AREA_END = TEST_AREA_START + TEST_AREA_SIZE;
//  eeprom_testsuit(&EfsTest);
//
//  /* run at the end */
//  TEST_AREA_START = EEPROM_SIZE - TEST_AREA_SIZE;
//  TEST_AREA_END = TEST_AREA_START + TEST_AREA_SIZE;
//  eeprom_testsuit(&EfsTest);
//
//  chFileStreamClose(&EfsTest);
//
//  /* switch off led */
//  palSetPad(GPIOB, GPIOB_LED_B);
//
//  return 0;
//}

/* shortcut to print OK message*/
#define OK(); do{cli_println(" ... OK"); chThdSleepMilliseconds(20);}while(0)

#define EEPROM_PAGE_SIZE    128
#define EEPROM_SIZE         65536
#define EEPROM_I2C_ADDR     0b1010000
#define EEPROM_WRITE_TIME   20
#define EEPROM_TX_DEPTH     (EEPROM_PAGE_SIZE + 2)

/**
 * Fills eeprom area with pattern, than read it back and compare
 */
static void pattern_fill(EepromFileStream *EfsTest, uint8_t pattern){
  uint32_t i = 0;
  uint32_t status = 0;
  uint32_t pos = 0;
  uint32_t len = chFileStreamGetSize(EfsTest);

  /* fill buffer with pattern */
  for (i = 0; i < len; i++)
    referencebuf[i] = pattern;

  /* move to begin of test area */
  pos = 0;
  chFileStreamSeek(EfsTest, pos);
  if (pos != chFileStreamGetPosition(EfsTest))
    chDbgPanic("file seek error");

  /* write */
  cli_print("    fill with pattern");
  status = chFileStreamWrite(EfsTest, referencebuf, len);
  if (status < len)
    chDbgPanic("write failed");
  OK();

  /* check */
  cli_print("    reading and comparing");
  pos = chFileStreamGetPosition(EfsTest);
  if (pos != len)
    chDbgPanic("writing error");

  pos = 0;
  chFileStreamSeek(EfsTest, pos);
  status = chFileStreamRead(EfsTest, checkbuf, len);
  if (status < len)
    chDbgPanic("reading back failed");
  if (memcmp(referencebuf, checkbuf, len) != 0)
    chDbgPanic("veryfication failed");
  OK();
}

/**
 *
 */
static void printfileinfo(BaseSequentialStream *sdp, EepromFileStream *efsp){
  chprintf(sdp, "size = %u, position = %u, barrier_low = %u, barrier_hi = %u,",
        chFileStreamGetSize(efsp),
        chFileStreamGetPosition(efsp),
        efsp->cfg->barrier_low,
        efsp->cfg->barrier_hi);
  cli_println("");
}

/**
 *
 */
static WORKING_AREA(EepromTestThreadWA, 512);
static msg_t EepromTestThread(void *sdp){
  chRegSetThreadName("EepromTst");

  uint8_t eeprom_buf[EEPROM_TX_DEPTH];
  EepromFileStream EfsTest;

  I2CEepromFileConfig eeprom_cfg = {
    &I2CD2,
    0,
    0,
    EEPROM_SIZE,
    EEPROM_PAGE_SIZE,
    EEPROM_I2C_ADDR,
    MS2ST(EEPROM_WRITE_TIME),
    FALSE,
    eeprom_buf,
  };

  cli_println("basic tests");
  cli_println("--------------------------------------------------------------");
  cli_print("mount aligned file sized to whole test area");
  eeprom_cfg.barrier_low  = TEST_AREA_START;
  eeprom_cfg.barrier_hi   = TEST_AREA_END;
  EepromFileOpen(&EfsTest, &eeprom_cfg);
  OK();
  printfileinfo(sdp, &EfsTest);

  cli_println("test fill with 0xFF");
  pattern_fill(&EfsTest, 0xFF);
  if (chThdShouldTerminate()){goto END;}
  cli_println("test fill with 0xAA");
  pattern_fill(&EfsTest, 0xAA);
  if (chThdShouldTerminate()){goto END;}
  cli_println("test fill with 0x55");
  pattern_fill(&EfsTest, 0x55);
  if (chThdShouldTerminate()){goto END;}
  cli_println("test fill with 0x00");
  pattern_fill(&EfsTest, 0x00);


  cli_println("barries functionallity tests");
  cli_println("--------------------------------------------------------------");
  /*
   * выровренная запись по барьерам в линейном файле
   * невыровненная
   * выровренная запись по барьерам в кольцевом
   * невыровненная
   * проверка файла из (1, 2, 4) байт (кольцевой, линейный)
   * проверка оберток??? (по идее не сильно надо)
   */
  cli_println("All tests passed successfully.");
END:
  chFileStreamClose(&EfsTest);
  chThdExit(0);
  return 0;
}

/*
 *******************************************************************************
 * EXPORTED FUNCTIONS
 *******************************************************************************
 */

/**
 *
 */
Thread* eepromtest_clicmd(int argc, const char * const * argv, SerialDriver *sdp){
  (void)argc;
  (void)argv;

  Thread *eeprom_tp = chThdCreateFromHeap(&ThdHeap,
                                  sizeof(EepromTestThreadWA),
                                  NORMALPRIO,
                                  EepromTestThread,
                                  sdp);
  if (eeprom_tp == NULL)
    chDbgPanic("Can not allocate memory");
  return eeprom_tp;
}


