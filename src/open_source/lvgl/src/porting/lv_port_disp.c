#if 1   /* ← 改这里 */

#include "lv_port_disp.h"
#include "lcd_ili9341.h"
#include <stdbool.h>

#define MY_DISP_HOR_RES  320
#define MY_DISP_VER_RES  240

/* 显示缓冲区：10行 */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[MY_DISP_HOR_RES * 10];
static lv_color_t buf2[MY_DISP_HOR_RES * 10];

static void disp_flush(lv_disp_drv_t *drv,
                        const lv_area_t *area,
                        lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    lcd_set_window(area->x1, area->y1,
                   area->x2, area->y2);
    lcd_write_pixels((uint16_t *)color_p, w * h);

    lv_disp_flush_ready(drv);
}

void lv_port_disp_init(void)
{
    /* 初始化LCD硬件 */
    lcd_init();

    /* 初始化LVGL显示缓冲区（双缓冲）*/
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2,
                          MY_DISP_HOR_RES * 10);

    /* 注册显示驱动 */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = MY_DISP_HOR_RES;
    disp_drv.ver_res  = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
