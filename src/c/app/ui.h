#pragma once

#include <pebble.h>
#include "data.h"

#define PADDING 6
#define CORNER_RADIUS_MAIN 14
#define CORNER_RADIUS_BUBBLE 8
#define REGION_BUBBLE_WIDTH 54
#define REGION_BUBBLE_HEIGHT 20
#define SCORE_BUBBLE_WIDTH 48
#define SCORE_BUBBLE_HEIGHT 34
#define LABEL_FONT FONT_KEY_GOTHIC_14_BOLD
#define DATE_FONT FONT_KEY_GOTHIC_14_BOLD
#define TIME_BUBBLE_HEIGHT 62

void ui_init(void);
void ui_deinit(void);
void update_all(void);
void update_score(TimePeriod time);
void draw_score_bubble(GContext *ctx, Layer *layer, TimePeriod time);
