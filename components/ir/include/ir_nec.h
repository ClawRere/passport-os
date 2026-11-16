#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t address;
    uint8_t  command;
} ir_nec_frame_t;

esp_err_t ir_nec_init(void);
bool      ir_nec_is_tx_busy(void);
esp_err_t ir_nec_send(const ir_nec_frame_t *frame);
void      ir_send_power_toggle(void);
void      ir_send_vol_up(void);
void      ir_send_vol_down(void);
void      ir_send_vol_mute(void);
void      ir_send_ch_up(void);
void      ir_send_ch_down(void);

#ifdef __cplusplus
}
#endif
