#ifndef TOUCH_NS2009_H
#define TOUCH_NS2009_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 引脚定义 ===== */
#define TOUCH_INT_GPIO      5       /* GPIO5  PIN27  MODE_4 触摸中断 */
#define TOUCH_I2C_BUS       1       /* I2C1 */
#define TOUCH_I2C_ADDR      0x48    /* NS2009 默认地址 A0=A1=0 */

/* ===== 屏幕分辨率 ===== */
#define TOUCH_SCREEN_W      320
#define TOUCH_SCREEN_H      240

/* ===== ADC原始值范围（需校准）===== */
#define TOUCH_X_MIN         200
#define TOUCH_X_MAX         3900
#define TOUCH_Y_MIN         200
#define TOUCH_Y_MAX         3900

/* ===== NS2009 命令字节 ===== */
/* C3C2C1C0 X PD0 M X */
/* 差分模式 12bit PD0=0省电模式 */
#define NS2009_CMD_READ_X   0xC0    /* 1100 0000 读X坐标 */
#define NS2009_CMD_READ_Y   0xD0    /* 1101 0000 读Y坐标 */
#define NS2009_CMD_READ_Z1  0xE0    /* 1110 0000 读Z1压力 */
#define NS2009_CMD_READ_Z2  0xF0    /* 1111 0000 读Z2压力 */

typedef struct {
    uint16_t x;         /* 屏幕坐标X (0~319) */
    uint16_t y;         /* 屏幕坐标Y (0~239) */
    bool     pressed;   /* 是否按下 */
} touch_data_t;

/* ===== 函数声明 ===== */
void touch_init(void);
bool touch_get_data(touch_data_t *out);
bool touch_is_pressed(void);

#endif