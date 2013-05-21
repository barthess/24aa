#include "main.h"
#include "eeprom_file.hpp"
#include "eeprom_fs.hpp"
#include "eeprom_mtd.hpp"
#include "eeprom_file_tree.hpp"

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

#define EEPROM_I2C_ADDR         0b1010000   /* EEPROM address on bus */

/*
 ******************************************************************************
 * EXTERNS
 ******************************************************************************
 */
EepromFile ParamFile;
EepromFile MissionFile;

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
#define EEPROM_SETTINGS_SIZE          4096
#define EEPROM_SETTINGS_START_PAGE    (EEPROM_FS_TOC_SIZE / EEPROM_PAGE_SIZE)

#define EEPROM_MISSION_START_PAGE     (EEPROM_SETTINGS_START_PAGE + (EEPROM_SETTINGS_SIZE / EEPROM_PAGE_SIZE))
#define EEPROM_MISSION_SIZE           8192

/**
 * During boot process system must verify correctness of data in EEPROM.
 * This array used for automatic detection of changes in file "tree".
 */
static const inode_t param_inode = {
    EEPROM_SETTINGS_START_PAGE,
    0,
    EEPROM_SETTINGS_SIZE,
};

static const inode_t mission_inode = {
    EEPROM_MISSION_START_PAGE,
    0,
    EEPROM_MISSION_SIZE,
};


static const toc_record_t reftoc[EEPROM_FS_MAX_FILE_COUNT] = {
    {(char*)"param",   param_inode},
    {(char*)"mission", mission_inode}
};

static const EepromMtdConfig mtd_cfg = {
  &I2CD2,
  MS2ST(EEPROM_WRITE_TIME_MS),
  EEPROM_SIZE,
  EEPROM_PAGE_SIZE,
  EEPROM_I2C_ADDR,
};

static EepromMtd   eemtd(&mtd_cfg);
static EepromFs    eefs(&eemtd, reftoc, sizeof(reftoc)/sizeof(reftoc[0]));

/*
 ******************************************************************************
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 */

/*
 ******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************
 */

void EepromFileTreeInit(void){
  eefs.mount();
  ParamFile.open(&eefs, (uint8_t*)"param");
  MissionFile.open(&eefs, (uint8_t*)"mission");

  /*
   * Now you can perform regular file operations on ParamFile and MissionFile
   * objects.
   */
}






