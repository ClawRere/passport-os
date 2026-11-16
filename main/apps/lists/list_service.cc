// Stub implementation of list_service — no-op; lists handled via XiaoZhi AI.
#include "list_service.h"
#include <string>

namespace lists {
void initialize() {}
uint32_t revision() { return 0; }
bool catalog(Summary *, size_t *, const char **) { return false; }
bool read(uint32_t, List *, const char **) { return false; }
bool create(const char *, const char *const *, size_t, uint32_t *, const char **) { return false; }
bool modify(uint32_t, uint32_t, Action, uint32_t, const char *, int, const char *const *, size_t, const char **) { return false; }
size_t tool_count() { return 0; }
std::string call(const char *, const cJSON *) { return "\"removed\""; }
}  // namespace lists