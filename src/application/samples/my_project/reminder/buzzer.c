#include "buzzer.h"
#include "gpio.h"
#include "pinctrl.h"
#include "cmsis_os2.h"

void buzzer_init(void)
{
    uapi_pin_set_mode(BUZZER_GPIO, PIN_MODE_0);
    uapi_gpio_set_dir(BUZZER_GPIO, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(BUZZER_GPIO, GPIO_LEVEL_LOW);  /* 默认关闭 */
}

void buzzer_on(void)
{
    uapi_gpio_set_val(BUZZER_GPIO, GPIO_LEVEL_HIGH);
}

void buzzer_off(void)
{
    uapi_gpio_set_val(BUZZER_GPIO, GPIO_LEVEL_LOW);
}

void buzzer_beep(uint32_t on_ms, uint32_t off_ms, uint8_t count)
{
    for(uint8_t i = 0; i < count; i++) {
        buzzer_on();
        osDelay(on_ms);
        buzzer_off();
        if(i < count - 1) osDelay(off_ms);
    }
}