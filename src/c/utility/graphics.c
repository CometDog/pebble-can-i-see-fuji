#include "graphics.h"
#include "math.h"

static void draw_sun(GContext *ctx, GPoint pos, int8_t size)
{
    const int8_t ray_length = round(size * 0.15);
    const int8_t angle_ray_length = round(size * 0.12);
    const int8_t sun_box_size = round(size * 0.46);
    const int8_t gap = round((size - sun_box_size - (ray_length * 2)) / 2);
    const int8_t center_x = pos.x + size / 2;
    const int8_t center_y = pos.y + size / 2;

    graphics_context_set_stroke_color(ctx, SCORE_SUN_COLOR);

    // Sun Box
    GPoint top_left = GPoint(ray_length + gap + pos.x, ray_length + gap + pos.y);
    GPoint top_right = GPoint(ray_length + gap + pos.x + sun_box_size, ray_length + gap + pos.y);
    GPoint bottom_left = GPoint(ray_length + gap + pos.x, ray_length + gap + pos.y + sun_box_size);
    GPoint bottom_right = GPoint(ray_length + gap + pos.x + sun_box_size, ray_length + gap + pos.y + sun_box_size);

    graphics_draw_line(ctx, top_left, top_right);       // Top
    graphics_draw_line(ctx, top_right, bottom_right);   // Right
    graphics_draw_line(ctx, bottom_right, bottom_left); // Bottom
    graphics_draw_line(ctx, bottom_left, top_left);     // Left

    // Define ray positions
    const struct
    {
        GPoint start;
        GPoint end;
    } rays[] = {
        // Straight rays
        {GPoint(center_x, pos.y), GPoint(center_x, pos.y + ray_length)},               // Top
        {GPoint(center_x, pos.y + size - ray_length), GPoint(center_x, pos.y + size)}, // Bottom
        {GPoint(pos.x, center_y), GPoint(pos.x + ray_length, center_y)},               // Left
        {GPoint(pos.x + size - ray_length, center_y), GPoint(pos.x + size, center_y)}, // Right

        // Diagonal rays
        {GPoint(pos.x + gap, pos.y + gap),
         GPoint(pos.x + gap + angle_ray_length, pos.y + gap + angle_ray_length)}, // Top-left
        {GPoint(pos.x + size - gap, pos.y + gap),
         GPoint(pos.x + size - gap - angle_ray_length, pos.y + gap + angle_ray_length)}, // Top-right
        {GPoint(pos.x + gap, pos.y + size - gap),
         GPoint(pos.x + gap + angle_ray_length, pos.y + size - gap - angle_ray_length)}, // Bottom-left
        {GPoint(pos.x + size - gap, pos.y + size - gap),
         GPoint(pos.x + size - gap - angle_ray_length, pos.y + size - gap - angle_ray_length)} // Bottom-right
    };

    // Draw each ray
    for (size_t i = 0; i < sizeof(rays) / sizeof(rays[0]); i++)
    {
        graphics_draw_line(ctx, rays[i].start, rays[i].end);
    }
}

static void draw_cloud(GContext *ctx, GPoint pos, int8_t size, bool fill)
{
    const int8_t center_x = pos.x + (size / 2);
    const int8_t top_y = pos.y + 16;

    graphics_context_set_stroke_color(ctx, SCORE_CLOUD_COLOR);

    // Define line segments as relative movements {dx, dy, length}
    const struct
    {
        int8_t dx;
        int8_t dy;
        float scale;
    } segments[] = {
        {-1, 0, 0.19},  // half 1 upper top
        {-1, 1, 0.11},  // half 1 upper top left
        {0, 1, 0.13},   // half 1 upper left
        {-1, 0, 0.12},  // half 1 lower top
        {-1, 1, 0.11},  // half 1 lower top left
        {0, 1, 0.23},   // half 1 lower left
        {1, 1, 0.11},   // half 1 lower bottom left
        {1, 0, 0.40},   // half 1 lower bottom
        {1, 0, 0.41},   // half 2 lower bottom
        {1, -1, 0.11},  // half 2 lower bottom right
        {0, -1, 0.23},  // half 2 lower right
        {-1, -1, 0.11}, // half 2 lower top right
        {-1, 0, 0.18},  // half 2 lower top
        {0, -1, 0.13},  // half 2 upper right
        {-1, -1, 0.11}, // half 2 upper top right
        {-1, 0, 0.11},  // half 2 upper top
    };

    // Create path for filling
    if (fill)
    {
        GPath *path = gpath_create(&(GPathInfo){
            .num_points = sizeof(segments) / sizeof(segments[0]) + 1,
            .points = malloc(sizeof(GPoint) * (sizeof(segments) / sizeof(segments[0]) + 1))});

        // First point
        path->points[0] = GPoint(center_x, top_y);
        GPoint current = path->points[0];

        // Generate path points
        for (size_t i = 0; i < sizeof(segments) / sizeof(segments[0]); i++)
        {
            int8_t length = round(size * segments[i].scale);
            current.x += segments[i].dx * length;
            current.y += segments[i].dy * length;
            path->points[i + 1] = current;
        }

        gpath_draw_filled(ctx, path);
        free(path->points);
        gpath_destroy(path);
    }

    // Draw outline
    GPoint current = GPoint(center_x, top_y);
    for (size_t i = 0; i < sizeof(segments) / sizeof(segments[0]); i++)
    {
        int8_t length = round(size * segments[i].scale);
        GPoint next = GPoint(
            current.x + (segments[i].dx * length),
            current.y + (segments[i].dy * length));
        graphics_draw_line(ctx, current, next);
        current = next;
    }
}

static void draw_partly_cloudy(GContext *ctx, GPoint pos, int8_t size)
{
    draw_sun(ctx, pos, size);
    draw_cloud(ctx, GPoint(pos.x, pos.y + size * 0.07), size * 0.66, true);
}

static void draw_mostly_cloudy(GContext *ctx, GPoint pos, int8_t size)
{
    draw_sun(ctx, GPoint((size - size * 0.66) + pos.x, pos.y), size * 0.66);
    draw_cloud(ctx, GPoint(pos.x, pos.y - (size * 0.14)), size, true);
}

static void draw_very_cloudy(GContext *ctx, GPoint pos, int8_t size)
{
    int8_t adjustment_y = -round(pos.x * 0.1);
    int8_t adjustment_size = round(size * 0.1);
    draw_cloud(ctx, GPoint(pos.x + adjustment_size * 3, pos.y + adjustment_y - adjustment_size * 2), size - adjustment_size, true);
    draw_cloud(ctx, GPoint(pos.x, pos.y + adjustment_y), size - adjustment_size, true);
}

void draw_score_image(int8_t score, GContext *ctx, GPoint pos, int8_t size)
{
    if (score >= 8)
        draw_sun(ctx, pos, size);
    else if (score >= 6)
        draw_partly_cloudy(ctx, pos, size);
    else if (score >= 3)
        draw_mostly_cloudy(ctx, pos, size);
    else
        draw_very_cloudy(ctx, pos, size);
}
