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
#define EEPROM_PAGE_SIZE    128
#define EEPROM_SIZE         65536
#define EEPROM_I2C_ADDR     0b1010000
#define EEPROM_WRITE_TIME   20
#define EEPROM_TX_DEPTH     (EEPROM_PAGE_SIZE + 2)

#define TEST_AREA_START   0
#define TEST_AREA_SIZE    1024
#define TEST_AREA_END     (TEST_AREA_START + TEST_AREA_SIZE)

/* shortcut to print OK message*/
#define OK(); do{cli_println(" ... OK"); chThdSleepMilliseconds(20);}while(0)

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

static uint8_t o_buf[EEPROM_TX_DEPTH];
static uint8_t i_buf[EEPROM_TX_DEPTH];
static EepromFileStream ifile;
static EepromFileStream ofile;

static I2CEepromFileConfig ocfg = {
  &I2CD2,
  0,
  0,
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
  MS2ST(EEPROM_WRITE_TIME),
  FALSE,
  o_buf,
};
static I2CEepromFileConfig icfg = {
  &I2CD2,
  0,
  0,
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
  MS2ST(EEPROM_WRITE_TIME),
  FALSE,
  i_buf,
};

/*
 *******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************
 */
/**
 *
 */
static void printfileinfo(BaseSequentialStream *sdp, EepromFileStream *efsp){
  chprintf(sdp, "size = %u, position = %u, barrier_low = %u, barrier_hi = %u",
        chFileStreamGetSize(efsp),
        chFileStreamGetPosition(efsp),
        efsp->cfg->barrier_low,
        efsp->cfg->barrier_hi);
  cli_println("");
}

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
  status = chFileStreamWrite(EfsTest, referencebuf, len);
  if (status != len)
    chDbgPanic("write failed");
  OK();

  /* check */
  pos = chFileStreamGetPosition(EfsTest);
  if (pos != len)
    chDbgPanic("writing error");

  pos = 0;
  chFileStreamSeek(EfsTest, pos);
  status = chFileStreamRead(EfsTest, checkbuf, len);
  if (status != len)
    chDbgPanic("reading back failed");
  if (memcmp(referencebuf, checkbuf, len) != 0)
    chDbgPanic("veryfication failed");
}

/**
 * Create overlapped files like this:
 *
 * |<---------- outer file ------------->|
 * |                                     |
 * [b1==b2=========================b3==b4]
 *      |                           |
 *      |<------ inner file ------->|
 *
 */
static void overflow_check(uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4,
                          uint32_t istart, uint32_t ilen, bool_t iring,
                          uint8_t pattern, bool_t pat_autoinc,
                          BaseSequentialStream *sdp){
  uint32_t status, i;
  uint8_t* bp = NULL;

  chDbgCheck(ilen < (b4-b1),"sequences more than length of outer file can not be verified");

  chprintf(sdp, "b1=%u, b2=%u, b3=%u, b4=%u, istart=%u, ilen=%u, ",
            b1, b2, b3, b4, istart, ilen);
  cli_print("iring=");
  if (iring)          cli_print("TRUE, ");
  else                cli_print("FALSE, ");
  cli_print("autoinc=");
  if (pat_autoinc)    cli_print("TRUE");
  else                cli_print("FALSE");

  /* open outer file and clear it */
  ocfg.barrier_low  = b1;
  ocfg.barrier_hi   = b4;
  EepromFileOpen(&ofile, &ocfg);
  pattern_fill(&ofile, 0x00);

  /* open inner file */
  icfg.barrier_low  = b2;
  icfg.barrier_hi   = b3;
  EepromFileOpen(&ifile, &icfg);

  /* reference buffer */
  memset(referencebuf, 0x00, b4-b1);
  if (iring){
    i = ilen;
    bp = referencebuf + b2 + istart;
    while (i > 0){
      *bp = pattern;
      bp++;
      if (bp > referencebuf + b3)
        bp = referencebuf + b2;
      i--;
      if (pat_autoinc)
        pattern++;
    }
  }
  else{
    bp = referencebuf + b2 + istart;
    while (bp < referencebuf + b3){
      *bp = pattern;
      bp++;
      if (pat_autoinc)
        pattern++;
    }
  }

  /* check buffer */
  uint32_t n = 0;
  while (n < ilen){
    checkbuf[n] = pattern;
    n++;
    if (pat_autoinc)
      pattern++;
  }

  /* now write check buffer content into inner file */
  chFileStreamSeek(&ifile, istart);
  status = chFileStreamWrite(&ifile, checkbuf, ilen);
  if (status < ilen)
    chDbgPanic("not all data written");

  /* read outer file and compare content with reference buffer */
  chFileStreamSeek(&ofile, 0);
  status = chFileStreamRead(&ofile, checkbuf, b4-b1);
  if (status < (b4-b1))
    chDbgPanic("reading back failed");
  if (memcmp(referencebuf, checkbuf, b4-b1) != 0)
    chDbgPanic("veryfication failed");

  chFileStreamClose(&ofile);
  chFileStreamClose(&ifile);
}

/**
 *
 */
static WORKING_AREA(EepromTestThreadWA, 512);
static msg_t EepromTestThread(void *sdp){
  chRegSetThreadName("EepromTst");

  cli_println("basic tests");
  cli_println("--------------------------------------------------------------");
  cli_print("mount aligned file sized to whole test area");
  ocfg.barrier_low  = TEST_AREA_START;
  ocfg.barrier_hi   = TEST_AREA_END;
  EepromFileOpen(&ofile, &ocfg);
  OK();
  printfileinfo(sdp, &ofile);


  cli_print("test fill with 0xFF");
  pattern_fill(&ofile, 0xFF);
  if (chThdShouldTerminate()){goto END;}
  cli_print("test fill with 0xAA");
  pattern_fill(&ofile, 0xAA);
  if (chThdShouldTerminate()){goto END;}
  cli_print("test fill with 0x55");
  pattern_fill(&ofile, 0x55);
  if (chThdShouldTerminate()){goto END;}
  cli_print("test fill with 0x00");
  pattern_fill(&ofile, 0x00);
  if (chThdShouldTerminate()){goto END;}
  cli_print("Closing file");
  chFileStreamClose(&ofile);
  OK();

  overflow_check(
      TEST_AREA_START,                        //b1
      TEST_AREA_START + EEPROM_PAGE_SIZE,     //b2
      TEST_AREA_END - EEPROM_PAGE_SIZE,       //b3
      TEST_AREA_END,                          //b4
      0,                                      //istart
      TEST_AREA_END - 2 * EEPROM_PAGE_SIZE,   //ilen
      FALSE,                                  //iring
      0x33,                                   //pattern
      FALSE,                                  //pat_autoinc
      sdp);
  if (chThdShouldTerminate()){goto END;}

  overflow_check(
      TEST_AREA_START,                        //b1
      TEST_AREA_START + EEPROM_PAGE_SIZE,     //b2
      TEST_AREA_END - EEPROM_PAGE_SIZE,       //b3
      TEST_AREA_END,                          //b4
      0,                                      //istart
      TEST_AREA_END - 2 * EEPROM_PAGE_SIZE,   //ilen
      FALSE,                                  //iring
      0x00,                                   //pattern
      TRUE,                                   //pat_autoinc
      sdp);
  if (chThdShouldTerminate()){goto END;}

  /*
   * запись по барьерам в линейном (кольцевом) файле когда файл выровнен (невыровнен)
   *
   * проверка файла из (1, 2, 4) байт (кольцевой, линейный)
   * базовая проверка оберток
   */
  cli_println("All tests passed successfully.");
END:
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


