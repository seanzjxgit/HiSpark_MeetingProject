#ifndef NFC_CHECKIN_H
#define NFC_CHECKIN_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 本机用户ID（烧录时修改）===== */
#define THIS_DEVICE_USER_ID   "000001"   /* ← 每块板子改成不同的ID */

void nfc_checkin_init(void);
void nfc_checkin_task(void);  /* 在主循环调用 */

/* NFC复位引脚 */
#define NFC_RST_GPIO   8    /* GPIO8 PIN32 MODE_0 */

#endif