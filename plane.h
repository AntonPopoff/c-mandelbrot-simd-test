#ifndef PLANE_H
#define PLANE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
} ms_vec2i;

typedef struct {
    double x;
    double y;
} ms_vec2d;

typedef struct {
    ms_vec2i screen;
    ms_vec2d offset;
    double scale;
    double zoom;
} ms_plane;

ms_vec2d plane_from_screen_no_offset(const ms_plane *p, double x, double y);

ms_vec2d plane_from_screen(const ms_plane *p, double x, double y);

ms_vec2d plane_to_screen(const ms_plane *p, double x, double y);

void plane_zoom_around(ms_plane *p, int32_t mouse_x, int32_t mouse_y, double wheel);

#endif
