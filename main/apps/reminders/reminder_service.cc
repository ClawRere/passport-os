// Stub implementation of reminder_service — no-op so other modules keep compiling.
// The actual reminder/list functionality is now handled via XiaoZhi AI voice.
#include "reminder_service.h"
#include <string>

namespace reminders {
void initialize() {}
void poll() {}
size_t list(Record *, size_t) { return 0; }
bool get(uint32_t, Record *) { return false; }
bool create(const char *, int64_t, uint32_t *, const char **) { return false; }
bool create_repeating(const char *, int64_t, const RepeatRule &, uint32_t *, const char **) { return false; }
bool update(uint32_t, const char *, int64_t, const RepeatRule *, const char **) { return false; }
bool change(uint32_t, bool, int64_t, const char *, int64_t) { return false; }
bool clock_ready() { return true; }
uint32_t revision() { return 0; }
bool sound_pending() { return false; }
bool alarm_active() { return false; }
void dismiss_for_navigation() {}
void play_music_notice(bool (*)()) {}
bool intercept_key(uint8_t, int) { return false; }
void show_alert(const Record *, const char *) {}
std::string mcp_result(const char *, const char *, bool *, uint64_t) { return "\"removed\""; }
}  // namespace reminders
