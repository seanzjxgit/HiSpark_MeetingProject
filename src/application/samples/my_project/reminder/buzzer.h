#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

/* 蜂鸣器GPIO，根据实际接线修改 */
#define BUZZER_GPIO     0    /* GPIO0 → 蜂鸣器，S8050高电平导通 */

void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);
void buzzer_beep(uint32_t on_ms, uint32_t off_ms, uint8_t count); /* 响n次 */

#endif