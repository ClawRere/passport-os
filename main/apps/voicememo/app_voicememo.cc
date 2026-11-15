#include "apps/app_base.h"
#include "launcher/app_registry.h"
#include "bsp_display.h"
#include "bsp_audio.h"
#include "ui_pixel.h"
#include "app_fonts.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "lvgl.h"
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

static const char *TAG = "voicememo";
static constexpr size_t K_BUF = 512, K_MAX = 128000, K_DIR_LEN = 32, K_MAX_N = 20;
enum State { S_IDLE, S_RECORDING };
static State s_state = S_IDLE;
static FILE *s_fp = nullptr;
static uint8_t s_buf[K_BUF];
static lv_obj_t *s_screen, *s_status, *s_list;
static int s_total = 0, s_cur = 0, s_nums[K_MAX_N];
static char s_dir[K_DIR_LEN];

static void scan_files(void) {
    s_total = 0;
    DIR *d = opendir(s_dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) && s_total < K_MAX_N) {
        int n = atoi(ent->d_name);
        if (n > 0 && n < 1000 && strlen(ent->d_name) == 7 && strcmp(ent->d_name + 3, ".pcm") == 0)
            s_nums[s_total++] = n;
    }
    closedir(d);
    for (int i = 0; i < s_total - 1; i++)
        for (int j = i + 1; j < s_total; j++)
            if (s_nums[j] > s_nums[i]) { int t = s_nums[i]; s_nums[i] = s_nums[j]; s_nums[j] = t; }
}

static void update_list(void) {
    char label[40];
    if (s_cur >= 0 && s_cur < s_total) snprintf(label, sizeof(label), "#%d/%d", s_cur + 1, s_total);
    else snprintf(label, sizeof(label), "暂无备忘");
    if (s_list) lv_label_set_text(s_list, label);
}

static void init(void) {
    strcpy(s_dir, "/memos");
    mkdir(s_dir, 0755);
    bsp_audio_init_playback();
    scan_files(); s_cur = 0;
}

static void start(void) {
    if (!bsp_lvgl_lock(1000)) return;
    if (s_screen) { bsp_lvgl_unlock(); return; }
    s_screen = lv_obj_create(nullptr);
    ui_pixel_background(s_screen);
    ui_pixel_label(ui_pixel_top_bar(s_screen), "语音备忘", &buddy_font_16, UI_TEXT);
    s_status = lv_label_create(s_screen);
    lv_label_set_text(s_status, s_total > 0 ? "按OK播放" : "按OK开始录音");
    lv_obj_set_style_text_font(s_status, &buddy_font_16, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(UI_ACCENT), 0);
    lv_obj_set_pos(s_status, 12, 50);
    lv_obj_set_size(s_status, 216, 24);
    s_list = lv_label_create(s_screen);
    lv_obj_set_style_text_font(s_list, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_list, lv_color_hex(UI_TEXT_DIM), 0);
    lv_obj_set_pos(s_list, 12, 82);
    lv_obj_set_size(s_list, 216, 28);
    static const char *hint[] = { "OK: 录音/停止/播放", "上键: 上一条", "下键: 下一条" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *h = lv_label_create(s_screen);
        lv_label_set_text(h, hint[i]);
        lv_obj_set_style_text_font(h, &buddy_font_16, 0);
        lv_obj_set_style_text_color(h, lv_color_hex(UI_TEXT_DIM), 0);
        lv_obj_set_pos(h, 12, 130 + i * 26);
        lv_obj_set_size(h, 216, 22);
    }
    ui_pixel_footer(s_screen, "", "长按确定返回", &buddy_font_16);
    update_list();
    lv_screen_load(s_screen);
    bsp_lvgl_unlock();
}

static void stop(void) {
    if (!bsp_lvgl_lock(-1)) return;
    if (s_state == S_RECORDING && s_fp) { fclose(s_fp); s_fp = nullptr; s_state = S_IDLE; }
    if (s_screen) { lv_obj_delete(s_screen); s_screen = nullptr; }
    bsp_lvgl_unlock();
}

static void on_key(uint8_t btn, app_btn_event_t event, uint16_t mv) {
    (void)mv;
    if (event != BTN_EVT_CLICK || !bsp_lvgl_lock(500)) return;
    if (!s_screen) { bsp_lvgl_unlock(); return; }
    if (btn == BSP_BTN_OK) {
        if (s_state == S_IDLE) {
            if (s_total > 0) {
                // Play selected memo
                char path[64]; snprintf(path, sizeof(path), "%s/%03d.pcm", s_dir, s_nums[s_cur]);
                FILE *f = fopen(path, "rb");
                if (f) {
                    s_state = S_RECORDING; // reuse as PLAYING flag
                    lv_label_set_text(s_status, "正在播放...");
                    while (true) { size_t n = fread(s_buf, 1, K_BUF, f); if (!n) break; bsp_audio_write(s_buf, n); }
                    fclose(f); s_state = S_IDLE; lv_label_set_text(s_status, "完成 · 再播按OK");
                }
            } else {
                // Start recording
                mkdir(s_dir, 0755);
                int next = s_total > 0 ? s_nums[0] + 1 : 1;
                char path[64]; snprintf(path, sizeof(path), "%s/%03d.pcm", s_dir, next);
                s_fp = fopen(path, "wb");
                if (s_fp) { s_state = S_RECORDING; lv_label_set_text(s_status, "录音中...按OK停止"); ESP_LOGI(TAG, "Recording -> %s", path); }
                else lv_label_set_text(s_status, "存储失败");
            }
        } else if (s_state == S_RECORDING) {
            if (s_fp) { fclose(s_fp); s_fp = nullptr; }
            s_state = S_IDLE; scan_files(); s_cur = 0; update_list();
            lv_label_set_text_fmt(s_status, "已保存 · 共%d条，再按OK播放", s_total);
        }
    } else if (btn == BSP_BTN_UP && s_cur > 0) { s_cur--; update_list(); }
    else if (btn == BSP_BTN_DOWN && s_cur < s_total - 1) { s_cur++; update_list(); }
    bsp_lvgl_unlock();
}

extern const passport_app_t g_voicememo_app = {
    .id = APP_ID_VOICEMEMO, .name = "语音备忘", .en_name = "VOICEMEMO",
    .desc = "录制与回放\n最长4秒·SPIFFS存储", .tag = "I2S",
    .theme_color = 0xA5315B, .init = init, .start = start, .stop = stop, .on_key = on_key,
};
