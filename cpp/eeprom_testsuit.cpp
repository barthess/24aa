#include <string.h>

#include "ch.hpp"
#include "hal.h"
#include "eeprom_file.hpp"

#if USE_EEPROM_TEST_SUIT
/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */
#define EEPROM_I2CD             I2CD2
#define EEPROM_I2C_ADDR         0b1010000   /* EEPROM address on bus */

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
#define START_TEST_PAGE     (EEPROM_FS_TOC_SIZE / EEPROM_PAGE_SIZE)

static const EepromMtdConfig mtd_cfg = {
  &EEPROM_I2CD,
  MS2ST(EEPROM_WRITE_TIME_MS),
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
};

static EepromMtd   eemtd(&mtd_cfg);

static uint8_t mtdbuf[4096];
static uint8_t filebuf[4096];
static uint8_t refbuf[4096];
static uint8_t pattern = 0x55;

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */

void run_unit(uint16_t startpage, uint16_t pageoffset, uint16_t size){
  fileoffset_t len;
  toc_record_t toc[1] = {{(char*)"test",   {startpage, pageoffset, size}}};
  EepromFs eefs(&eemtd, toc, 1);
  EepromFile testfile;
  size_t filestatus = 0;
  size_t cmpstatus = 0;
  msg_t mtdstatus = RDY_RESET;
  size_t absoffset = 0;

  memset(mtdbuf, 0, sizeof(mtdbuf));
  memset(refbuf, 0, sizeof(refbuf));
  memset(filebuf, 0, sizeof(filebuf));

  // start of file relative to MTD start
  absoffset = toc[0].inode.startpage * EEPROM_PAGE_SIZE + toc[0].inode.pageoffset;

  eefs.mount();
  mtdstatus = eemtd.read(refbuf, 0, EEPROM_FS_TOC_SIZE); // store TOC in refbuf
  chDbgCheck((RDY_OK == mtdstatus), "");

  testfile.open(&eefs, (uint8_t*)"test");
  len = testfile.getSize();

  // fill file with pattern
  memset(filebuf, pattern, sizeof(filebuf));
  memset(refbuf + absoffset, pattern, size);
  filestatus = testfile.write(filebuf, len);
  chDbgCheck((len == filestatus), "");

  // main check
  mtdstatus = eemtd.read(mtdbuf, 0, sizeof(mtdbuf));
  chDbgCheck((RDY_OK == mtdstatus), "");

  cmpstatus = memcmp(mtdbuf, refbuf, sizeof(mtdbuf));
  chDbgCheck((0 == cmpstatus), "");

  testfile.close();
  eefs.umount();
  pattern++;
}

/**
 *
 */
void uberunit(uint16_t sp, uint16_t off){
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2);

  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE     - 1);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2 - 1);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE     + 1);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2 + 1);

  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE     - 2);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2 - 2);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE     + 2);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2 + 2);

  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE     - 3);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2 - 3);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE     + 3);
  run_unit(START_TEST_PAGE + sp, off, EEPROM_PAGE_SIZE * 2 + 3);
}

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

void EepromTestSuit(void){
  msg_t mtdstatus = RDY_RESET;
  mtdstatus = eemtd.massErase();
  chDbgCheck((RDY_OK == mtdstatus), "");

  // test very small files
  run_unit(START_TEST_PAGE, 0, 2);
  run_unit(START_TEST_PAGE, 1, 2);
  run_unit(START_TEST_PAGE, 0, 1);
  run_unit(START_TEST_PAGE, 1, 1);

  uint16_t sp = 0;
  uint16_t off = 0;
  for (sp=0; sp<3; sp++){
    for (off=0; off<3; off++){
      uberunit(sp, off);
    }
    uberunit(sp, EEPROM_PAGE_SIZE - 2);
    uberunit(sp, EEPROM_PAGE_SIZE - 1);
  }
}

#endif /* USE_EEPROM_TEST_SUIT */
