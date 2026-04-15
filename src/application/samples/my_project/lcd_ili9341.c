#include "lcd_ili9341.h"
#include "spi.h"
#include "gpio.h"
#include "pinctrl.h"
#include "cmsis_os2.h"

/* =====================================================================
 * 底层GPIO操作
 * ===================================================================== */
static void lcd_dc_cmd(void)
{
    uapi_gpio_set_val(LCD_DC_GPIO, GPIO_LEVEL_LOW);
}

static void lcd_dc_data(void)
{
    uapi_gpio_set_val(LCD_DC_GPIO, GPIO_LEVEL_HIGH);
}

static void lcd_rst_low(void)
{
    uapi_gpio_set_val(LCD_RST_GPIO, GPIO_LEVEL_LOW);
}

static void lcd_rst_high(void)
{
    uapi_gpio_set_val(LCD_RST_GPIO, GPIO_LEVEL_HIGH);
}

/* =====================================================================
 * 底层SPI发送（统一入口）
 * ===================================================================== */
static void lcd_spi_write(uint8_t *buf, uint32_t len)
{
    spi_xfer_data_t xfer = {0};
    xfer.tx_buff  = buf;
    xfer.tx_bytes = len;
    xfer.rx_buff  = NULL;
    xfer.rx_bytes = 0;
    uapi_spi_master_write(LCD_SPI_BUS, &xfer, 100);
}

static void lcd_write_cmd(uint8_t cmd)
{
    lcd_dc_cmd();
    lcd_spi_write(&cmd, 1);
}

static void lcd_write_data8(uint8_t data)
{
    lcd_dc_data();
    lcd_spi_write(&data, 1);
}

static void lcd_write_data16(uint16_t data)
{
    lcd_dc_data();
    uint8_t buf[2];
    buf[0] = (data >> 8) & 0xFF;
    buf[1] =  data & 0xFF;
    lcd_spi_write(buf, 2);
}

/* =====================================================================
 * GPIO初始化
 * ===================================================================== */
static void lcd_gpio_init(void)
{
    /* RST → GPIO14, MODE_0 */
    uapi_pin_set_mode(14, PIN_MODE_0);
    uapi_gpio_set_dir(14, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(14, GPIO_LEVEL_HIGH);

    /* DCX → GPIO13, MODE_0 */
    uapi_pin_set_mode(13, PIN_MODE_0);
    uapi_gpio_set_dir(13, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(13, GPIO_LEVEL_HIGH);

    /* BL → GPIO0, MODE_0 */
    uapi_pin_set_mode(0, PIN_MODE_0);
    uapi_gpio_set_dir(0, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(0, GPIO_LEVEL_LOW);  /* 先关背光 */
}

/* =====================================================================
 * SPI初始化  — 使用 uapi_spi_init + spi_attr_t
 * ===================================================================== */
static void lcd_spi_init(void)
{
    /* SPI引脚复用配置（对照引脚.md）*/
    uapi_pin_set_mode(9,  PIN_MODE_3);  /* GPIO9  → SPI0_OUT(MOSI) MODE_3 ⚠️ */
    uapi_pin_set_mode(7,  PIN_MODE_3);  /* GPIO7  → SPI0_SCK       MODE_3    */
    /* MISO不接，单向写屏 */
    /* CS由SPI硬件控制，暂不配置，或用软件CS */

    spi_attr_t spi_cfg = {0};
    spi_cfg.is_slave         = false;
    spi_cfg.slave_num        = 1;
    spi_cfg.bus_clk          = 40000000;
    spi_cfg.freq_mhz         = 10;
    spi_cfg.clk_polarity     = SPI_CFG_CLK_CPOL_0;
    spi_cfg.clk_phase        = SPI_CFG_CLK_CPHA_0;
    spi_cfg.frame_format     = SPI_CFG_FRAME_FORMAT_MOTOROLA_SPI;
    spi_cfg.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;
    spi_cfg.frame_size       = HAL_SPI_FRAME_SIZE_8;
    spi_cfg.tmod             = HAL_SPI_TRANS_MODE_TX;
    spi_cfg.ndf              = 0;
    spi_cfg.sste             = SPI_CFG_SSTE_DISABLE;

    uapi_spi_init(LCD_SPI_BUS, &spi_cfg, NULL);
}

/* =====================================================================
 * 硬件复位
 * ===================================================================== */
static void lcd_hard_reset(void)
{
    lcd_rst_high();
    osDelay(10);
    lcd_rst_low();
    osDelay(20);
    lcd_rst_high();
    osDelay(150);
}

/* =====================================================================
 * ILI9341 初始化序列
 * ===================================================================== */
void lcd_init(void)
{
    lcd_gpio_init();
    lcd_spi_init();
    lcd_hard_reset();

    lcd_write_cmd(0x01);   /* 软复位 */
    osDelay(100);
    lcd_write_cmd(0x11);   /* 退出睡眠 */
    osDelay(120);

    /* 电源控制 */
    lcd_write_cmd(0xC0);
    lcd_write_data8(0x23);

    lcd_write_cmd(0xC1);
    lcd_write_data8(0x10);

    lcd_write_cmd(0xC5);
    lcd_write_data8(0x3E);
    lcd_write_data8(0x28);

    lcd_write_cmd(0xC7);
    lcd_write_data8(0x86);

    /* 横屏 320x240，BGR颜色顺序 */
    lcd_write_cmd(0x36);
    lcd_write_data8(0x48);

    /* 16bit RGB565 */
    lcd_write_cmd(0x3A);
    lcd_write_data8(0x55);

    /* 帧率 */
    lcd_write_cmd(0xB1);
    lcd_write_data8(0x00);
    lcd_write_data8(0x18);

    /* 显示功能 */
    lcd_write_cmd(0xB6);
    lcd_write_data8(0x08);
    lcd_write_data8(0x82);
    lcd_write_data8(0x27);

    /* Gamma */
    lcd_write_cmd(0x26);
    lcd_write_data8(0x01);

    lcd_write_cmd(0xE0);
    lcd_write_data8(0x0F); lcd_write_data8(0x31);
    lcd_write_data8(0x2B); lcd_write_data8(0x0C);
    lcd_write_data8(0x0E); lcd_write_data8(0x08);
    lcd_write_data8(0x4E); lcd_write_data8(0xF1);
    lcd_write_data8(0x37); lcd_write_data8(0x07);
    lcd_write_data8(0x10); lcd_write_data8(0x03);
    lcd_write_data8(0x0E); lcd_write_data8(0x09);
    lcd_write_data8(0x00);

    lcd_write_cmd(0xE1);
    lcd_write_data8(0x00); lcd_write_data8(0x0E);
    lcd_write_data8(0x14); lcd_write_data8(0x03);
    lcd_write_data8(0x11); lcd_write_data8(0x07);
    lcd_write_data8(0x31); lcd_write_data8(0xC1);
    lcd_write_data8(0x48); lcd_write_data8(0x08);
    lcd_write_data8(0x0F); lcd_write_data8(0x0C);
    lcd_write_data8(0x31); lcd_write_data8(0x36);
    lcd_write_data8(0x0F);

    lcd_write_cmd(0x29);   /* 开显示 */
    osDelay(100);

    lcd_backlight(1);      /* 开背光 */
}

/* =====================================================================
 * 设置显示窗口
 * ===================================================================== */
void lcd_set_window(uint16_t x1, uint16_t y1,
                    uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2A);      /* 列地址 */
    lcd_write_data16(x1);
    lcd_write_data16(x2);

    lcd_write_cmd(0x2B);      /* 行地址 */
    lcd_write_data16(y1);
    lcd_write_data16(y2);

    lcd_write_cmd(0x2C);      /* 写GRAM */
}

/* =====================================================================
 * 批量写像素（LVGL的flush_cb调用）
 * ===================================================================== */
void lcd_write_pixels(uint16_t *data, uint32_t len)
{
    lcd_dc_data();

    /* 每次最多发送一行(320像素)，避免栈溢出 */
    static uint8_t line_buf[320 * 2];
    uint32_t sent = 0;

    while(sent < len) {
        uint32_t batch = len - sent;
        if(batch > 320) batch = 320;

        /* RGB565大端字节序转换 */
        for(uint32_t i = 0; i < batch; i++) {
            line_buf[i * 2]     = (data[sent + i] >> 8) & 0xFF;
            line_buf[i * 2 + 1] =  data[sent + i] & 0xFF;
        }

        spi_xfer_data_t xfer = {0};
        xfer.tx_buff  = line_buf;
        xfer.tx_bytes = batch * 2;
        uapi_spi_master_write(LCD_SPI_BUS, &xfer, 1000);

        sent += batch;
    }
}

/* =====================================================================
 * 背光控制
 * ===================================================================== */
void lcd_backlight(uint8_t on)
{
    uapi_gpio_set_val(LCD_BL_GPIO,
        on ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}