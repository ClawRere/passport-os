// Application drawer: single-card travel-instrument style (original layout),
// white background, no entry animations, full-width text labels.
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

static const char *TAG = "launcher";

namespace {

// ── Layout constants ────────────────────────────────────────────────────
constexpr int kIconSize   = 88;   // pixel-art icon display size
constexpr int kTitleY     = 110;  // y-position of app name inside card
constexpr int kCardW      = 216;
constexpr int kCardH      = 146;
constexpr int kCardX      = 12;
constexpr int kCardY      = 84;

// Pixel-icon geometry (unchanged from original design)
constexpr int kGlyphCells = 17;
constexpr int kGlyphCell  = 3;

// ── Widget handles ──────────────────────────────────────────────────────
lv_obj_t *s_screen        = nullptr;
lv_obj_t *s_card          = nullptr;
lv_obj_t *s_previous      = nullptr;
lv_obj_t *s_next          = nullptr;
lv_obj_t *s_app_title     = nullptr;
lv_obj_t *s_page_number   = nullptr;
lv_obj_t *s_icon          = nullptr;
lv_obj_t *s_clock         = nullptr;
ui_pixel_battery_t s_battery = {};
lv_obj_t *s_wifi[3]       = {};

passport_app_id_t s_icon_id    = APP_ID_COUNT;
uint32_t          s_icon_color = UI_TEXT;
size_t            s_selected_index = 0;

// ── Icon glyph definitions (same scheme as original launcher) ───────────
struct Glyph { passport_app_id_t id; const char *rows[kGlyphCells]; };
static const Glyph kIconGlyphs[] = {
    {APP_ID_XIAOZHI, {
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
    {APP_ID_RADIO, {
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
    {APP_ID_NETEASE, {
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
    {APP_ID_POMODORO, {
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
    {APP_ID_STOPWATCH, {
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
    {APP_ID_TETRIS, {
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
    {APP_ID_IR, {
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
    {APP_ID_VOICEMEMO, {
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
    {APP_ID_SETTINGS, {
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

// ── Icon drawing helpers ────────────────────────────────────────────────
static void draw_app_icon_event(lv_event_t *event) {
    const Glyph *glyph = nullptr;
    for (const Glyph &g : kIconGlyphs)
        if (g.id == s_icon_id) { glyph = &g; break; }
    if (!glyph) return;
    pixel_scene::Ink ink(event);
    constexpr int kGlyphOrigin = (kIconSize - kGlyphCells * kGlyphCell) / 2;
    for (int y = 0; y < kGlyphCells; ++y) {
        const char *row = glyph->rows[y];
        for (int x = 0; x < kGlyphCells;) {
            if (row[x] != '#') { ++x; continue; }
            const int start = x;
            while (x < kGlyphCells && row[x] == '#') ++x;
            ink.box(kGlyphOrigin + start * kGlyphCell,
                    kGlyphOrigin + y * kGlyphCell,
                    (x - start) * kGlyphCell, kGlyphCell, s_icon_color);
        }
    }
}

// Chevron arrows painted into neighbour labels (no text dependency).
static void draw_neighbour_chevron(lv_event_t *event) {
    auto *label = static_cast<lv_obj_t *>(lv_event_get_target(event));
    const char *text = lv_label_get_text(label);
    if (!text || !text[0]) return;
    lv_point_t size;
    lv_text_get_size(&size, text, lv_obj_get_style_text_font(label, LV_PART_MAIN),
                     0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const int w = lv_obj_get_width(label);
    const int x = (w - size.x) / 2 - 14;
    const bool up = lv_event_get_user_data(event) != nullptr;
    pixel_scene::Ink ink(event);
    for (int row = 0; row < 4; ++row) {
        const int half = up ? row : 3 - row;
        ink.box(x + 3 - half, 8 + row, 1 + half * 2, 1, UI_BORDER);
    }
}

// ── App name lookup ─────────────────────────────────────────────────────
static const char *app_chinese_name(passport_app_id_t id) {
    const passport_app_t *a = app_registry_get_by_id(id);
    if (a && a->name && a->name[0]) return a->name;
    switch (id) {
        case APP_ID_WIFI:      return "无线网络";
        case APP_ID_SETTINGS:  return "设置";
        default:               return "";
    }
}

// ── Update card content (title, page number, icon, neighbours) ─────────
static void update_card_content() {
    const passport_app_t *app = app_registry_get_by_index(s_selected_index);
    if (!app) return;

    // Page number: e.g. "01 / 09"
    lv_label_set_text_fmt(s_page_number, "%02u / %02u",
                          (unsigned)s_selected_index + 1,
                          (unsigned)app_registry_get_count());

    // App name — large, centred, wraps for long names
    lv_label_set_text(s_app_title, app_chinese_name(app->id));

    // Icon: colour based on active theme
    s_icon_id    = app->id;
    s_icon_color = UI_TEXT;
    if (ui_theme_is_multicolor()) {
        switch (app->id) {
            case APP_ID_RADIO:       case APP_ID_POMODORO: s_icon_color = UI_TERTIARY; break;
            case APP_ID_TETRIS:      s_icon_color = UI_SECONDARY;                   break;
            case APP_ID_XIAOZHI:     case APP_ID_STOPWATCH: s_icon_color = UI_SUCCESS;  break;
            case APP_ID_NETEASE:     s_icon_color = UI_DANGER;                        break;
            case APP_ID_IR:          s_icon_color = UI_ACCENT;                         break;
            case APP_ID_VOICEMEMO:   s_icon_color = UI_TEXT;                           break;
            case APP_ID_SETTINGS:    s_icon_color = UI_BORDER;                         break;
            default: break;
        }
    }
    lv_obj_invalidate(s_icon);

    // Neighbour hints above / below
    const size_t count = app_registry_get_count();
    auto prev_app = app_registry_get_by_index((s_selected_index + count - 1) % count);
    auto next_app = app_registry_get_by_index((s_selected_index + 1) % count);
    ui_pixel_label_set_text(s_previous, prev_app ? app_chinese_name(prev_app->id) : "");
    ui_pixel_label_set_text(s_next,       next_app       ? app_chinese_name(next_app->id)       : "");
}

}  // namespace

void launcher_ui_init(void) {
    bsp_lvgl_lock(-1);

    s_screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    // 纯白底色
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);

    // Status bar
    ui_pixel_top_bar(s_screen);
    ui_pixel_wifi_bars_create(s_screen, UI_STATUS_WIFI_X, 18, s_wifi);
    s_battery = ui_pixel_battery_create(s_screen, UI_STATUS_BATTERY_X, 6,
                                         UI_TEXT, &lv_font_montserrat_14);

    // Title + clock in status bar area
    auto *s_catalogue = ui_pixel_label(s_screen, "应用", &buddy_font_16, UI_TEXT);
    ui_pixel_status_text_align(s_catalogue, 42, 40, LV_TEXT_ALIGN_LEFT);
    s_clock = ui_pixel_label(s_screen, "--:--", &lv_font_montserrat_14, UI_TEXT);
    ui_pixel_status_text_align(s_clock, 92, 56, LV_TEXT_ALIGN_CENTER);

    // Neighbour hints
    s_previous = ui_pixel_label(s_screen, "", &buddy_font_16, UI_TEXT_DIM);
    s_next     = ui_pixel_label(s_screen, "", &buddy_font_16, UI_TEXT_DIM);
    lv_obj_set_pos(s_previous, 12, 50); lv_obj_set_size(s_previous, 216, 22);
    lv_obj_set_pos(s_next,     12, 242); lv_obj_set_size(s_next,     216, 22);
    for (lv_obj_t *p : {s_previous, s_next}) {
        lv_obj_set_style_text_align(p, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_opa(p, LV_OPA_70, 0);
        lv_label_set_long_mode(p, LV_LABEL_LONG_WRAP);
    }
    lv_obj_add_event_cb(s_previous, draw_neighbour_chevron,
                        LV_EVENT_DRAW_MAIN_END, s_previous);
    lv_obj_add_event_cb(s_next, draw_neighbour_chevron,
                        LV_EVENT_DRAW_MAIN_END, nullptr);

    // Card
    s_card = lv_obj_create(s_screen);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_card, kCardX, kCardY);
    lv_obj_set_size(s_card, kCardW, kCardH);
    lv_obj_set_style_radius(s_card, 8, 0);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(0xF5F5F5), 0);  // 浅灰卡片底
    lv_obj_set_style_border_color(s_card, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_width(s_card, 1, 0);
    lv_obj_set_style_pad_all(s_card, 0, 0);

    // Page number (top-right of card)
    s_page_number = ui_pixel_label(s_card, "", &lv_font_montserrat_14, UI_TEXT_DIM);
    lv_obj_set_pos(s_page_number, 140, 6);
    lv_obj_set_size(s_page_number, 64, 18);
    lv_obj_set_style_text_align(s_page_number, LV_TEXT_ALIGN_RIGHT, 0);

    // Pixel icon
    s_icon = lv_obj_create(s_card);
    lv_obj_remove_style_all(s_icon);
    lv_obj_set_size(s_icon, kIconSize, kIconSize);
    lv_obj_set_pos(s_icon, 64, 18);
    lv_obj_remove_flag(s_icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_icon, draw_app_icon_event, LV_EVENT_DRAW_MAIN, nullptr);

    // App title (large, centred, wraps for long Chinese names)
    s_app_title = ui_pixel_label(s_card, "", &ui_font_20, UI_TEXT);
    lv_obj_set_pos(s_app_title, 0, kTitleY);
    lv_obj_set_size(s_app_title, kCardW, 44);
    lv_label_set_long_mode(s_app_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_app_title, LV_TEXT_ALIGN_CENTER, 0);

    // Footer
    ui_pixel_footer(s_screen, "上下切换 · OK 打开", "长按 OK 进小智对话", &buddy_font_16);

    update_card_content();
    launcher_ui_update_status();
    lv_scr_load(s_screen);
    ESP_LOGI(TAG, "Launcher card init (%zu apps)", app_registry_get_count());
    bsp_lvgl_unlock();
}

void launcher_ui_show(void) {
    if (!s_screen) { launcher_ui_init(); return; }
    bsp_lvgl_lock(-1);
    update_card_content();
    launcher_ui_update_status();
    bsp_lvgl_unlock();
}

void launcher_ui_hide(void) {
    bsp_lvgl_lock(-1);
    if (s_screen) lv_obj_delete(s_screen);
    s_screen = s_card = nullptr;
    s_previous = s_next = s_app_title = s_page_number = s_icon = s_clock = nullptr;
    s_battery = {nullptr, nullptr};
    for (auto *bar : s_wifi) bar = nullptr;
    bsp_lvgl_unlock();
}

void launcher_ui_update_status(void) {
    if (!s_screen || !s_clock) return;
    char time_str[8];
    time_sync_format_local(time_str, sizeof(time_str));
    ui_pixel_label_set_text(s_clock, time_str);
    ui_pixel_battery_set(&s_battery, bsp_battery_soc());

    const bool connected = WifiManager::GetInstance().IsConnected();
    int bars = 0;
    uint32_t color = UI_TEXT_DIM;
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
        s_selected_index = (s_selected_index + count - 1) % count;
        bsp_lvgl_lock(-1);
        update_card_content();
        bsp_lvgl_unlock();
    } else if (btn == BSP_BTN_DOWN && event == BTN_EVT_CLICK) {
        s_selected_index = (s_selected_index + 1) % count;
        bsp_lvgl_lock(-1);
        update_card_content();
        bsp_lvgl_unlock();
    } else if (btn == BSP_BTN_OK) {
        const passport_app_t *app = app_registry_get_by_index(s_selected_index);
        if (app) {
            ESP_LOGI(TAG, "User launched: %s (ID:%d)", app->name, app->id);
            app_registry_switch_to(app->id);
        }
    } else if (btn == BSP_BTN_UP && event == BTN_EVT_LONG) {
        // Long UP goes back to home
        const passport_app_t *app = app_registry_get_by_index(s_selected_index);
        if (app) {
            ESP_LOGI(TAG, "User went back to home");
            app_registry_switch_to(APP_ID_HOME);
        }
    }
}
