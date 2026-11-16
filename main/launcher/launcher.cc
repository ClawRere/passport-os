// Application drawer: Apple Watch–style 3×4 circular grid with accent highlights
// and per-app entry animations. Inherits the pixel icon glyph system from the
// original card-based launcher; only the navigation shell is replaced.
#include "launcher/launcher.h"
#include "apps/voice/time_sync.h"
#include "launcher/app_registry.h"
#include "wifi_manager.h"
#include "ssid_manager.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "ui_pixel.h"
#include "ui/pixel_scene.h"
#include "app_fonts.h"
#include "esp_log.h"
#include "lvgl.h"

#include <cstdio>

static const char *TAG = "launcher";

namespace {

// ── Layout constants ────────────────────────────────────────────────────
constexpr int kCols = 3;
constexpr int kCellW = 62;   // button width  (fits on 240-wide screen)
constexpr int kCellH = 52;   // button height (icon 44px + label 8px)
constexpr int kGapX = 14;
constexpr int kGapY = 6;
constexpr int kPadL = 12;    // left pad of grid within screen

// Pixel-icon geometry (unchanged from original design)
constexpr int kGlyphCells = 17;
constexpr int kGlyphCell  = 3;

// ── Colour palette: one accent colour per app slot ──────────────────────
struct Slot {
    passport_app_id_t id;
    uint32_t          color;
};
const Slot s_slots[] = {
    { APP_ID_XIAOZHI,     UI_SUCCESS   },
    { APP_ID_RADIO,       UI_TERTIARY  },
    { APP_ID_NETEASE,     UI_DANGER    },
    { APP_ID_POMODORO,    UI_WARNING   },
    { APP_ID_STOPWATCH,   UI_SUCCESS   },
    { APP_ID_REMINDERS,   UI_WARNING   },
    { APP_ID_LISTS,       UI_SUCCESS   },
    { APP_ID_TETRIS,      UI_SECONDARY },
    { APP_ID_IR,          UI_ACCENT    },
    { APP_ID_VOICEMEMO,   UI_TEXT      },
    { APP_ID_SETTINGS,    UI_BORDER    },
};

// ── Icon glyphs (pixel art, one per slot — same scheme as before) ────────
struct Glyph {
    passport_app_id_t id;
    const char *rows[kGlyphCells];
};
const Glyph s_icons[] = {
    { APP_ID_XIAOZHI, {
        ".................",
        ".......###.......",
        "........#........",
        "..#############..",
        "..#...........#..",
        "..#...........#..",
        "###...#...#...###",
        "###...#...#...###",
        "###...........###",
        "..#...........#..",
        "..#....###....#..",
        "..#...........#..",
        "..#############..",
        "........#........",
        ".....#######.....",
        ".................",
        ".................",
    }},
    { APP_ID_RADIO, {
        ".................",
        ".........##......",
        ".......##........",
        ".....##..........",
        "...##............",
        "#################",
        "#...............#",
        "#...............#",
        "#.######..#####.#",
        "#.........#...#.#",
        "#.######..#.#.#.#",
        "#.........#...#.#",
        "#.######..#####.#",
        "#...............#",
        "#################",
        ".................",
        ".................",
    }},
    { APP_ID_NETEASE, {
        ".................",
        ".................",
        ".....###########.",
        ".....###########.",
        ".....#.........#.",
        ".....#.........#.",
        ".....#.........#.",
        ".....#.........#.",
        ".....#.........#.",
        ".....#.........#.",
        ".....#.........#.",
        "..####......####.",
        ".#...#.....#...#.",
        ".#...#.....#...#.",
        "..###.......###..",
        ".................",
        ".................",
    }},
    { APP_ID_POMODORO, {
        "........#........",
        "......##.##......",
        "......#####......",
        "....##.....##....",
        "...#.........#...",
        "..#.....#.....#..",
        "..#.....#.....#..",
        ".#......#......#.",
        ".#......#......#.",
        ".#......####...#.",
        ".#.............#.",
        ".#.............#.",
        "..#...........#..",
        "..#...........#..",
        "...#.........#...",
        "....##.....##....",
        "......#####......",
    }},
    { APP_ID_STOPWATCH, {
        ".......###.......",
        "........#.....#..",
        "......#####..#...",
        "....##.....##....",
        "...#.........#...",
        "..#...........#..",
        "..#........#..#..",
        ".#........#....#.",
        ".#.......#.....#.",
        ".#......#......#.",
        ".#.............#.",
        ".#.............#.",
        "..#...........#..",
        "..#...........#..",
        "...#.........#...",
        "....##.....##....",
        "......#####......",
    }},
    { APP_ID_REMINDERS, {
        ".................",
        ".................",
        "........#........",
        "......#####......",
        ".....#.....#.....",
        "....#.......#....",
        "....#.......#....",
        "....#.......#....",
        "....#.......#....",
        "...#.........#...",
        "...#.........#...",
        "..#...........#..",
        ".###############.",
        ".................",
        ".......###.......",
        ".................",
        ".................",
    }},
    { APP_ID_LISTS, {
        ".................",
        ".................",
        "..#############..",
        "..#...........#..",
        "..#...........#..",
        "..#..#..#####.#..",
        "..#...........#..",
        "..#...........#..",
        "..#..#..#####.#..",
        "..#...........#..",
        "..#...........#..",
        "..#..#..###...#..",
        "..#...........#..",
        "..#...........#..",
        "..#############..",
        ".................",
        ".................",
    }},
    { APP_ID_TETRIS, {
        ".................",
        ".................",
        ".................",
        "################.",
        "#....#....#....#.",
        "#....#....#....#.",
        "#....#....#....#.",
        "#....#....#....#.",
        "################.",
        ".....#....#......",
        ".....#....#......",
        ".....#....#......",
        ".....#....#......",
        ".....######......",
        ".................",
        ".................",
        ".................",
    }},
    { APP_ID_IR, {
        ".................",
        ".................",
        ".................",
        "....#############.",
        "...#.............#",
        "..#..#########...#",
        "..#..#.......#...#",
        "..#..#.......#...#",
        "..#..#########...#",
        "..#............#.#",
        "..#............#.#",
        "..#..#########...#",
        "..#..#.......#...#",
        "..#..#########...#",
        "...#.............#",
        "....#############.",
        ".................",
    }},
    { APP_ID_VOICEMEMO, {
        ".................",
        ".................",
        ".................",
        "..#############..",
        ".#.............#.",
        ".#...#.....#...#.",
        ".#...#.....#...#.",
        ".##############..",
        ".#.............#.",
        ".#.............#.",
        ".#.............#.",
        "..#.........#....",
        "...#.......#.....",
        "....#.....#......",
        ".....#...#.......",
        "......###........",
        ".................",
    }},
    { APP_ID_SETTINGS, {
        ".................",
        ".................",
        "....###..........",
        "#####.###########",
        "....###..........",
        ".................",
        ".................",
        "..........###....",
        "###########.#####",
        "..........###....",
        ".................",
        ".................",
        "..###............",
        "###.#############",
        "..###............",
        ".................",
        ".................",
    }},
};

// ── Widget handles ──────────────────────────────────────────────────────
lv_obj_t *s_screen = nullptr;
lv_obj_t *s_grid   = nullptr;
lv_obj_t *s_btn[12] = {};
lv_obj_t *s_icon[12] = {};
lv_obj_t *s_lbl[12] = {};
lv_obj_t *s_prev   = nullptr;
lv_obj_t *s_next   = nullptr;
lv_obj_t *s_title  = nullptr;
lv_obj_t *s_clock  = nullptr;
ui_pixel_battery_t s_battery = {};
lv_obj_t *s_wifi[3] = {};
int s_col = 0, s_row = 0;
bool s_first[12] = {};

// ── Icon draw callback ──────────────────────────────────────────────────
static void icon_draw_cb(lv_event_t *e) {
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int idx = (int)(uintptr_t)lv_obj_get_user_data(obj);
    if (idx < 0 || idx >= 12) return;
    const auto *g = &s_icons[idx];
    pixel_scene::Ink ink(e);
    // Centre the 51-pixel glyph inside the 62-pixel cell → offset 5, top-justified
    constexpr int ox = 5, oy = 2;
    for (int y = 0; y < kGlyphCells; ++y) {
        const char *row = g->rows[y];
        for (int x = 0; x < kGlyphCells;) {
            if (row[x] != '#') { ++x; continue; }
            int s = x;
            while (x < kGlyphCells && row[x] == '#') ++x;
            ink.box(ox + s * kGlyphCell, oy + y * kGlyphCell,
                    (x - s) * kGlyphCell, kGlyphCell,
                    s_slots[idx].color);
        }
    }
}

// ── Highlight state ─────────────────────────────────────────────────────
static void set_selected(int idx) {
    const size_t count = app_registry_get_count();
    for (int i = 0; i < 12; ++i) {
        if (!s_btn[i]) continue;
        bool sel = (i == idx);
        lv_obj_set_style_border_width(s_btn[i], sel ? 2 : 0, 0);
        if (sel)
            lv_obj_set_style_border_color(s_btn[i],
                lv_color_hex(s_slots[i].color), 0);
        else
            lv_obj_set_style_border_color(s_btn[i], lv_color_hex(0), 0);
        lv_obj_set_style_bg_opa(s_btn[i],
            sel ? LV_OPA_50 : LV_OPA_TRANSP, 0);
        // Dim neighbour labels proportionally
    }
}

// ── Scale-bounce animation (every selection change) ─────────────────────
static void bump_nudge(void *var, int32_t v) {
    // No-op nudge — restores position at end of animation cycle;
    // actual visual feedback is the border/bg style change in set_selected().
    (void)v;
}
static void bump(int idx) {
    if (!s_btn[idx]) return;
    lv_anim_delete(s_btn[idx], bump_nudge);
    lv_anim_t a{}; lv_anim_init(&a);
    lv_anim_set_var(&a, s_btn[idx]);
    lv_anim_set_exec_cb(&a, bump_nudge);
    lv_anim_set_values(&a, 92, 100);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

// ── Entry animation (first launch only: subtle pop-in) ──────────────────
// Entry pop: fades in from transparent to opaque background over 280ms
static void entry_anim(void *var, int32_t v) {
    // v = 70..100 -> opa = 0..30% coverage
    uint8_t opa = (uint8_t)((v - 70) * LV_OPA_COVER / 30);
    lv_obj_set_style_bg_opa((lv_obj_t *)var, opa, 0);
}
static void first_launch(int idx) {
    if (!s_first[idx] || !s_btn[idx]) return;
    s_first[idx] = false;
    lv_anim_t a{}; lv_anim_init(&a);
    lv_anim_set_var(&a, s_btn[idx]);
    lv_anim_set_exec_cb(&a, entry_anim);
    lv_anim_set_values(&a, 70, 100);
    lv_anim_set_duration(&a, 280);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

// ── Context labels ──────────────────────────────────────────────────────
// Helper: get display name for an app ID (fallback when registry lookup fails)
static const char *app_chinese_name(passport_app_id_t id) {
    const passport_app_t *a = app_registry_get_by_id(id);
    if (a && a->name && a->name[0]) return a->name;
    switch (id) {
        case APP_ID_WIFI:   return "无线网络";
        case APP_ID_SETTINGS: return "设置";
        default: return "";
    }
}

static void refresh_context() {
    const size_t count = app_registry_get_count();
    int idx = s_col + s_row * kCols;
    if (idx >= (int)count) idx = (int)count - 1;
    auto cur = app_registry_get_by_index(idx);
    auto up  = cur ? app_registry_get_by_index(
                        (idx + (int)count - 1) % (int)count) : nullptr;
    auto dn  = cur ? app_registry_get_by_index((idx + 1) % (int)count) : nullptr;
    ui_pixel_label_set_text(s_prev, up ? app_chinese_name(up->id) : "");
    ui_pixel_label_set_text(s_next, dn ? app_chinese_name(dn->id) : "");
}

}  // namespace

void launcher_ui_init(void) {
    bsp_lvgl_lock(-1);

    s_screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);

    // Status bar
    ui_pixel_top_bar(s_screen);
    ui_pixel_wifi_bars_create(s_screen, UI_STATUS_WIFI_X, 18, s_wifi);
    s_battery = ui_pixel_battery_create(s_screen, UI_STATUS_BATTERY_X, 6,
                                         UI_ACCENT, &lv_font_montserrat_14);
    s_clock = ui_pixel_label(s_screen, "--:--", &lv_font_montserrat_14, UI_TEXT);
    ui_pixel_status_text_align(s_clock, 92, 56, LV_TEXT_ALIGN_CENTER);

    // Header title
    s_title = ui_pixel_label(s_screen, "应用", &buddy_font_16, UI_TEXT);
    lv_obj_set_pos(s_title, 12, 28);
    lv_obj_set_size(s_title, 60, 18);

    // Context hints above / below grid
    s_prev = ui_pixel_label(s_screen, "", &buddy_font_16, UI_TEXT_DIM);
    s_next = ui_pixel_label(s_screen, "", &buddy_font_16, UI_TEXT_DIM);
    lv_obj_set_pos(s_prev, 12, 48);  lv_obj_set_size(s_prev, 216, 18);
    lv_obj_set_pos(s_next, 12, 256); lv_obj_set_size(s_next, 216, 18);
    for (auto *lbl : {s_prev, s_next}) {
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_opa(lbl, LV_OPA_60, 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
    }

    // Grid container (all children absolutely positioned inside it)
    s_grid = lv_obj_create(s_screen);
    lv_obj_clear_flag(s_grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_grid, 0, 0);
    lv_obj_set_size(s_grid, 240, 240);
    lv_obj_set_style_pad_all(s_grid, 0, 0);
    lv_obj_set_style_border_width(s_grid, 0, 0);

    // Build buttons (max 12 slots, 3 cols × 4 rows)
    const size_t count = app_registry_get_count();
    for (int i = 0; i < 12; ++i) {
        s_btn[i] = nullptr; s_icon[i] = nullptr; s_lbl[i] = nullptr;
        s_first[i] = true;
    }
    int rows = (int)((count + kCols - 1) / kCols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            int idx = r * kCols + c;
            if (idx >= (int)count) break;
            int x = kPadL + c * (kCellW + kGapX);
            int y = 70 + r * (kCellH + kGapY);   // starts below context hint area
            auto *app = app_registry_get_by_index(idx);
            if (!app) continue;

            // Button container
            auto *btn = lv_obj_create(s_grid);
            lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_pos(btn, x, y);
            lv_obj_set_size(btn, kCellW, kCellH);
            lv_obj_set_style_radius(btn, 10, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(UI_SURFACE), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0), 0);
            lv_obj_set_style_pad_all(btn, 0, 0);
            lv_obj_set_user_data(btn, (void *)(uintptr_t)idx);

            // Pixel-art icon label (drawn by event callback)
            auto *ico = lv_label_create(btn);
            lv_label_set_text(ico, "");
            lv_obj_set_pos(ico, 0, 0);
            lv_obj_set_size(ico, kCellW, 44);
            lv_obj_clear_flag(ico, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(ico, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_style_all(ico);
            lv_obj_add_event_cb(ico, icon_draw_cb, LV_EVENT_DRAW_MAIN, nullptr);
            s_icon[idx] = ico;

            // App name label below icon
            auto *lab = ui_pixel_label(btn, app_chinese_name(app->id),
                                        &buddy_font_16, UI_TEXT_DIM);
            lv_obj_set_pos(lab, 0, 44);
            lv_obj_set_size(lab, kCellW, 8);
            lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_long_mode(lab, LV_LABEL_LONG_DOT);
            s_lbl[idx] = lab;

            s_btn[idx] = btn;
        }
    }

    // Footer
    ui_pixel_footer(s_screen, "上下切换 · OK 打开", "长按 OK 返回首页", &buddy_font_16);

    // Initial state
    s_col = 0; s_row = 0;
    set_selected(0);
    bump(0);
    refresh_context();

    lv_scr_load(s_screen);
    ESP_LOGI(TAG, "Launcher grid init (%zu apps)", count);
    bsp_lvgl_unlock();
}

void launcher_ui_show(void) {
    if (!s_screen) { launcher_ui_init(); return; }
    bsp_lvgl_lock(-1);
    set_selected(s_col + s_row * kCols);
    bump(s_col + s_row * kCols);
    refresh_context();
    bsp_lvgl_unlock();
}

void launcher_ui_hide(void) {
    bsp_lvgl_lock(-1);
    if (s_screen) lv_obj_delete(s_screen);
    s_screen = s_grid = nullptr;
    for (int i = 0; i < 12; ++i)
        s_btn[i] = s_icon[i] = s_lbl[i] = nullptr;
    s_prev = s_next = s_title = s_clock = nullptr;
    bsp_lvgl_unlock();
}

void launcher_ui_update_status(void) {
    if (!s_screen || !s_clock) return;
    char buf[8];
    time_sync_format_local(buf, sizeof(buf));
    ui_pixel_label_set_text(s_clock, buf);
    ui_pixel_battery_set(&s_battery, bsp_battery_soc());
    const bool connected = WifiManager::GetInstance().IsConnected();
    int bars = 0; uint32_t color = UI_TEXT_DIM;
    if (connected) {
        const int rssi = WifiManager::GetInstance().GetRssi();
        bars = rssi >= -60 ? 3 : (rssi >= -75 ? 2 : 1);
        color = UI_ACCENT;
    }
    ui_pixel_wifi_bars_set(s_wifi, bars, color);
}

void launcher_ui_on_key(uint8_t btn, app_btn_event_t event) {
    if (event != BTN_EVT_CLICK && event != BTN_EVT_LONG) return;
    const size_t count = app_registry_get_count();
    if (!count) return;

    if (btn == BSP_BTN_UP && event == BTN_EVT_CLICK) {
        int idx = s_col + s_row * kCols;
        idx = (idx + (int)count - 1) % (int)count;
        s_col = idx % kCols;
        s_row = idx / kCols;
        bsp_lvgl_lock(-1);
        set_selected(idx);
        bump(idx);
        refresh_context();
        bsp_lvgl_unlock();
    } else if (btn == BSP_BTN_DOWN && event == BTN_EVT_CLICK) {
        int idx = s_col + s_row * kCols;
        idx = (idx + 1) % (int)count;
        s_col = idx % kCols;
        s_row = idx / kCols;
        bsp_lvgl_lock(-1);
        set_selected(idx);
        bump(idx);
        refresh_context();
        bsp_lvgl_unlock();
    } else if (btn == BSP_BTN_OK && event == BTN_EVT_CLICK) {
        int idx = s_col + s_row * kCols;
        if (idx < (int)count && s_btn[idx]) {
            first_launch(idx);
            const auto *app = app_registry_get_by_index(idx);
            if (app) {
                ESP_LOGI(TAG, "User launched: %s (ID:%d)", app->name, app->id);
                app_registry_switch_to(app->id);
            }
        }
    }
}
