// ir_nec.c - NEC IR TX via GPIO toggle loop
#include "ir_nec.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ir_nec";
#define IR_TX_GPIO 11
enum { HDR_PULSE_US=9000, HDR_SPACE_US=4500, BIT_PULSE_US=560,
       BIT0_SP_US=560, BIT1_SP_US=1680, TRAIL_US=560 };
static volatile bool s_busy = false;

esp_err_t ir_nec_init(void) {
    gpio_config_t io = { .pin_bit_mask = (gpio_num_t)(1ULL << IR_TX_GPIO),
                         .mode = GPIO_MODE_OUTPUT, .pull_up_en = GPIO_PULLUP_DISABLE };
    esp_err_t e = gpio_config(&io);
    if (e != ESP_OK) return e;
    gpio_set_level(IR_TX_GPIO, 0);
    s_busy = false;
    ESP_LOGI(TAG, "IR init GPIO%d", IR_TX_GPIO);
    return ESP_OK;
}
bool ir_nec_is_tx_busy(void) { return s_busy; }
static void delay_us(uint32_t us) { volatile uint32_t n = us * 4u; while (n--) __asm volatile("nop"); }
static void hi(void)   { gpio_set_level(IR_TX_GPIO, 1); }
static void lo(void)   { gpio_set_level(IR_TX_GPIO, 0); }
static void pulse_us(uint32_t us) { hi(); delay_us(us); lo(); }

esp_err_t ir_nec_send(const ir_nec_frame_t *frame) {
    if (!frame || s_busy) return frame ? ESP_ERR_INVALID_STATE : ESP_ERR_INVALID_ARG;
    s_busy = true;
    pulse_us(HDR_PULSE_US); delay_us(HDR_SPACE_US);
    uint8_t b[4] = { (uint8_t)(frame->address & 0xFF), (uint8_t)((~frame->address) & 0xFF),
                     (uint8_t)(frame->command & 0xFF), (uint8_t)((~frame->command) & 0xFF) };
    for (int byte = 0; byte < 4; byte++)
        for (int bit = 7; bit >= 0; bit--) {
            pulse_us(BIT_PULSE_US);
            delay_us((b[byte] >> bit) & 1 ? BIT1_SP_US : BIT0_SP_US);
        }
    pulse_us(TRAIL_US);
    s_busy = false;
    return ESP_OK;
}
void ir_send_power_toggle(void) {
    ir_nec_frame_t f = { .address = 0x0010, .command = 0x44 };
    ir_nec_send(&f);
}