#if 1

#include "lv_port_indev.h"
#include "../../lvgl.h"
#include "touch_ns2009.h"

static lv_indev_t *indev_touchpad;

/* 触摸读取回调 */
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;

    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    touch_data_t touch = {0};
    touch_get_data(&touch);

    if(touch.pressed) {
        last_x = (lv_coord_t)touch.x;
        last_y = (lv_coord_t)touch.y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    data->point.x = last_x;
    data->point.y = last_y;
}

void lv_port_indev_init(void)
{
    /* 初始化触摸硬件 */
    touch_init();

    /* 注册到LVGL */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}

#else
typedef int keep_pedantic_happy;
#endif