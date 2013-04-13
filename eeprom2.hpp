/*
Copyright 2012 Uladzimir Pylinski aka barthess.
You may use this work without restrictions, as long as this notice is included.
The work is provided "as is" without warranty of any kind, neither express nor implied.
*/

#ifndef EEPROM_FILE2_CPP_H_
#define EEPROM_FILE2_CPP_H_

#include "ch.hpp"
#include "fs.hpp"
#include "hal.h"

typedef struct I2CEepromFileConfig_2 {
  /**
   * Driver connecte to IC.
   */
  I2CDriver   *i2cp;
  /**
   * Lower barrier of file in EEPROM memory array.
   */
  uint32_t    barrier_low;
  /**
   * Higher barrier of file in EEPROM memory array.
   */
  uint32_t    barrier_hi;
  /**
   * Size of memory array in bytes.
   * Check datasheet!!!
   */
  uint32_t    size;
  /**
   * Size of single page in bytes.
   * Check datasheet!!!
   */
  uint16_t    pagesize;
  /**
   * Address of IC on I2C bus.
   */
  i2caddr_t   addr;
  /**
   * Time needed by IC for single page writing.
   * Check datasheet!!!
   */
  systime_t   write_time;
  /**
   * Pointer to write buffer. The safest size is (pagesize + 2)
   */
  uint8_t     *write_buf;
}I2CEepromFileConfig;



class myEepromFile : public chibios_fs::BaseFileStreamInterface{
public:
  fileoffset_t getSize(void);
};




#endif /* EEPROM_FILE2_CPP_H_ */
