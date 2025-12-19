#include "ui.h"
#include "../utility/graphics.h"
#include "../utility/utility.h"
#include "data.h"
#include <math.h>

static Window *s_loading_window;
static BitmapLayer *s_loading_bitmap_layer;
static GBitmap *s_loading_bitmap;
static TextLayer *s_loading_text_layer;
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
#ifdef PBL_ROUND
    const int16_t inset = PLATFORM_SCALE(22);
    const int16_t bubble_height = PLATFORM_SCALE(57);
    const int16_t base_y = PADDING * 2 + REGION_BUBBLE_HEIGHT;
    const int16_t y = (time == TIME_MORNING) ? base_y : base_y + bubble_height + PADDING;
    return GRect(inset, y, bounds.size.w - inset * 2, bubble_height);
#else
    const int16_t base_y = PADDING * 2 + REGION_BUBBLE_HEIGHT;
    const int16_t y = (time == TIME_MORNING) ? base_y : base_y + TIME_BUBBLE_HEIGHT + PADDING;
    return GRect(PADDING, y, bounds.size.w - PADDING * 2, TIME_BUBBLE_HEIGHT);
#endif
}

static GRect calculate_score_rect(TimePeriod time, GRect bounds, int8_t line_count)
{
    GRect bubble = calculate_bubble_rect(time, bounds);
    int8_t single_line_padding_y = line_count == 1 ? PADDING : 0;
    int16_t inset = PADDING * 2;
    int16_t bubble_height = TIME_BUBBLE_HEIGHT;
#ifdef PBL_ROUND
    inset = PLATFORM_SCALE(22) + PADDING;
    bubble_height = PLATFORM_SCALE(57);
#endif
    return GRect(bounds.size.w / 2 + PADDING,
                 bubble.origin.y + ((bubble_height / 2) - (SCORE_BUBBLE_HEIGHT / 2)) + single_line_padding_y,
                 bounds.size.w / 2 - inset - 10, SCORE_BUBBLE_HEIGHT - (single_line_padding_y * 2));
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
    text_layer_set_text(s_region_layer, get_current_region() == REGION_NORTH ? "North" : "South");
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

    int16_t region_inset = bounds.size.w - PADDING - REGION_BUBBLE_WIDTH;
#ifdef PBL_ROUND
    region_inset = (bounds.size.w - REGION_BUBBLE_WIDTH) / 2;
#endif
    // Region bubble at top right for rectangular displays
    graphics_context_set_fill_color(ctx, REGION_BUBBLE_COLOR);
    graphics_fill_rect(ctx, GRect(region_inset, PADDING, REGION_BUBBLE_WIDTH, REGION_BUBBLE_HEIGHT),
                       CORNER_RADIUS_BUBBLE, GCornersAll);

    // Morning/Afternoon sections
    graphics_context_set_fill_color(ctx, TIME_MORNING_BUBBLE_COLOR);
    graphics_fill_rect(ctx, calculate_bubble_rect(TIME_MORNING, bounds), CORNER_RADIUS_MAIN, GCornersAll);
    graphics_context_set_fill_color(ctx, TIME_AFTERNOON_BUBBLE_COLOR);
    graphics_fill_rect(ctx, calculate_bubble_rect(TIME_AFTERNOON, bounds), CORNER_RADIUS_MAIN, GCornersAll);

    // Score indicators
    draw_score_bubble(ctx, layer, TIME_MORNING);
    draw_score_bubble(ctx, layer, TIME_AFTERNOON);
}

static void morning_score_image_layer_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    GRect bubble_rect = calculate_bubble_rect(TIME_MORNING, bounds);

    graphics_context_set_stroke_width(ctx, DRAWING_STROKE);
    graphics_context_set_fill_color(ctx, TIME_MORNING_BUBBLE_COLOR);
#ifdef PBL_ROUND
    const int16_t icon_size = PLATFORM_SCALE(24);
    draw_score_image(get_current_region_score(TIME_MORNING), ctx,
                     GPoint(bubble_rect.origin.x + PLATFORM_SCALE(15), bubble_rect.origin.y + PADDING), icon_size);
#else
    draw_score_image(get_current_region_score(TIME_MORNING), ctx,
                     GPoint(PLATFORM_SCALE(24), bubble_rect.origin.y + PADDING), DRAWING_SIZE);
#endif
}

static void afternoon_score_image_layer_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    GRect bubble_rect = calculate_bubble_rect(TIME_AFTERNOON, bounds);

    graphics_context_set_stroke_width(ctx, DRAWING_STROKE);
    graphics_context_set_fill_color(ctx, TIME_AFTERNOON_BUBBLE_COLOR);
#ifdef PBL_ROUND
    const int16_t icon_size = PLATFORM_SCALE(24);
    draw_score_image(get_current_region_score(TIME_AFTERNOON), ctx,
                     GPoint(bubble_rect.origin.x + PLATFORM_SCALE(15), bubble_rect.origin.y + PADDING), icon_size);
#else
    draw_score_image(get_current_region_score(TIME_AFTERNOON), ctx,
                     GPoint(PLATFORM_SCALE(24), bubble_rect.origin.y + PADDING), DRAWING_SIZE);
#endif
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

#ifdef PBL_ROUND
    // Date layer at bottom center for round displays
    s_date_layer = text_layer_create(GRect(PADDING, bounds.size.h - REGION_BUBBLE_HEIGHT - PADDING,
                                           bounds.size.w - PADDING * 2, REGION_BUBBLE_HEIGHT));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

    // Region layer at top center for round displays
    int16_t region_x = (bounds.size.w - REGION_BUBBLE_WIDTH) / 2;
    s_region_layer = text_layer_create(GRect(region_x + 4, PADDING, REGION_BUBBLE_WIDTH - 8, REGION_BUBBLE_HEIGHT));
#else
    // Date layer (top left) for rectangular displays
    s_date_layer = text_layer_create(GRect(PADDING, PADDING, bounds.size.w - PADDING * 2, REGION_BUBBLE_HEIGHT));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);

    // Region layer (top right bubble) for rectangular displays
    s_region_layer = text_layer_create(GRect(bounds.size.w - PADDING - REGION_BUBBLE_WIDTH + 4, PADDING,
                                             REGION_BUBBLE_WIDTH - 8, REGION_BUBBLE_HEIGHT));
#endif

    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, DATE_TEXT_COLOR);
    text_layer_set_font(s_date_layer, fonts_get_system_font(DATE_FONT));
    layer_add_child(s_data_layer, text_layer_get_layer(s_date_layer));

    text_layer_set_background_color(s_region_layer, GColorClear);
    text_layer_set_text_color(s_region_layer, REGION_TEXT_COLOR);
    text_layer_set_font(s_region_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text_alignment(s_region_layer, GTextAlignmentCenter);
    layer_add_child(s_data_layer, text_layer_get_layer(s_region_layer));

    // Morning label
#ifdef PBL_ROUND
    GRect morning_bubble = calculate_bubble_rect(TIME_MORNING, bounds);
    s_morning_label_layer =
        text_layer_create(GRect(morning_bubble.origin.x + PADDING, morning_bubble.origin.y + PLATFORM_SCALE(36),
                                bounds.size.w / 2 - PADDING * 3, REGION_BUBBLE_HEIGHT));
#else
    s_morning_label_layer = text_layer_create(
        GRect(round(PADDING * 2.5), calculate_bubble_rect(TIME_MORNING, bounds).origin.y + (REGION_BUBBLE_HEIGHT * 2),
              bounds.size.w / 2 - PADDING * 3, REGION_BUBBLE_HEIGHT));
#endif
    text_layer_set_background_color(s_morning_label_layer, GColorClear);
    text_layer_set_text_color(s_morning_label_layer, TIME_MORNING_TEXT_COLOR);
    text_layer_set_font(s_morning_label_layer, fonts_get_system_font(LABEL_FONT));
    text_layer_set_text(s_morning_label_layer, "Morning");
    layer_add_child(s_data_layer, text_layer_get_layer(s_morning_label_layer));

    // Afternoon label
#ifdef PBL_ROUND
    GRect afternoon_bubble = calculate_bubble_rect(TIME_AFTERNOON, bounds);
    s_afternoon_label_layer =
        text_layer_create(GRect(afternoon_bubble.origin.x + PADDING, afternoon_bubble.origin.y + PLATFORM_SCALE(36),
                                bounds.size.w / 2, REGION_BUBBLE_HEIGHT));
#else
    s_afternoon_label_layer = text_layer_create(
        GRect(PADDING * 2, calculate_bubble_rect(TIME_AFTERNOON, bounds).origin.y + (REGION_BUBBLE_HEIGHT * 2),
              bounds.size.w / 2, REGION_BUBBLE_HEIGHT));
#endif
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
    graphics_fill_rect(ctx, calculate_score_rect(time, bounds, line_count), CORNER_RADIUS_BUBBLE, GCornersAll);
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

static void loading_window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Fuji image
    s_loading_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FUJI_80);
    s_loading_bitmap_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(s_loading_bitmap_layer, s_loading_bitmap);
    bitmap_layer_set_alignment(s_loading_bitmap_layer, GAlignCenter);
    bitmap_layer_set_background_color(s_loading_bitmap_layer, GColorWhite);
    bitmap_layer_set_compositing_mode(s_loading_bitmap_layer, GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_loading_bitmap_layer));

    // Loading text
    s_loading_text_layer =
        text_layer_create(GRect(0, bounds.size.h / 2 + LOADING_TEXT_Y_PADDING, bounds.size.w, LOADING_TEXT_HEIGHT));
    text_layer_set_background_color(s_loading_text_layer, GColorClear);
    text_layer_set_text_color(s_loading_text_layer, GColorBlack);
    text_layer_set_font(s_loading_text_layer, fonts_get_system_font(LOADING_FONT));
    text_layer_set_text_alignment(s_loading_text_layer, GTextAlignmentCenter);
    text_layer_set_text(s_loading_text_layer, "Loading...");
    layer_add_child(window_layer, text_layer_get_layer(s_loading_text_layer));
}

static void loading_window_unload(Window *window)
{
    text_layer_destroy(s_loading_text_layer);
    bitmap_layer_destroy(s_loading_bitmap_layer);
    gbitmap_destroy(s_loading_bitmap);
}

void show_main_window(void)
{
    if (!s_main_window)
    {
        s_main_window = window_create();
        window_set_window_handlers(s_main_window, (WindowHandlers){
                                                      .load = main_window_load,
                                                      .unload = main_window_unload,
                                                  });
        window_set_click_config_provider(s_main_window, click_config_provider);
    }

    window_stack_remove(s_loading_window, false);
    window_stack_push(s_main_window, true);

    window_destroy(s_loading_window);
    s_loading_window = NULL;
}

void ui_init(void)
{
    if (get_data_loaded_progress() == 4)
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
    else
    {
        s_loading_window = window_create();
        window_set_window_handlers(s_loading_window, (WindowHandlers){
                                                         .load = loading_window_load,
                                                         .unload = loading_window_unload,
                                                     });
        window_stack_push(s_loading_window, true);
    }
}

void ui_deinit(void)
{
    if (s_loading_window)
    {
        window_destroy(s_loading_window);
    }
    if (s_main_window)
    {
        window_destroy(s_main_window);
    }
}
