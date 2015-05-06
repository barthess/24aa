#ifndef MTD_CONF_H_
#define MTD_CONF_H_

#include "pads.h"

#define eeprom_led_on()     red_led_on()
#define eeprom_led_off()    red_led_off()

#define MTD_USE_MUTUAL_EXCLUSION            TRUE
#define MTD_WRITE_BUF_SIZE                  (32 + 2)

#endif /* MTD_CONF_H_ */
