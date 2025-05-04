#ifndef MANDELBROT_H
#define MANDELBROT_H

#include "plane.h"
#include <immintrin.h>
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    Color *data;
    Image surface;
    size_t size;
} ms_surface;

typedef enum {
    SCALAR,
    SSE4,
    AVX2,
} ms_impl;

void ms_surface_init(ms_surface *s, int32_t width, int32_t height);

void ms_surface_free(ms_surface *s);

void ms_surface_set_alpha(ms_surface *s, size_t offset, uint8_t a);

void ms_plot(const ms_plane *p, ms_surface *s, ms_impl i);

void ms_plot_scalar(const ms_plane *p, ms_surface *s);

void ms_plot_avx2(const ms_plane *p, ms_surface *s);

void ms_plot_sse4(const ms_plane *p, ms_surface *s);

#endif
