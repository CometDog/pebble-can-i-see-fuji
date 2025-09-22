#pragma once

#include <pebble.h>

void communication_init(void);
void communication_deinit(void);
bool send_update_all_message(void);
