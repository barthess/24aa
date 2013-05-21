#ifndef EEPROM_MTD_CONF_H_
#define EEPROM_MTD_CONF_H_

#define EEPROM_PAGE_SIZE        128    /* page size in bytes. Consult datasheet. */
#define EEPROM_SIZE             65536  /* total amount of memory in bytes */
#define EEPROM_WRITE_TIME_MS    20     /* time to write one page in mS. Consult datasheet! */

#define eeprom_led_on()         {palClearPad(GPIOB, GPIOB_LED_R);}
#define eeprom_led_off()        {palSetPad(GPIOB, GPIOB_LED_R);}

#endif /* EEPROM_MTD_CONF_H_ */
