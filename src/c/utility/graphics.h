#pragma once

#include <pebble.h>

#ifdef PBL_BW
#define WINDOW_COLOR GColorBlack
#define BACKGROUND_BUBBLE_COLOR GColorWhite
#define DATE_TEXT_COLOR GColorBlack
#define REGION_BUBBLE_COLOR GColorBlack
#define REGION_TEXT_COLOR GColorWhite
#define SCORE_VISIBLE_BUBBLE_COLOR GColorWhite
#define SCORE_PARTLY_VISIBLE_BUBBLE_COLOR GColorWhite
#define SCORE_BARELY_VISIBLE_BUBBLE_COLOR GColorWhite
#define SCORE_NOT_VISIBLE_BUBBLE_COLOR GColorWhite
#define SCORE_TEXT_COLOR GColorBlack
#define TIME_MORNING_BUBBLE_COLOR GColorBlack
#define TIME_AFTERNOON_BUBBLE_COLOR GColorBlack
#define TIME_MORNING_TEXT_COLOR GColorWhite
#define TIME_AFTERNOON_TEXT_COLOR GColorWhite
#define SCORE_SUN_COLOR GColorWhite
#define SCORE_CLOUD_COLOR GColorWhite
#else
#define WINDOW_COLOR GColorOxfordBlue
#define BACKGROUND_BUBBLE_COLOR GColorBabyBlueEyes
#define DATE_TEXT_COLOR GColorWhite
#define REGION_BUBBLE_COLOR GColorVeryLightBlue
#define REGION_TEXT_COLOR GColorWhite
#define SCORE_VISIBLE_BUBBLE_COLOR GColorIslamicGreen
#define SCORE_PARTLY_VISIBLE_BUBBLE_COLOR GColorVividCerulean
#define SCORE_BARELY_VISIBLE_BUBBLE_COLOR GColorRajah
#define SCORE_NOT_VISIBLE_BUBBLE_COLOR GColorRed
#define SCORE_TEXT_COLOR GColorWhite
#define TIME_MORNING_BUBBLE_COLOR GColorSunsetOrange
#define TIME_AFTERNOON_BUBBLE_COLOR GColorLiberty
#define TIME_MORNING_TEXT_COLOR GColorMelon
#define TIME_AFTERNOON_TEXT_COLOR GColorPictonBlue
#define SCORE_SUN_COLOR GColorYellow
#define SCORE_CLOUD_COLOR GColorWhite
#endif

void draw_score_image(int8_t score, GContext *ctx, GPoint pos, int16_t size);