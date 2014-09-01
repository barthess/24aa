#ifndef PADS_H_
#define PADS_H_

static inline void nand_wp_assert(void)   {palClearPad(GPIOB, GPIOB_NAND_WP);}
static inline void nand_wp_release(void)  {palSetPad(GPIOB, GPIOB_NAND_WP);}
static inline void red_led_on(void)       {palSetPad(GPIOI, GPIOI_LED_R);}
static inline void red_led_off(void)      {palClearPad(GPIOI, GPIOI_LED_R);}
static inline void green_led_on(void)     {palSetPad(GPIOI, GPIOI_LED_G);}
static inline void green_led_off(void)    {palClearPad(GPIOI, GPIOI_LED_G);}
static inline void nvram_power_on(void)   {palClearPad(GPIOB, GPIOB_NVRAM_PWR);}
static inline void nvram_power_off(void)  {palSetPad(GPIOB, GPIOB_NVRAM_PWR);}

#endif /* PADS_H_ */
