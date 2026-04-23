#include "touch_ns2009.h"
#include "i2c.h"
#include "gpio.h"
#include "pinctrl.h"
#include "cmsis_os2.h"

/* =====================================================================
 * NS2009 I2C 底层操作
 * ===================================================================== */

/* 写命令字节到NS2009 */
static bool ns2009_write_cmd(uint8_t cmd)
{
    uint8_t buf[1] = { cmd };
    i2c_data_t xfer = {0};
    xfer.send_buf    = buf;
    xfer.send_len    = 1;
    xfer.receive_buf = NULL;
    xfer.receive_len = 0;

    return (uapi_i2c_master_write(TOUCH_I2C_BUS,
                                  TOUCH_I2C_ADDR,
                                  &xfer) == ERRCODE_SUCC);
}

/* 从NS2009读取2字节ADC结果 */
static bool ns2009_read_result(uint16_t *val)
{
    uint8_t buf[2] = {0};
    i2c_data_t xfer = {0};
    xfer.send_buf    = NULL;
    xfer.send_len    = 0;
    xfer.receive_buf = buf;
    xfer.receive_len = 2;

    if(uapi_i2c_master_read(TOUCH_I2C_BUS,
                             TOUCH_I2C_ADDR,
                             &xfer) != ERRCODE_SUCC) {
        return false;
    }

    /* NS2009数据格式：
     * byte0 = D11~D4（高8位）
     * byte1 = D3~D0 + 0000（低4位在高半字节）
     * 合并后右移4位得到12位ADC值 */
    *val = ((uint16_t)buf[0] << 4) | (buf[1] >> 4);
    return true;
}

/* 发送命令后读取ADC值（写+读） */
static bool ns2009_read_adc(uint8_t cmd, uint16_t *val)
{
    if(!ns2009_write_cmd(cmd)) return false;
    osDelay(2);   /* 等待ADC转换，约2ms */
    return ns2009_read_result(val);
}

/* =====================================================================
 * 多次采样取平均滤波
 * ===================================================================== */
#define SAMPLE_COUNT  4

static uint16_t ns2009_sample_avg(uint8_t cmd)
{
    uint32_t sum   = 0;
    uint8_t  valid = 0;

    for(uint8_t i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t v = 0;
        if(ns2009_read_adc(cmd, &v)) {
            sum += v;
            valid++;
        }
    }
    return (valid > 0) ? (uint16_t)(sum / valid) : 0;
}

/* =====================================================================
 * ADC原始值映射到屏幕坐标
 * ===================================================================== */
static uint16_t map_x(uint16_t adc)
{
    if(adc <= TOUCH_X_MIN) return 0;
    if(adc >= TOUCH_X_MAX) return TOUCH_SCREEN_W - 1;
    return (uint16_t)((uint32_t)(adc - TOUCH_X_MIN) *
                      (TOUCH_SCREEN_W - 1) /
                      (TOUCH_X_MAX - TOUCH_X_MIN));
}

static uint16_t map_y(uint16_t adc)
{
    if(adc <= TOUCH_Y_MIN) return 0;
    if(adc >= TOUCH_Y_MAX) return TOUCH_SCREEN_H - 1;
    return (uint16_t)((uint32_t)(adc - TOUCH_Y_MIN) *
                      (TOUCH_SCREEN_H - 1) /
                      (TOUCH_Y_MAX - TOUCH_Y_MIN));
}

/* =====================================================================
 * 触摸初始化
 * ===================================================================== */
void touch_init(void)
{
    /* I2C引脚复用 */
    uapi_pin_set_mode(15, PIN_MODE_2);  /* GPIO15 → I2C1_SDA */
    uapi_pin_set_mode(16, PIN_MODE_2);  /* GPIO16 → I2C1_SCL */

    /* 初始化I2C1，100KHz，hscode=0（标准模式不用高速码）*/
    uapi_i2c_master_init(TOUCH_I2C_BUS, 100000, 0);

    /* INT引脚配置为输入 */
    uapi_pin_set_mode(TOUCH_INT_GPIO, PIN_MODE_4);
    uapi_gpio_set_dir(TOUCH_INT_GPIO, GPIO_DIRECTION_INPUT);

    /* 发一次命令唤醒NS2009 */
    ns2009_write_cmd(NS2009_CMD_READ_Y);
    osDelay(10);
}

/* =====================================================================
 * 检测是否有触摸（INT低电平=有触摸）
 * ===================================================================== */
bool touch_is_pressed(void)
{
    /* uapi_gpio_get_val 只有1个参数，直接返回电平值 */
    return (uapi_gpio_get_val(TOUCH_INT_GPIO) == GPIO_LEVEL_LOW);
}

/* =====================================================================
 * 读取触摸数据
 * ===================================================================== */
bool touch_get_data(touch_data_t *out)
{
    if(out == NULL) return false;

    if(!touch_is_pressed()) {
        out->pressed = false;
        return true;
    }

    /* 读取X、Y原始ADC值 */
    uint16_t adc_x = ns2009_sample_avg(NS2009_CMD_READ_X);
    uint16_t adc_y = ns2009_sample_avg(NS2009_CMD_READ_Y);

    /* 读Z1判断压力是否真实 */
    uint16_t z1 = 0;
    ns2009_read_adc(NS2009_CMD_READ_Z1, &z1);

    /* z1太小说明没有真实触摸 */
    if(z1 < 100 || adc_x == 0 || adc_y == 0) {
        out->pressed = false;
        return true;
    }

    out->x       = map_x(adc_x);
    out->y       = map_y(adc_y);
    out->pressed = true;

    return true;
}