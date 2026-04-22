#ifndef REMINDER_H
#define REMINDER_H

#include <stdint.h>
#include <stdbool.h>
#include "../lv_mainstart.h"  /* 访问 meeting_time/meeting_content */

/* =====================================================================
 * 用户可配置接口：提前多少分钟提醒
 * ===================================================================== */
#define REMINDER_ADVANCE_MIN_DEFAULT   5    /* 默认提前5分钟 */

/* =====================================================================
 * 关闭按键
 * ===================================================================== */
#define REMINDER_BTN_GPIO   4       /* IO4 PIN26 MODE_0 */

/* =====================================================================
 * 提醒状态
 * ===================================================================== */
typedef enum {
    REMINDER_STATE_IDLE = 0,    /* 无提醒 */
    REMINDER_STATE_ACTIVE,      /* 提醒触发中 */
    REMINDER_STATE_DISMISSED,   /* 已关闭 */
} reminder_state_t;

/* =====================================================================
 * 提醒配置结构体
 * ===================================================================== */
typedef struct {
    uint8_t advance_minutes;    /* 提前提醒分钟数，用户可设置 */
} reminder_config_t;

/* =====================================================================
 * 函数声明
 * ===================================================================== */
void reminder_init(void);
void reminder_set_advance_minutes(uint8_t minutes); /* 用户设置接口 */
void reminder_tick(void);   /* 在LVGL任务主循环里调用，定期检查 */
void reminder_dismiss(void);/* 关闭当前提醒 */
reminder_state_t reminder_get_state(void);

/* 解析时间字符串 "HH:MM" → 分钟数 */
uint16_t reminder_parse_time_to_minutes(const char *time_str);

/* 获取当前系统时间（分钟数，00:00=0，23:59=1439）*/
uint16_t reminder_get_current_minutes(void);

#endif