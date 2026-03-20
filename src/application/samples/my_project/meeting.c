#include "pinctrl.h" //引脚控制
#include "gpio.h"  //GPIO控制
#include "app_init.h" //应用程序初始化
#include "cmsis_os2.h"




static void adc_entry(void)
{
    printf("hello world");
}
 
/* Run the adc_entry. */
app_run(adc_entry);