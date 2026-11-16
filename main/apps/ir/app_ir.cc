#include "apps/app_base.h"
#include "ir_nec.h"
#include "launcher/app_registry.h"
#include "bsp_display.h"
#include "ui_pixel.h"
#include "app_fonts.h"
#include "esp_log.h"
#include "lvgl.h"

static lv_obj_t *s_screen, *s_status;

// Signal index: 0=power,1=vol+,2=vol-,3=mute,4=ch+,5=ch-
enum { SIG_POWER, SIG_VOL_UP, SIG_VOL_DOWN, SIG_MUTE, SIG_CH_UP, SIG_CH_DOWN, SIG_COUNT };
static int s_sig_idx = SIG_POWER;

static const char *sig_name[] = { "鐢垫簮", "闊抽噺+", "闊抽噺-", "闈欓煶", "棰戦亾+", "棰戦亾-" };
static void (*send_fn[]) (void) = {
    ir_send_power_toggle, ir_send_vol_up, ir_send_vol_down,
    ir_send_vol_mute,     ir_send_ch_up,  ir_send_ch_down,
};

static void set_text(lv_obj_t **dst, const char *txt) {
    if (*dst) lv_obj_delete(*dst);
    *dst = lv_label_create(s_screen);
    lv_label_set_text(*dst, txt);
    lv_obj_set_style_text_font(*dst, &buddy_font_16, 0);
    lv_obj_set_pos(*dst, 12, 50);
    lv_obj_set_size(*dst, 216, 24);
}

static void init(void) { ir_nec_init(); }

static void start(void) {
    if (!bsp_lvgl_lock(1000)) return;
    if (s_screen) { bsp_lvgl_unlock(); return; }
    s_screen = lv_obj_create(nullptr);
    ui_pixel_background(s_screen);
    ui_pixel_label(ui_pixel_top_bar(s_screen), "绾㈠閬ユ帶", &buddy_font_16, UI_TEXT);
    s_status = nullptr;
    set_text(&s_status, "灏辩华 路 鎸塐K鍙戦€佷俊鍙?);
    // Show current signal name as secondary info
    {
        lv_obj_t *info = lv_label_create(s_screen);
        char buf[40];
        snprintf(buf, sizeof(buf), "[%d/%d] %s", s_sig_idx + 1, SIG_COUNT, sig_name[s_sig_idx]);
        lv_label_set_text(info, buf);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(info, lv_color_hex(UI_ACCENT), 0);
        lv_obj_set_pos(info, 12, 80);
        lv_obj_set_size(info, 216, 20);
    }
    static const char *hint[] = { "OK: 鍙戦€佸綋鍓嶄俊鍙?, "涓婇敭: 鍒囨崲淇″彿", "涓嬮敭: 鍒囨崲淇″彿" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *h = lv_label_create(s_screen);
        lv_label_set_text(h, hint[i]);
        lv_obj_set_style_text_font(h, &buddy_font_16, 0);
        lv_obj_set_style_text_color(h, lv_color_hex(UI_TEXT_DIM), 0);
        lv_obj_set_pos(h, 12, 115 + i * 26);
        lv_obj_set_size(h, 216, 22);
    }
    ui_pixel_footer(s_screen, "", "闀挎寜纭畾杩斿洖", &buddy_font_16);
    lv_screen_load(s_screen);
    bsp_lvgl_unlock();
}

static void stop(void) {
    if (!bsp_lvgl_lock(-1)) return;
    if (s_screen) { lv_obj_delete(s_screen); s_screen = nullptr; }
    bsp_lvgl_unlock();
}

static void on_key(uint8_t btn, app_btn_event_t event, uint16_t mv) {
    (void)mv;
    if (event != BTN_EVT_CLICK || !bsp_lvgl_lock(500)) return;
    if (!s_screen) { bsp_lvgl_unlock(); return; }
    if (btn == BSP_BTN_OK && !ir_nec_is_tx_busy()) {
        send_fn[s_sig_idx]();
        char buf[60];
        snprintf(buf, sizeof(buf), "宸插彂閫?%s 路 OK鍐嶅彂", sig_name[s_sig_idx]);
        set_text(&s_status, buf);
    } else if (btn == BSP_BTN_UP && s_sig_idx > 0) {
        --s_sig_idx;
        set_text(&s_status, sig_name[s_sig_idx]);
    } else if (btn == BSP_BTN_DOWN && s_sig_idx < SIG_COUNT - 1) {
        ++s_sig_idx;
        set_text(&s_status, sig_name[s_sig_idx]);
    }
    bsp_lvgl_unlock();
}

extern const passport_app_t g_ir_app = {
    .id = APP_ID_IR, .name = "绾㈠閬ユ帶", .en_name = "IR Remote",
    .desc = "鍙戦€佺孩澶栦俊鍙穃n鎺у埗瀹剁數", .tag = "GPIO",
    .theme_color = 0x8CF5A6,
    .init = init, .start = start, .stop = stop, .on_key = on_key,
};
