#ifndef EEPROM_MTD_HPP_
#define EEPROM_MTD_HPP_

#include "ch.hpp"
#include "hal.h"
#include "eeprom_conf.h"

typedef uint16_t eeprom_size_t;

typedef struct EepromMtdConfig {
  /**
   * Driver connecte to IC.
   */
  I2CDriver     *i2cp;
  /**
   * Time needed by IC for single page writing.
   * Check datasheet!!!
   */
  systime_t     writetime;
  /**
   * Size of memory array in pages.
   * Check datasheet!!!
   */
  size_t        devsize;
  /**
   * Size of single page in bytes.
   * Check datasheet!!!
   */
  size_t        pagesize;
  /**
   * Address of IC on I2C bus.
   */
  i2caddr_t     addr;
}EepromBlkConfig;


class EepromMtd{
public:
  EepromMtd(const EepromMtdConfig *cfg);
  msg_t read(uint8_t *data, size_t absoffset, size_t len);
  msg_t write(const uint8_t *data, size_t absoffset, size_t len);
  size_t getPageSize(void);
  msg_t massErase(void);

private:
  const EepromMtdConfig *cfg;
  uint8_t writebuf[EEPROM_PAGE_SIZE + 2];
};

#endif /* EEPROM_MTD_HPP_ */
