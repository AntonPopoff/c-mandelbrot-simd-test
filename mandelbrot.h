#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <immintrin.h>
#include <math.h>
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

#include "plane.h"

#define MAX_ZOOM_LEVEL 1e11

typedef struct {
    Image surface;
    size_t size;
} ms_surface;

typedef enum {
    SCALAR,
    SSE4,
    AVX2,
} ms_impl;

typedef void (*ms_plot)(const ms_plane *, ms_surface *, int32_t);

double zoom_speed(double z);

int32_t zoom_effort(double z);

void ms_surface_init(ms_surface *s, int32_t width, int32_t height);

void ms_surface_free(ms_surface *s);

void ms_plot_scalar(const ms_plane *p, ms_surface *s, int32_t effort);

void ms_plot_avx2(const ms_plane *p, ms_surface *s, int32_t effort);

void ms_plot_sse4(const ms_plane *p, ms_surface *s, int32_t effort);

#endif
