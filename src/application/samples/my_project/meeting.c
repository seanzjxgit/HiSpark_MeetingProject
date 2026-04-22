/* meeting.c 顶部改成这样 */
#include "../../open_source/lvgl/src/porting/lv_port_disp.h"
#include "../../open_source/lvgl/src/porting/lv_port_indev.h"
#include "lv_mainstart.h"
#include "lvgl.h"
#include "app_init.h"
#include "cmsis_os2.h"
#include "reminder/reminder.h"

static void lvgl_task(void *arg)
{
    (void)arg;

    lv_init();              /* 1. 初始化LVGL */
    lv_port_disp_init();    /* 2. 初始化显示驱动 */
    lv_port_indev_init();   /* 3. 初始化触摸驱动 */
    lv_mainstart();         /* 4. 启动你的UI */

    /* reminder蜂鸣器*/
    /* 初始化提醒模块 */
    reminder_init();

    /* 可选：设置提前提醒分钟数（默认5分钟）*/
    reminder_set_advance_minutes(5);

    while(1) {
        lv_task_handler();
        reminder_tick();    /* ← 每次循环检查  reminder加入*/
        osDelay(5);
    }
}

static void adc_entry(void)
{
    osThreadAttr_t attr = {
        .name       = "lvgl_task",
        .stack_size = 1024 * 16,  /* 16KB，LCD驱动需要更多栈 */
        .priority   = osPriorityNormal,
    };
    osThreadNew(lvgl_task, NULL, &attr);
}

app_run(adc_entry);