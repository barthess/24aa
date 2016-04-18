/*
    Abstraction layer for EEPROM ICs.

    Copyright (C) 2012..2016 Uladzimir Pylinski aka barthess

    This file is part of 24AA lib.

    24AA lib is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    24AA lib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "string.h"
#include "ch.hpp"
#include "hal.h"

#include "mtd24.hpp"
#include "nvram_fs.hpp"

#include "mtd_conf.h"
#include "nvram_test.hpp"
#include "pads.h"

using namespace chibios_rt;
using namespace nvram;

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

#define FRAM_I2C_ADDR           0b1010000
#define FRAM_I2CD               I2CD3
#define FRAM_SIZE               (1024 * 32)

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

static const I2CConfig i2cfg = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
};

static void mtd_led_on(Mtd *mtd) {
  (void)mtd;
  red_led_on();
}

static void mtd_led_off(Mtd *mtd) {
  (void)mtd;
  red_led_off();
}

static const MtdConfig fram_cfg = {
    0,
    0,
    1,
    FRAM_SIZE,
    2,
    mtd_led_on,
    mtd_led_off,
    nullptr,
    nullptr,
    mtd_led_on,
    mtd_led_off,
};

static uint8_t workbuf[MTD_WRITE_BUF_SIZE];

static Mtd24 nvram_mtd(fram_cfg, workbuf, MTD_WRITE_BUF_SIZE, &FRAM_I2CD, FRAM_I2C_ADDR);

static Fs nvram_fs(nvram_mtd);

static const uint8_t test_string[] = "Lorem ipsum dolor";
static uint8_t read_back_buf[sizeof(test_string)];

/*
 *******************************************************************************
 *******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************
 *******************************************************************************
 */

/**
 *
 */
void NvramInit(void) {
  if (OSAL_SUCCESS != nvram_fs.mount()){
    nvram_fs.mkfs();
    if (OSAL_SUCCESS != nvram_fs.mount()){
      osalSysHalt("Storage broken");
    }
  }
}

/**
 * @brief     Try to open file.
 * @details   Functions creates file if it does not exists.
 *
 * @param[in] name    file name.
 * @param[in] size    size of file to be created if it does not exists yet.
 *
 * @return            pointer to file.
 */
File *NvramTryOpen(const char *name, size_t size) {

  /* try to open exiting file */
  File *file = nvram_fs.open(name);

  if (nullptr == file) {
    /* boot strapping when first run */
    if (nvram_fs.df() < size)
      osalSysHalt("Not enough free space in nvram to create file");
    else {
      /* No such file. Just create it. */
      file = nvram_fs.create(name, size);
      osalDbgCheck(nullptr != file);
    }
  }

  return file;
}

/**
 *
 */
int main(void) {

  halInit();
  System::init();
  i2cStart(&FRAM_I2CD, &i2cfg);

  green_led_off();
  red_led_off();

  nvram_power_on();
  chThdSleepMilliseconds(10); /* time for NVRAM IC start */

  /* run extencive data destruction tests */
  nvramTestSuite(nvram_mtd);

  /* normal test and simple example of usage */
  NvramInit();
  File *f = NvramTryOpen("test0", 1024);
  osalDbgCheck(nullptr != f);
  fileoffset_t status;

  status = f->write(test_string, sizeof(test_string));
  osalDbgCheck(sizeof(test_string) == status);

  status = f->setPosition(0);
  osalDbgCheck(FILE_OK == status);

  status = f->read(read_back_buf, sizeof(test_string));
  osalDbgCheck(sizeof(test_string) == status);

  status = strcmp((const char *)read_back_buf, (const char *)test_string);
  osalDbgCheck(0 == status);

  nvram_fs.close(f);

  nvram_power_off();

  while (TRUE){
    osalThreadSleepMilliseconds(100);
    green_led_on();
    osalThreadSleepMilliseconds(100);
    green_led_off();
  }
  return 0;
}



