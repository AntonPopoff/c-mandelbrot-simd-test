#ifndef PLANE_H
#define PLANE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
} vec2i;

typedef struct {
    double x;
    double y;
} vec2d;

typedef struct {
    vec2i screen;
    vec2d scale;
    vec2d offset;
    double zoom;
} plane;

vec2d plane_from_screen_no_offset(const plane *p, double x, double y);

vec2d plane_from_screen(const plane *p, double x, double y);

vec2d plane_to_screen(const plane *p, double x, double y);

void plane_zoom_around(plane *p, int64_t mouse_x, int64_t mouse_y, double wheel);

#endif
