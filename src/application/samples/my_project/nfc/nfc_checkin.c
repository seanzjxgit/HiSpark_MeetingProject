#include "nfc_checkin.h"
#include "../net/http_client.h"
#include "../reminder/buzzer.h"
#include "gpio.h"
#include "pinctrl.h"
#include "cmsis_os2.h"
#include "../lv_mainstart.h"
#include <string.h>
#include <stdio.h>

/* =====================================================================
 * PN532 I2C 基础操作（简化版，仅用于检测卡片）
 * ===================================================================== */
#include "i2c.h"

#define PN532_I2C_BUS      1        /* I2C1，与触摸共用 */
#define PN532_I2C_ADDR     0x24     /* 拨码10对应地址 */

/* PN532命令 */
#define PN532_CMD_SAMCONFIGURATION  0x14
#define PN532_CMD_INLISTPASSIVETARGET 0x4A



/* 发送PN532帧 */
static bool pn532_send_cmd(uint8_t *cmd, uint8_t len)
{
    /* 构造PN532 I2C帧格式 */
    uint8_t frame[32] = {0};
    uint8_t idx = 0;

    frame[idx++] = 0x00;  /* Preamble */
    frame[idx++] = 0x00;  /* Start code 1 */
    frame[idx++] = 0xFF;  /* Start code 2 */
    frame[idx++] = len + 1;          /* LEN */
    frame[idx++] = (uint8_t)(~(len + 1) + 1); /* LCS */
    frame[idx++] = 0xD4;  /* TFI host→PN532 */

    uint8_t dcs = 0xD4;
    for(uint8_t i = 0; i < len; i++) {
        frame[idx++] = cmd[i];
        dcs += cmd[i];
    }
    frame[idx++] = (uint8_t)(~dcs + 1); /* DCS */
    frame[idx++] = 0x00;  /* Postamble */

    i2c_data_t xfer = {0};
    xfer.send_buf    = frame;
    xfer.send_len    = idx;
    xfer.receive_buf = NULL;
    xfer.receive_len = 0;

    return (uapi_i2c_master_write(PN532_I2C_BUS,
                                   PN532_I2C_ADDR,
                                   &xfer) == ERRCODE_SUCC);
}

/* 读取PN532响应 */
static bool pn532_read_response(uint8_t *buf, uint8_t *len)
{
    uint8_t raw[32] = {0};
    i2c_data_t xfer = {0};
    xfer.send_buf    = NULL;
    xfer.send_len    = 0;
    xfer.receive_buf = raw;
    xfer.receive_len = 20;

    if(uapi_i2c_master_read(PN532_I2C_BUS,
                             PN532_I2C_ADDR,
                             &xfer) != ERRCODE_SUCC) {
        return false;
    }

    /* PN532 I2C响应第一字节是状态，0x01=就绪 */
    if(raw[0] != 0x01) return false;

    /* 找响应数据起始位置（跳过Preamble和Start code）*/
    /* raw: [状态][0x00][0x00][0xFF][LEN][LCS][TFI][DATA...][DCS][0x00] */
    if(raw[1] != 0x00 || raw[2] != 0x00 || raw[3] != 0xFF) return false;

    uint8_t data_len = raw[4];
    if(data_len > 20) return false;

    /* 跳过TFI(0xD5)，拷贝数据 */
    *len = data_len - 1;
    for(uint8_t i = 0; i < *len; i++) {
        buf[i] = raw[7 + i];
    }
    return true;
}

/* 初始化PN532 */
static bool pn532_init(void)
{
    /* 复位PN532 */
    uapi_pin_set_mode(NFC_RST_GPIO, PIN_MODE_0);
    uapi_gpio_set_dir(NFC_RST_GPIO, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(NFC_RST_GPIO, GPIO_LEVEL_LOW);
    osDelay(10);
    uapi_gpio_set_val(NFC_RST_GPIO, GPIO_LEVEL_HIGH);
    osDelay(100);

    /* SAMConfiguration：普通模式 */
    uint8_t sam_cmd[] = { PN532_CMD_SAMCONFIGURATION,
                          0x01,  /* Normal mode */
                          0x14,  /* Timeout 20*50ms=1s */
                          0x01   /* IRQ使能 */
                        };
    return pn532_send_cmd(sam_cmd, sizeof(sam_cmd));
}

/* 检测ISO14443A卡片（最常见的NFC卡）*/
static bool pn532_detect_card(void)
{
    uint8_t cmd[] = { PN532_CMD_INLISTPASSIVETARGET,
                      0x01,   /* 最多1张卡 */
                      0x00    /* 106 kbps type A */
                    };
    if(!pn532_send_cmd(cmd, sizeof(cmd))) return false;

    osDelay(20);  /* 等待PN532扫描 */

    uint8_t resp[20] = {0};
    uint8_t resp_len = 0;
    if(!pn532_read_response(resp, &resp_len)) return false;

    /* resp[0]=命令响应码(0x4B)，resp[1]=找到的卡数量 */
    return (resp_len > 1 && resp[1] >= 1);
}

/* =====================================================================
 * 打卡状态机
 * ===================================================================== */
typedef enum {
    NFC_STATE_IDLE = 0,
    NFC_STATE_CARD_DETECTED,
    NFC_STATE_SENDING,
    NFC_STATE_DONE,
    NFC_STATE_COOLDOWN,
} nfc_state_t;

static nfc_state_t g_nfc_state   = NFC_STATE_IDLE;
static uint32_t    g_last_done_ms = 0;
#define COOLDOWN_MS   3000   /* 打卡后3秒冷却，防重复 */

/* =====================================================================
 * 初始化
 * ===================================================================== */
void nfc_checkin_init(void)
{
    if(!pn532_init()) {
        /* PN532初始化失败，可能硬件未连接 */
        g_nfc_state = NFC_STATE_IDLE;
    }
}

/* =====================================================================
 * 主循环调用
 * ===================================================================== */
void nfc_checkin_task(void)
{
    uint32_t now = osKernelGetTickCount();

    switch(g_nfc_state) {

    case NFC_STATE_IDLE:
        /* 每次都尝试检测卡片 */
        if(pn532_detect_card()) {
            g_nfc_state = NFC_STATE_CARD_DETECTED;
        }
        break;

    case NFC_STATE_CARD_DETECTED:
        g_nfc_state = NFC_STATE_SENDING;

        /* 发送打卡请求 */
        {
            checkin_result_t res = {0};
            bool ok = http_checkin(THIS_DEVICE_USER_ID,
                                   home_time,   /* 当前时间从UI变量取 */
                                   &res);

            if(ok) {
                /* 签到：短响两声；签退：长响一声 */
                if(strcmp(res.status, "checkin") == 0) {
                    buzzer_beep(200, 100, 2);
                } else if(strcmp(res.status, "checkout") == 0) {
                    buzzer_beep(500, 0, 1);
                } else {
                    buzzer_beep(100, 50, 3);  /* 异常：快响三声 */
                }
            } else {
                /* 网络失败：长响后短响 */
                buzzer_beep(400, 100, 1);
                buzzer_beep(100, 0,   1);
            }
        }

        g_last_done_ms = now;
        g_nfc_state    = NFC_STATE_COOLDOWN;
        break;

    case NFC_STATE_COOLDOWN:
        if(now - g_last_done_ms >= COOLDOWN_MS) {
            g_nfc_state = NFC_STATE_IDLE;
        }
        break;

    default:
        g_nfc_state = NFC_STATE_IDLE;
        break;
    }
}