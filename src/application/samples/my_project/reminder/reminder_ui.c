#include "reminder_ui.h"
#include "reminder.h"

/* 颜色复用主题色 */
#define COLOR_BG_CARD   lv_color_hex(0xFFFFFF)
#define COLOR_SHADOW    lv_color_hex(0x1A0050)
#define COLOR_WHITE     lv_color_hex(0xFFFFFF)
#define COLOR_LEVEL_HIGH lv_color_hex(0xFF5252)
#define COLOR_LEVEL_MID  lv_color_hex(0xFFD74E)
#define COLOR_LEVEL_LOW  lv_color_hex(0x69FF7A)

static lv_obj_t *g_notification = NULL;  /* 通知容器 */

/* 叉号按钮回调 */
static void close_btn_cb(lv_event_t *e)
{
    (void)e;
    reminder_dismiss();
}

/* =====================================================================
 * 显示iOS风格通知
 * ===================================================================== */
void reminder_ui_show(const char *title,
                      const char *time_str,
                      uint8_t     level)
{
    /* 避免重复创建 */
    if(g_notification != NULL) {
        lv_obj_del(g_notification);
        g_notification = NULL;
    }

    lv_color_t level_color;
    if(level == 0)      level_color = COLOR_LEVEL_HIGH;
    else if(level == 1) level_color = COLOR_LEVEL_MID;
    else                level_color = COLOR_LEVEL_LOW;

    /* ---- 通知主体：圆角矩形卡片，屏幕顶部居中 ---- */
    g_notification = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_notification, 280, 70);
    lv_obj_align(g_notification, LV_ALIGN_TOP_MID, 0, 8);

    /* 玻璃质感样式 */
    lv_obj_set_style_bg_color(g_notification, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(g_notification, 230, 0);   /* ~90%不透明 */
    lv_obj_set_style_radius(g_notification, 16, 0);
    lv_obj_set_style_border_width(g_notification, 0, 0);
    lv_obj_set_style_shadow_color(g_notification, COLOR_SHADOW, 0);
    lv_obj_set_style_shadow_width(g_notification, 20, 0);
    lv_obj_set_style_shadow_opa(g_notification, LV_OPA_40, 0);
    lv_obj_set_style_shadow_ofs_y(g_notification, 4, 0);
    lv_obj_set_style_pad_all(g_notification, 0, 0);
    lv_obj_set_scrollbar_mode(g_notification, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_notification, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- 左侧重要程度色条 ---- */
    lv_obj_t *bar = lv_obj_create(g_notification);
    lv_obj_set_size(bar, 5, 50);
    lv_obj_align(bar, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_bg_color(bar, level_color, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);

    /* ---- 铃铛图标 ---- */
    lv_obj_t *bell = lv_label_create(g_notification);
    lv_label_set_text(bell, LV_SYMBOL_BELL);
    lv_obj_set_style_text_color(bell, level_color, 0);
    lv_obj_set_style_text_font(bell, &lv_font_montserrat_14, 0);
    lv_obj_align(bell, LV_ALIGN_LEFT_MID, 20, 0);
    

    /* ---- 会议名称 ---- */
    lv_obj_t *title_lbl = lv_label_create(g_notification);
    lv_label_set_text(title_lbl, title ? title : "Meeting");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x1A1A1A), 0);
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title_lbl, 160);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 40, 12);

    /* ---- 时间提示 ---- */
    lv_obj_t *time_lbl = lv_label_create(g_notification);
    char buf[32];
    lv_snprintf(buf, sizeof(buf),time_str ? time_str : "");
    lv_label_set_text(time_lbl, buf);
    lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(0x666666), 0);
    lv_obj_align_to(time_lbl, title_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    /* ---- 右侧关闭叉号按钮 ---- */
    lv_obj_t *close_btn = lv_btn_create(g_notification);
    lv_obj_set_size(close_btn, 28, 28);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xC0C0C0),
                               LV_STATE_PRESSED);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(close_lbl);

    lv_obj_add_event_cb(close_btn, close_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 通知置于最顶层，不被其他控件遮挡 */
    lv_obj_move_foreground(g_notification);
}

/* =====================================================================
 * 隐藏通知
 * ===================================================================== */
void reminder_ui_hide(void)
{
    if(g_notification) {
        lv_obj_del(g_notification);
        g_notification = NULL;
    }
}