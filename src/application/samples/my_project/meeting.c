/* meeting.c 顶部改成这样 */
#include "../../open_source/lvgl/src/porting/lv_port_disp.h"
#include "../../open_source/lvgl/src/porting/lv_port_indev.h"
#include "lv_mainstart.h"
#include "lvgl.h"
#include "app_init.h"
#include "cmsis_os2.h"
#include "reminder/reminder.h"
#include "nfc/nfc_checkin.h"

#include "lcd_ili9341.h"  // 全红测试需要加上的

/*
static void lvgl_task(void *arg)
{
    (void)arg;

    lv_init();              // 1. 初始化LVGL 
    lv_port_disp_init();    // 2. 初始化显示驱动
    lv_port_indev_init();   // 3. 初始化触摸驱动 
    lv_mainstart();         // 4. 启动你的UI 

    // reminder蜂鸣器
    // 初始化提醒模块
    // reminder_init();

    // 可选：设置提前提醒分钟数（默认5分钟）
    // reminder_set_advance_minutes(5);

    // nfc_checkin_init();
    while(1) {
        lv_task_handler();
        // reminder_tick();    // ← 每次循环检查  reminder加入
        osDelay(5);
    }
}*/

static void lvgl_task(void *arg)
{
    (void)arg;

    // 1. 调用刚才写的硬件初始化
    lcd_init(); 
    
    // 2. 刷红色测试
    // 如果屏幕驱动、引脚、SPI 都通了，屏幕现在会瞬间变红
    lcd_test_full_red();

    while(1) {
        // 维持现状，不要让 LVGL 的任务跑起来干扰
        lcd_test_full_red();
        osDelay(100);
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