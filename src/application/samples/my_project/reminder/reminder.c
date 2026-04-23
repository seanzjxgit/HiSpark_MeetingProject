#include "reminder.h"
#include "buzzer.h"
#include "reminder_ui.h"
#include "gpio.h"
#include "pinctrl.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>

/* =====================================================================
 * 外部数据引用（来自 lv_mainstart.c）
 * ===================================================================== */
extern char     *meeting_time[];
extern char     *meeting_content[];
extern uint8_t   meeting_level[];

/* MESSAGE_MEETING_NUM 在 lv_mainstart.h 里定义 */
#ifndef MESSAGE_MEETING_NUM
#define MESSAGE_MEETING_NUM 5
#endif

/* =====================================================================
 * 内部状态
 * ===================================================================== */
static reminder_config_t  g_cfg = { REMINDER_ADVANCE_MIN_DEFAULT };
static reminder_state_t   g_state = REMINDER_STATE_IDLE;
static bool               g_reminded[MESSAGE_MEETING_NUM] = {0}; /* 防重复提醒 */
static uint32_t           g_last_check_ms = 0;
#define CHECK_INTERVAL_MS  10000   /* 每10秒检查一次 */

/* =====================================================================
 * 按键去抖状态
 * ===================================================================== */
static bool g_btn_last = false;

/* =====================================================================
 * 时间解析："HH:MM" → 分钟数
 * ===================================================================== */
uint16_t reminder_parse_time_to_minutes(const char *time_str)
{
    if(!time_str || strlen(time_str) < 5) return 0;
    uint16_t h = (time_str[0]-'0')*10 + (time_str[1]-'0');
    uint16_t m = (time_str[3]-'0')*10 + (time_str[4]-'0');
    return h * 60 + m;
}

/* =====================================================================
 * 获取当前时间分钟数
 * 实际项目里从RTC或NTP同步时间，这里先用系统tick模拟
 * 替换此函数对接真实时间源
 * ===================================================================== */
uint16_t reminder_get_current_minutes(void)
{
    /* TODO：接入真实RTC
     * 示例：从外部变量 home_time ("HH:MM") 读取
     */
    extern char *home_time;
    return reminder_parse_time_to_minutes(home_time);
}

/* =====================================================================
 * 初始化
 * ===================================================================== */
void reminder_init(void)
{
    buzzer_init();

    /* 关闭按键初始化 */
    uapi_pin_set_mode(REMINDER_BTN_GPIO, PIN_MODE_0);
    uapi_gpio_set_dir(REMINDER_BTN_GPIO, GPIO_DIRECTION_INPUT);

    memset(g_reminded, 0, sizeof(g_reminded));
    g_state = REMINDER_STATE_IDLE;
}

/* =====================================================================
 * 用户设置接口：修改提前提醒分钟数
 * ===================================================================== */
void reminder_set_advance_minutes(uint8_t minutes)
{
    if(minutes > 60) minutes = 60;  /* 最大提前60分钟 */
    if(minutes < 1)  minutes = 1;
    g_cfg.advance_minutes = minutes;
}

/* =====================================================================
 * 关闭提醒
 * ===================================================================== */
void reminder_dismiss(void)
{
    if(g_state == REMINDER_STATE_ACTIVE) {
        buzzer_off();
        reminder_ui_hide();
        g_state = REMINDER_STATE_DISMISSED;
    }
}

reminder_state_t reminder_get_state(void)
{
    return g_state;
}

/* =====================================================================
 * 主循环调用：检查会议时间 + 按键扫描
 * ===================================================================== */
void reminder_tick(void)
{
    uint32_t now_ms = osKernelGetTickCount();

    /* ---- 按键扫描（IO4低电平=按下）---- */
    /* reminder.c 里的按键读取 */
    bool btn_pressed = (uapi_gpio_get_val(REMINDER_BTN_GPIO) == GPIO_LEVEL_LOW);
    if(btn_pressed && !g_btn_last) {
        /* 上升沿：按键刚按下 */
        reminder_dismiss();
    }
    g_btn_last = btn_pressed;

    /* ---- 定时检查会议（每10秒一次）---- */
    if(now_ms - g_last_check_ms < CHECK_INTERVAL_MS) return;
    g_last_check_ms = now_ms;

    /* 如果已有提醒在显示，不触发新的 */
    if(g_state == REMINDER_STATE_ACTIVE) return;

    uint16_t cur_min = reminder_get_current_minutes();

    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM; i++) {
        if(!meeting_time[i] || g_reminded[i]) continue;

        /* 解析会议开始时间（取 "HH:MM" 前5个字符）*/
        uint16_t meet_min = reminder_parse_time_to_minutes(meeting_time[i]);

        /* 计算差值，处理跨天 */
        int16_t diff = (int16_t)meet_min - (int16_t)cur_min;
        if(diff < 0) diff += 1440;  /* 跨天补偿 */

        /* 判断是否在提醒窗口内（0 ~ advance_minutes 分钟）*/
        if(diff >= 0 && diff <= (int16_t)g_cfg.advance_minutes) {
            /* 触发提醒 */
            g_reminded[i] = true;
            g_state = REMINDER_STATE_ACTIVE;

            /* 蜂鸣器响3次 */
            buzzer_beep(300, 200, 3);

            /* 显示LVGL通知 */
            reminder_ui_show(meeting_content[i], meeting_time[i],
                             meeting_level[i]);
            break;  /* 一次只提醒一个 */
        }
    }
}