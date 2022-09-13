#pragma once

#include "render.h"
#include "config.h"
#include "input.h"
#include "time.h"

typedef struct global {
	Config_State config;
	Input_State input;
	Time_State time;
} Global;

extern Global global;

