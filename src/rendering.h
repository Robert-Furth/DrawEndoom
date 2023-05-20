#pragma once

#include <stdbool.h>
#include <stdint.h>

bool setup_renderer(const char* window_title, int window_x, int window_y);
void show_endoom(uint8_t* endoom);
void teardown_renderer(void);