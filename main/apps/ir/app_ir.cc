#include "apps/app_base.h"
#include "ir_nec.h"
#include "launcher/app_registry.h"
#include "bsp_display.h"
#include "ui/ui_pixel.h"
#include "app_fonts.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "app_ir";
static lv_obj_t *s_screen, *s_status;

static void set_text(lv_obj_t **dst, const char *txt) {
    if (*dst) lv_obj_delete(*dst);
    *dst = lv_label_create(s_screen);
    lv_label_set_text(*dst, txt);
    lv_obj_set_style_text_font(*dst, &buddy_font_16, 0);
    lv_obj_set_pos(*dst, 12, 50);
    lv_obj_set_size(*dst, 216, 24);
}

void init(void) { ir_nec_init(); }

void start(void) {
    if (!bsp_lvgl_lock(1000)) return;
    if (s_screen) { bsp_lvgl_unlock(); return; }
    s_screen = lv_obj_create(nullptr);
    ui_pixel_background(s_screen);
    ui_pixel_label(ui_pixel_top_bar(s_screen), "红外遥控", &buddy_font_16, UI_TEXT);
    s_status = nullptr;
    set_text(&s_status, "就绪 · 按OK发信号");
    ui_pixel_footer(s_screen, "", "长按确定返回", &buddy_font_16);
    lv_screen_load(s_screen);
    bsp_lvgl_unlock();
}

void stop(void) {
    if (!bsp_lvgl_lock(-1)) return;
    if (s_screen) { lv_obj_delete(s_screen); s_screen = nullptr; }
    bsp_lvgl_unlock();
}

void on_key(uint8_t btn, app_btn_event_t event, uint16_t mv) {
    (void)mv;
    if (event != BTN_EVT_CLICK || !bsp_lvgl_lock(500)) return;
    if (!s_screen) { bsp_lvgl_unlock(); return; }
    if (btn == BSP_BTN_OK && !ir_nec_is_tx_busy()) {
        ir_send_power_toggle();
        set_text(&s_status, "已发送 · OK再发");
    }
    bsp_lvgl_unlock();
}

extern const passport_app_t g_ir_app = {
    .id = APP_ID_IR, .name = "红外遥控", .en_name = "IR Remote",
    .desc = "发送红外信号\n控制家电", .tag = "GPIO",
    .theme_color = 0x8CF5A6,
    .init = init, .start = start, .stop = stop, .on_key = on_key,
};