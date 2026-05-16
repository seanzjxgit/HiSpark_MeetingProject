#ifndef TOUCH_FT6336U_H
#define TOUCH_FT6336U_H
 
#include <stdint.h>
#include <stdbool.h>
 
/* ===== 引脚定义 ===== */
#define TOUCH_INT_GPIO      5       /* GPIO5  PIN27  MODE_4 触摸中断 */
#define TOUCH_I2C_BUS       1       /* I2C1 */
#define TOUCH_I2C_ADDR      0x38    /* FT6336U 固定地址 */
 
/* ===== 屏幕分辨率 ===== */
#define TOUCH_SCREEN_W      320
#define TOUCH_SCREEN_H      240
 
/* ===== FT6336U 寄存器 ===== */
#define FT_REG_TD_STATUS    0x02    /* 触摸点数 */
#define FT_REG_TOUCH1_XH    0x03    /* 第1点X高字节 */
#define FT_REG_TOUCH1_XL    0x04    /* 第1点X低字节 */
#define FT_REG_TOUCH1_YH    0x05    /* 第1点Y高字节 */
#define FT_REG_TOUCH1_YL    0x06    /* 第1点Y低字节 */
 
typedef struct {
    uint16_t x;         /* 屏幕坐标X (0~319) */
    uint16_t y;         /* 屏幕坐标Y (0~239) */
    bool     pressed;   /* 是否按下 */
} touch_data_t;
 
/* ===== 函数声明 ===== */
void touch_init(void);
bool touch_get_data(touch_data_t *out);
bool touch_is_pressed(void);
void touch_hardware_test_loop(void);//触摸测试
 
#endif
 