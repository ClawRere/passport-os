#include "apps/homeassistant/ha_client.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "nvs.h"

#include <cstdio>
#include <cstring>

static const char* TAG = "ha_client";
static constexpr char kNvsNamespace[] = "ha";

namespace homeassistant {

namespace {

Config s_config{};
bool s_initialized = false;

esp_err_t http_request(const char* method, const char* url, 
                       const char* body, char* response, size_t resp_len) {
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", 
                                std::string("Bearer ") + s_config.token);
    esp_http_client_set_header(client, "X-HA-access", s_config.token);
    
    if (strcmp(method, "POST") == 0) {
        esp_http_client_set_method(client, "POST");
    } else {
        esp_http_client_set_method(client, "GET");
    }
    
    esp_err_t err = esp_http_client_open(client, body ? strlen(body) : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    
    if (body) {
        int written = esp_http_client_write(client, body, strlen(body));
        if (written < 0) {
            ESP_LOGE(TAG, "HTTP write failed");
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    }
    
    int status = esp_http_client_fetch_headers(client);
    if (status < 0) {
        ESP_LOGE(TAG, "HTTP headers failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    int content_len = esp_http_client_content_length(client);
    if (content_len > 0 && content_len < (int)resp_len) {
        int read_len = esp_http_client_read(client, response, resp_len - 1);
        if (read_len > 0) response[read_len] = '\0';
    }
    
    esp_http_client_cleanup(client);
    return ESP_OK;
}

}  // anonymous namespace

void init() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS not available");
        return;
    }
    
    size_t len = sizeof(s_config.url);
    err = nvs_get_str(handle, "url", s_config.url, &len);
    if (err == ESP_OK) {
        len = sizeof(s_config.token);
        nvs_get_str(handle, "token", s_config.token, &len);
        s_config.enabled = true;
        ESP_LOGI(TAG, "HA configured: %s", s_config.url);
    }
    
    nvs_close(handle);
    s_initialized = true;
}

bool is_configured() {
    return s_initialized && s_config.enabled;
}

std::string call_service(const char* domain, const char* service,
                         const char* entity_id, const char* data_json) {
    if (!is_configured()) return std::string("{"error":"HA not configured"}");
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/services/%s/%s", 
             s_config.url, domain, service);
    
    char body[512];
    if (data_json) {
        snprintf(body, sizeof(body), 
                 "{"entity_id":"%s",%s}", entity_id, data_json);
    } else {
        snprintf(body, sizeof(body), 
                 "{"entity_id":"%s"}", entity_id);
    }
    
    char response[1024];
    esp_err_t err = http_request("POST", url, body, response, sizeof(response));
    if (err != ESP_OK) return std::string("{"error":"HTTP failed"}");
    
    return std::string(response);
}

std::string get_state(const char* entity_id) {
    if (!is_configured()) return std::string("{"error":"HA not configured"}");
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/states/%s", s_config.url, entity_id);
    
    char response[1024];
    esp_err_t err = http_request("GET", url, nullptr, response, sizeof(response));
    if (err != ESP_OK) return std::string("{"error":"HTTP failed"}");
    
    return std::string(response);
}

std::string set_state(const char* entity_id, const char* attrs_json) {
    // Use toggle service for simplicity
    return call_service("input_boolean", "toggle", entity_id, attrs_json);
}

std::string get_all_entities() {
    if (!is_configured()) return std::string("{"error":"HA not configured"}");
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/states", s_config.url);
    
    char response[8192];
    esp_err_t err = http_request("GET", url, nullptr, response, sizeof(response));
    if (err != ESP_OK) return std::string("{"error":"HTTP failed"}");
    
    return std::string(response);
}

}  // namespace homeassistant