#include "data.h"

static RegionScores s_region_scores[2] = {
    {.morning = -1, .afternoon = -1}, // North
    {.morning = -1, .afternoon = -1}  // South
};

static Region s_current_region = REGION_NORTH;

Region get_current_region(void)
{
    return s_current_region;
}

int8_t get_current_region_score(TimePeriod time)
{
    if (time == TIME_MORNING)
    {
        return s_region_scores[s_current_region].morning;
    }
    else
    {
        return s_region_scores[s_current_region].afternoon;
    }
}

void set_region_score(Region region, TimePeriod time, int8_t score)
{
    if (time == TIME_MORNING)
    {
        s_region_scores[region].morning = score;
    }
    else
    {
        s_region_scores[region].afternoon = score;
    }
}

void set_current_region(Region region)
{
    s_current_region = region;
}

int get_data_loaded_progress(void)
{
    int progress = 0;
    if (s_region_scores[REGION_NORTH].morning != -1)
        progress++;
    if (s_region_scores[REGION_NORTH].afternoon != -1)
        progress++;
    if (s_region_scores[REGION_SOUTH].morning != -1)
        progress++;
    if (s_region_scores[REGION_SOUTH].afternoon != -1)
        progress++;
    return progress;
}

void data_init(void)
{
}

void data_deinit(void)
{
}