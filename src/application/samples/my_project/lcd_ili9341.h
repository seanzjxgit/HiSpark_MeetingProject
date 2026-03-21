#ifndef LCD_ILI9341_H
#define LCD_ILI9341_H

#include <stdint.h>
#include "spi.h"
#include "gpio.h"
#include "pinctrl.h"

/* ===== 引脚定义 ===== */
#define LCD_RST_GPIO    0       /* IO0 → RST  */
#define LCD_BL_GPIO     1       /* IO1 → 背光 */
#define LCD_DC_GPIO     2       /* IO2 → DCX  */
#define LCD_SPI_BUS     SPI_BUS_0

/* ===== 屏幕分辨率 ===== */
#define LCD_WIDTH       320
#define LCD_HEIGHT      240

/* ===== 函数声明 ===== */
void lcd_init(void);
void lcd_set_window(uint16_t x1, uint16_t y1,
                    uint16_t x2, uint16_t y2);
void lcd_write_pixels(uint16_t *data, uint32_t len);
void lcd_backlight(uint8_t on);

#endif