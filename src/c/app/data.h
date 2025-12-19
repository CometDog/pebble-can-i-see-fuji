#pragma once

#include <pebble.h>

typedef enum
{
    REGION_NORTH = 0,
    REGION_SOUTH = 1
} Region;

typedef enum
{
    TIME_MORNING = 0,
    TIME_AFTERNOON = 1
} TimePeriod;

typedef struct
{
    int8_t morning;
    int8_t afternoon;
} RegionScores;

void data_init(void);
void data_deinit(void);
Region get_current_region(void);
void set_region_score(Region region, TimePeriod time, int8_t score);
int8_t get_current_region_score(TimePeriod time);
void set_current_region(Region region);
int get_data_loaded_progress(void);