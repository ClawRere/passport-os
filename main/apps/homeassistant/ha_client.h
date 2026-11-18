#pragma once

#include <cstdint>
#include <string>

namespace homeassistant {

struct Config {
    char url[128] = {};
    char token[256] = {};
    bool enabled = false;
};

void init();
bool is_configured();
std::string call_service(const char* domain, const char* service, 
                         const char* entity_id, const char* data_json);
std::string get_state(const char* entity_id);
std::string set_state(const char* entity_id, const char* attributes_json);
std::string get_all_entities();

constexpr int kRefreshIntervalSec = 30;

}  // namespace homeassistant