/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "eeprom_testsuit.h"
#include "cli.h"
#include "chprintf.h"

#include "eeprom_conf.h"

#if USE_EEPROM_TEST_SUIT
/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define TEST_AREA_START     0
#define TEST_AREA_SIZE      8192
#define TEST_AREA_END       (TEST_AREA_START + TEST_AREA_SIZE)

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
  &EEPROM_I2CD,
  0,
  0,
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
  MS2ST(EEPROM_WRITE_TIME_MS),
  o_buf,
};

static I2CEepromFileConfig icfg = {
  &EEPROM_I2CD,
  0,
  0,
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
  MS2ST(EEPROM_WRITE_TIME_MS),
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
  chThdSleepMilliseconds(100);
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
 *       |<--------- outer file ------------>|
 *       |                                   |
 * ======b1==b2========================b3===b4======
 * |         |                          |          |
 * |         |<------ inner file ------>|          |
 * |<----------------- EEPROM -------------------->|
 */
static void overflow_check(uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4,
                          uint32_t istart, uint32_t ilen,
                          uint8_t pattern, bool_t pat_autoinc,
                          BaseSequentialStream *sdp){
  uint32_t status, i, n;

  chDbgCheck(ilen < (b4-b1),"sequences more than length of outer file can not be verified");

  chprintf(sdp, "b1=%u, b2=%u, b3=%u, b4=%u, istart=%u, ilen=%u, ",
            b1, b2, b3, b4, istart, ilen);
  cli_print("autoinc=");
  if (pat_autoinc)    cli_print("TRUE");
  else                cli_print("FALSE");
  chThdSleepMilliseconds(50);

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
  n = b2 - b1 + istart;
  if ((ilen + istart) > (b3-b2))
    i = b3 - b2 - istart;
  else
    i = ilen;
  while (i > 0){
    referencebuf[n] = pattern;
    n++;
    i--;
    if (pat_autoinc)
      pattern++;
    }

  /* check buffer */
  n = 0;
  while (n < ilen){
    checkbuf[n] = pattern;
    n++;
    if (pat_autoinc)
      pattern++;
  }

  /* now write check buffer content into inner file */
  chThdSleepMilliseconds(20);
  chFileStreamSeek(&ifile, istart);
  status = chFileStreamWrite(&ifile, checkbuf, ilen);

  if ((istart + ilen) > (b3 - b2)){ /* data must be clamped */
    if (status != (b3 - b2 - istart))
      chDbgPanic("not all data written or overflow ocrred");
  }
  else{/* data fitted in file */
    if (status != ilen)
      chDbgPanic("not all data written or overflow ocrred");
  }

  /* read outer file and compare content with reference buffer */
  memset(checkbuf, 0x00, b4-b1);
  chFileStreamSeek(&ofile, 0);
  status = chFileStreamRead(&ofile, checkbuf, b4-b1);
  if (status != (b4-b1))
    chDbgPanic("reading back failed");
  if (memcmp(referencebuf, checkbuf, b4-b1) != 0)
    chDbgPanic("veryfication failed");

  chFileStreamClose(&ofile);
  chFileStreamClose(&ifile);
  OK();
}

/**
 *
 */
static WORKING_AREA(EepromTestThreadWA, 1024);
msg_t EepromTestThread(void *sdp){
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
  OK();
  cli_print("test fill with 0xAA");
  pattern_fill(&ofile, 0xAA);
  if (chThdShouldTerminate()){goto END;}
  OK();
  cli_print("test fill with 0x55");
  pattern_fill(&ofile, 0x55);
  if (chThdShouldTerminate()){goto END;}
  OK();
  cli_print("test fill with 0x00");
  pattern_fill(&ofile, 0x00);
  if (chThdShouldTerminate()){goto END;}
  OK();
  cli_print("Closing file");
  chFileStreamClose(&ofile);
  OK();


  uint32_t b1, b2, b3, b4, istart, ilen;
  uint8_t pattern = 0x55;
  b1 = TEST_AREA_START;
  b2 = TEST_AREA_START + EEPROM_PAGE_SIZE;
  b3 = TEST_AREA_END - EEPROM_PAGE_SIZE;
  b4 = TEST_AREA_END;
  istart = 0;
  ilen = b3-b2;

  cli_println("    Linear barriers testing.");
  chThdSleepMilliseconds(20);
  overflow_check( b1, b2, b3, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2, b3, b4, istart + 1, ilen - 1, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2, b3, b4, istart + 1, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2, b3, b4, istart + 1, ilen + 23, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 1, b3 + 1, b4, istart, ilen, pattern,  FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 2, b3 + 2, b4, istart + 2, ilen, pattern,  FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 1, b3 + 1, b4, istart + 1, ilen + 23, pattern,  FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 2, b3 + 2, b4, istart + 1, ilen + 23, pattern,  FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 + 2, b3 - 3, b4, istart + 2, ilen, pattern, FALSE,  sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;


  overflow_check( b1, b2, b2 + 1, b4, istart, ilen, pattern,  FALSE,  sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2, b2 + 2, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2, b2 + 3, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;


  overflow_check( b1, b2 + 1, b2 + 2, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 + 1, b2 + 3, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 + 1, b2 + 4, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;

  overflow_check( b1, b2 - 1, b2,     b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 1, b2 + 1, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 1, b2 + 2, b4, istart, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;

  overflow_check( b1, b2 - 1, b2 + 1, b4, istart + 1, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 1, b2 + 2, b4, istart + 1, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;
  overflow_check( b1, b2 - 1, b2 + 3, b4, istart + 1, ilen, pattern, FALSE, sdp);
  if (chThdShouldTerminate()){goto END;}
  pattern++;

  cli_println("    Basic API testing.");
  chThdSleepMilliseconds(20);

  ocfg.barrier_low  = TEST_AREA_START;
  ocfg.barrier_hi   = TEST_AREA_END;
  EepromFileOpen(&ofile, &ocfg);
  chFileStreamSeek(&ofile, 0);
  EepromWriteByte(&ofile, 0x11);
  EepromWriteHalfword(&ofile, 0x2222);
  EepromWriteWord(&ofile, 0x33333333);
  chFileStreamSeek(&ofile, 0);
  if(EepromReadByte(&ofile) != 0x11)
    chDbgPanic("");
  if(EepromReadHalfword(&ofile) != 0x2222)
    chDbgPanic("");
  if(EepromReadWord(&ofile) != 0x33333333)
    chDbgPanic("");
  chFileStreamClose(&ofile);
  OK();

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

#endif /* USE_EEPROM_TEST_SUIT */
