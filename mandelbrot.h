#ifndef MANDELBROT_H
#define MANDELBROT_H

#include "plane.h"
#include <immintrin.h>
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    Color *data;
} mandelbrot;

void mandelbrot_plot_scalar(const plane *plane, mandelbrot *set);

void mandelbrot_plot_avx2(const plane *p, mandelbrot *pixels);

void mandelbrot_plot_sse4(const plane *p, mandelbrot *pixels);

#endif
