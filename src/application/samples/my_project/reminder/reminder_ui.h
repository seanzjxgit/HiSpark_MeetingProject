#ifndef REMINDER_UI_H
#define REMINDER_UI_H

#include <stdint.h>
#include "lvgl.h"

void reminder_ui_show(const char *title,
                      const char *time_str,
                      uint8_t     level);
void reminder_ui_hide(void);

#endif