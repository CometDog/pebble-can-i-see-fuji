#include "ui.h"
#include "data.h"
#include "../utility/utility.h"
#include "../utility/graphics.h"
#include "math.h"

static Window *s_main_window;
static TextLayer *s_date_layer;
static TextLayer *s_region_layer;
static TextLayer *s_morning_label_layer;
static TextLayer *s_afternoon_label_layer;
static TextLayer *s_morning_score_layer;
static TextLayer *s_afternoon_score_layer;
static Layer *s_morning_score_image_layer;
static Layer *s_afternoon_score_image_layer;
static Layer *s_canvas_layer;
static Layer *s_data_layer;

static GRect calculate_bubble_rect(TimePeriod time, GRect bounds)
{
    const int16_t base_y = PADDING * 2 + REGION_BUBBLE_HEIGHT;
    const int16_t y = (time == TIME_MORNING) ? base_y : base_y + TIME_BUBBLE_HEIGHT + PADDING;
    return GRect(PADDING, y, bounds.size.w - PADDING * 2, TIME_BUBBLE_HEIGHT);
}

static GRect calculate_score_rect(TimePeriod time, GRect bounds, int8_t line_count)
{
    GRect bubble = calculate_bubble_rect(time, bounds);
    int8_t single_line_padding_y = line_count == 1 ? PADDING : 0;
    return GRect(bounds.size.w / 2 + PADDING,
                 bubble.origin.y + ((TIME_BUBBLE_HEIGHT / 2) - (SCORE_BUBBLE_HEIGHT / 2)) + single_line_padding_y,
                 bounds.size.w / 2 - PADDING * 2 - 10,
                 SCORE_BUBBLE_HEIGHT - (single_line_padding_y * 2));
}

static void update_date()
{
    time_t now = time(NULL) + (9 * 3600);
    struct tm *tick_time = localtime(&now);
    static char date_buffer[16];
    strftime(date_buffer, sizeof(date_buffer), "%a %b %e", tick_time);
    text_layer_set_text(s_date_layer, date_buffer);
}

static void update_region()
{
    text_layer_set_text(s_region_layer,
                        get_current_region() == REGION_NORTH ? "North" : "South");
}

static void region_toggle_click_handler(ClickRecognizerRef recognizer, void *context)
{
    set_current_region((get_current_region() == REGION_NORTH) ? REGION_SOUTH : REGION_NORTH);
    update_all();
}

static void click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_UP, region_toggle_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, region_toggle_click_handler);
}

static void canvas_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);

    // Main background
    graphics_context_set_fill_color(ctx, BACKGROUND_BUBBLE_COLOR);
    graphics_fill_rect(ctx, bounds, CORNER_RADIUS_MAIN, GCornersAll);

    // Region bubble
    graphics_context_set_fill_color(ctx, REGION_BUBBLE_COLOR);
    graphics_fill_rect(ctx,
                       GRect(bounds.size.w - PADDING - REGION_BUBBLE_WIDTH,
                             PADDING,
                             REGION_BUBBLE_WIDTH,
                             REGION_BUBBLE_HEIGHT),
                       CORNER_RADIUS_BUBBLE, GCornersAll);

    // Morning/Afternoon sections
    graphics_context_set_fill_color(ctx, TIME_MORNING_BUBBLE_COLOR);
    graphics_fill_rect(ctx, calculate_bubble_rect(TIME_MORNING, bounds),
                       CORNER_RADIUS_MAIN, GCornersAll);
    graphics_context_set_fill_color(ctx, TIME_AFTERNOON_BUBBLE_COLOR);
    graphics_fill_rect(ctx, calculate_bubble_rect(TIME_AFTERNOON, bounds),
                       CORNER_RADIUS_MAIN, GCornersAll);

    // Score indicators
    draw_score_bubble(ctx, layer, TIME_MORNING);
    draw_score_bubble(ctx, layer, TIME_AFTERNOON);
}

static void morning_score_image_layer_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    GRect bubble_rect = calculate_bubble_rect(TIME_MORNING, bounds);

    graphics_context_set_stroke_width(ctx, 3);
    graphics_context_set_fill_color(ctx, TIME_MORNING_BUBBLE_COLOR);
    draw_score_image(get_current_region_score(TIME_MORNING), ctx,
                     GPoint(PADDING * 4, bubble_rect.origin.y + PADDING),
                     30);
}

static void afternoon_score_image_layer_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    GRect bubble_rect = calculate_bubble_rect(TIME_AFTERNOON, bounds);

    graphics_context_set_stroke_width(ctx, 3);
    graphics_context_set_fill_color(ctx, TIME_AFTERNOON_BUBBLE_COLOR);
    draw_score_image(get_current_region_score(TIME_AFTERNOON), ctx,
                     GPoint(PADDING * 4, bubble_rect.origin.y + PADDING),
                     30);
}

static void main_window_load(Window *window)
{
    window_set_background_color(window, WINDOW_COLOR);
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Create canvas layer for custom drawing
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    // Create data layer for information
    s_data_layer = layer_create(bounds);
    layer_add_child(window_layer, s_data_layer);

    // Date layer (top left)
    s_date_layer = text_layer_create(GRect(PADDING, PADDING,
                                           bounds.size.w - PADDING * 2, 20));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, DATE_TEXT_COLOR);
    text_layer_set_font(s_date_layer, fonts_get_system_font(DATE_FONT));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
    layer_add_child(s_data_layer, text_layer_get_layer(s_date_layer));

    // Region layer (top right bubble)
    s_region_layer = text_layer_create(
        GRect(bounds.size.w - PADDING - REGION_BUBBLE_WIDTH + 4,
              PADDING,
              REGION_BUBBLE_WIDTH - 8,
              REGION_BUBBLE_HEIGHT));
    text_layer_set_background_color(s_region_layer, GColorClear);
    text_layer_set_text_color(s_region_layer, REGION_TEXT_COLOR);
    text_layer_set_font(s_region_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text_alignment(s_region_layer, GTextAlignmentCenter);
    layer_add_child(s_data_layer, text_layer_get_layer(s_region_layer));

    // Morning label
    s_morning_label_layer = text_layer_create(
        GRect(round(PADDING * 2.5),
              calculate_bubble_rect(TIME_MORNING, bounds).origin.y + 40,
              bounds.size.w / 2 - PADDING * 3,
              20));
    text_layer_set_background_color(s_morning_label_layer, GColorClear);
    text_layer_set_text_color(s_morning_label_layer, TIME_MORNING_TEXT_COLOR);
    text_layer_set_font(s_morning_label_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text(s_morning_label_layer, "Morning");
    layer_add_child(s_data_layer, text_layer_get_layer(s_morning_label_layer));

    // Afternoon label
    s_afternoon_label_layer = text_layer_create(
        GRect(PADDING * 2,
              calculate_bubble_rect(TIME_AFTERNOON, bounds).origin.y + 40,
              bounds.size.w / 2,
              20));
    text_layer_set_background_color(s_afternoon_label_layer, GColorClear);
    text_layer_set_text_color(s_afternoon_label_layer, TIME_AFTERNOON_TEXT_COLOR);
    text_layer_set_font(s_afternoon_label_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text(s_afternoon_label_layer, "Afternoon");
    layer_add_child(s_data_layer, text_layer_get_layer(s_afternoon_label_layer));

    // Score layers
    s_morning_score_layer = text_layer_create(calculate_score_rect(TIME_MORNING, bounds, 2));
    text_layer_set_background_color(s_morning_score_layer, GColorClear);
    text_layer_set_text_color(s_morning_score_layer, SCORE_TEXT_COLOR);
    text_layer_set_font(s_morning_score_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text_alignment(s_morning_score_layer, GTextAlignmentCenter);
    layer_add_child(s_data_layer, text_layer_get_layer(s_morning_score_layer));

    s_afternoon_score_layer = text_layer_create(calculate_score_rect(TIME_AFTERNOON, bounds, 2));
    text_layer_set_background_color(s_afternoon_score_layer, GColorClear);
    text_layer_set_text_color(s_afternoon_score_layer, SCORE_TEXT_COLOR);
    text_layer_set_font(s_afternoon_score_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text_alignment(s_afternoon_score_layer, GTextAlignmentCenter);
    layer_add_child(s_data_layer, text_layer_get_layer(s_afternoon_score_layer));

    s_morning_score_image_layer = layer_create(bounds);
    layer_set_update_proc(s_morning_score_image_layer, morning_score_image_layer_update_proc);
    layer_add_child(s_data_layer, s_morning_score_image_layer);

    s_afternoon_score_image_layer = layer_create(bounds);
    layer_set_update_proc(s_afternoon_score_image_layer, afternoon_score_image_layer_update_proc);
    layer_add_child(s_data_layer, s_afternoon_score_image_layer);

    // Initial display update
    update_all();
}

static void main_window_unload(Window *window)
{
    layer_destroy(s_canvas_layer);
    layer_destroy(s_data_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_region_layer);
    text_layer_destroy(s_morning_label_layer);
    text_layer_destroy(s_afternoon_label_layer);
    text_layer_destroy(s_morning_score_layer);
    text_layer_destroy(s_afternoon_score_layer);
    layer_destroy(s_morning_score_image_layer);
    layer_destroy(s_afternoon_score_image_layer);
}

void draw_score_bubble(GContext *ctx, Layer *layer, TimePeriod time)
{
    int8_t score = get_current_region_score(time);
    GRect bounds = layer_get_bounds(layer);
    int8_t line_count = score >= 8 ? 1 : 2;
    graphics_context_set_fill_color(ctx, get_score_bubble_color(score));
    graphics_fill_rect(ctx, calculate_score_rect(time, bounds, line_count),
                       CORNER_RADIUS_BUBBLE, GCornersAll);
}

void update_score(TimePeriod time)
{
    TextLayer *layer = (time == TIME_MORNING) ? s_morning_score_layer : s_afternoon_score_layer;
    GRect bounds = layer_get_bounds((Layer *)s_main_window);
    int8_t score = get_current_region_score(time);
    int8_t line_count = score >= 8 ? 1 : 2;
    text_layer_set_text(layer, get_score_text(score));
    layer_set_frame((Layer *)layer, calculate_score_rect(time, bounds, line_count));
    layer_mark_dirty((time == TIME_MORNING) ? s_morning_score_image_layer : s_afternoon_score_image_layer);
}

void update_all(void)
{
    update_date();
    update_region();
    update_score(TIME_MORNING);
    update_score(TIME_AFTERNOON);
}

void ui_init(void)
{
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers){
                                                  .load = main_window_load,
                                                  .unload = main_window_unload,
                                              });
    window_set_click_config_provider(s_main_window, click_config_provider);
    window_stack_push(s_main_window, true);
    update_all();
}

void ui_deinit(void)
{
    window_destroy(s_main_window);
}
