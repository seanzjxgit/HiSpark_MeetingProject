#include "touch_ft6336u.h"
#include "i2c.h"
#include "gpio.h"
#include "pinctrl.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include "lcd_ili9341.h" // 测试：显式包含LCD头文件，提供 lcd_set_window 声明和 LCD_SPI_BUS 宏
#include "spi.h"         // 测试：显式包含SPI头文件，提供 spi_xfer_data_t 结构体定义

static bool ft_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    i2c_data_t xfer = {0};
    xfer.send_buf    = &reg;   /* 要读的寄存器地址 (如 0x02) */
    xfer.send_len    = 1;
    xfer.receive_buf = buf;    /* 接收缓冲区 */
    xfer.receive_len = len;

    /* 使用 writeread 接口确保时序连贯 */
    return (uapi_i2c_master_writeread(1, 0x38, &xfer) == ERRCODE_SUCC);
}

void touch_init(void)
{
    /* 严格按照复用表设置 */
    uapi_pin_set_mode(15, 2);  /* GPIO_15 -> I2C1_SDA */
    uapi_pin_set_mode(16, 2);  /* GPIO_16 -> I2C1_SCL */

    /* 提高驱动能力，防止电平被拉死 */
    uapi_pin_set_ds(15, PIN_DS_4);
    uapi_pin_set_ds(16, PIN_DS_4);

    /* 关键：I2C 引脚不要开启任何内部上下拉，完全靠外部电阻 */
    uapi_pin_set_pull(15, PIN_PULL_TYPE_DISABLE);
    uapi_pin_set_pull(16, PIN_PULL_TYPE_DISABLE);

    /* 初始化 I2C1 (总线编号必须为 1) */
    uapi_i2c_master_init(1, 100000, 0); 

    /* 设置 INT 引脚 (GPIO 5) */
    uapi_pin_set_mode(5, 4); // 复用表显示 MODE 4 是 GPIO 功能
    uapi_gpio_set_dir(5, GPIO_DIRECTION_INPUT);
}


bool touch_is_pressed(void)
{
    // INT引脚低电平表示有触摸 [cite: 154]
    return (uapi_gpio_get_val(TOUCH_INT_GPIO) == GPIO_LEVEL_LOW);
}

bool touch_get_data(touch_data_t *out)
{
    if (out == NULL) return false;

    // 首先检查中断引脚电平，避免不必要的 I2C 轮询
    /*
    if (!touch_is_pressed()) {
        out->pressed = false;
        return true;
    }
    */
    uint8_t td_status = 0;
    // 读 0x02 寄存器获取触摸点数
    if (!ft_read_reg(FT_REG_TD_STATUS, &td_status, 1)) return false;

    uint8_t touch_count = td_status & 0x0F;
    if (touch_count == 0 || touch_count > 2) {
        out->pressed = false;
        return true;
    }

    /* 读取第一点的 XH, XL, YH, YL */
    uint8_t buf[4] = {0};
    if (!ft_read_reg(FT_REG_TOUCH1_XH, buf, 4)) return false;

    /* 手册解析逻辑：XH[3:0]为高4位，XL[7:0]为低8位 */
    uint16_t raw_x = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
    uint16_t raw_y = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];

    /* 根据 ILI9341 横屏方向(320x240)进行坐标映射 */
    // 注意：如果点击左右反了或上下反了，在此处交换 x/y 或使用 (W - 1 - x)
    out->x = raw_x;
    out->y = raw_y;
    out->pressed = true;

    return true;
}

/* =====================================================================
 * 物理层红蓝屏交替测试函数（追加在 touch_ft6336u.c 文件最末尾）
 * ===================================================================== */
void touch_hardware_test_loop(void)
{
    // 内部快捷全屏刷色闭包
    void local_fill_screen(uint16_t color) {
        uint8_t data[2] = {(color >> 8) & 0xFF, color & 0xFF};
        lcd_set_window(0, 0, 319, 239); 
        for (uint32_t i = 0; i < 320 * 240; i++) {
            uapi_spi_master_write(LCD_SPI_BUS, &(spi_xfer_data_t){.tx_buff = data, .tx_bytes = 2}, 1000);
        }
    }

    printf("[TEST] Entering independent Hardware Test Loop...\n");
    
    // 1. 初始状态：刷成纯红色
    uint16_t current_color = 0xF800; 
    local_fill_screen(current_color);
    printf("[TEST] Screen filled with RED. Please touch the screen...\n");

    touch_data_t test_data = {0};
    bool last_pressed = false;

    while (1) {
        if (touch_is_pressed()) {
            // INT 引脚（GPIO5）变低，代表电容屏硬件感知到了触摸
            if (touch_get_data(&test_data)) {
                
                if (test_data.pressed && !last_pressed) {
                    // 按下瞬间翻转颜色
                    current_color = (current_color == 0xF800) ? 0x001F : 0xF800;
                    local_fill_screen(current_color);
                    printf("[TEST] Success! Parsed Coordinates X:%d, Y:%d\n", test_data.x, test_data.y);
                }
                last_pressed = test_data.pressed;
            } else {
                // 如果手按下去屏幕不变色，但是串口疯狂报这一行，说明底层 I2C 硬件不响应
                printf("[TEST] INT Pulled Low, but I2C Read FAILED!\n");
            }
        } else {
            last_pressed = false;
        }
        osDelay(20); // 20ms 周期的硬件扫描
    }
}