#pragma once

#include <stdlib.h>
#include "input.h"
#include "types.h"

typedef struct config {
	u8 keybinds[5];
} Config_State;

Config_State *config_state(void);
void config_key_bind(Input_Key key, const char *key_name);

void config_init();
