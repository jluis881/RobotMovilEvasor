#pragma once

#include <stdint.h>
#include <stdbool.h>

void ble_init(void);
void ble_get_last_value(char *out, uint16_t max_len);
bool ble_has_new_value(void);
void ble_clear_new_value_flag(void);

