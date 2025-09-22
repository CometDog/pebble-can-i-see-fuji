#include "utility.h"
#include "graphics.h"

char *get_score_text(int8_t score)
{
    if (score >= 8)
        return "Visible";
    if (score >= 6)
        return "Partly\nVisible";
    if (score >= 3)
        return "Barely\nVisible";
    return "Not\nVisible";
}

GColor get_score_bubble_color(int8_t score)
{
#ifdef PBL_BW
    return GColorWhite;
#else
    if (score >= 8)
        return SCORE_VISIBLE_BUBBLE_COLOR;
    if (score >= 6)
        return SCORE_PARTLY_VISIBLE_BUBBLE_COLOR;
    if (score >= 3)
        return SCORE_BARELY_VISIBLE_BUBBLE_COLOR;
    return SCORE_NOT_VISIBLE_BUBBLE_COLOR;
#endif
}
