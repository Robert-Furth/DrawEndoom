#pragma once

#include <stdint.h>

static const int ROW_COUNT = 25;
static const int COL_COUNT = 80;

uint8_t* load_endoom_from_ansifile(const char* filename);
uint8_t* load_endoom_from_wad(const char* wad_path);