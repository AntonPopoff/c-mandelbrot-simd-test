#include <emmintrin.h>
#include <immintrin.h>
#include <math.h>
#include <raylib.h>
#include <stdint.h>

#include "mandelbrot.h"
#include "plane.h"

#define DEFAULT_EFFORT 100
#define EFFORT_SCALE 150

void ms_surface_init(ms_surface *s, int32_t w, int32_t h) {
    s->surface = GenImageColor(w, h, WHITE);
    s->size = w * h;
}

void ms_surface_free(ms_surface *s) {
    UnloadImage(s->surface);
    s->size = -1;
}

void ms_surface_set_alpha(ms_surface *s, size_t offset, uint8_t a) {
    ((Color *)s->surface.data)[offset].a = a;
}

double zoom_speed(double z) {
    double l = log10(z);
    return 2 + 2 * l * l * l * l;
}

int32_t zoom_effort(double z) {
    double l = log(z);
    return (int32_t)(100 + 100 * (l > 0 ? l : 0));
}

void ms_plot_scalar(const ms_plane *p, ms_surface *s, int32_t effort) {
    double transform_x = 1 / p->scale / p->zoom;
    double transform_y = -1 / p->scale / p->zoom;

#pragma omp parallel for schedule(guided)
    for (size_t i = 0; i < s->size; ++i) {
        int32_t x = i % p->screen.x;
        int32_t y = i / p->screen.x;
        double cx = x * transform_x - p->offset.x;
        double cy = y * transform_y - p->offset.y;
        double zx = 0;
        double zy = 0;
        double zabs = 0;
        int32_t step = 0;

        while (step < effort && zabs <= 4) {
            double zx2 = zx * zx;
            double zy2 = zy * zy;
            double zxzy = zx * zy;
            zabs = zx2 + zy2;
            zx = zx2 - zy2 + cx;
            zy = zxzy + zxzy + cy;
            step++;
        }

        uint8_t a = round(255.0 * ((double)(step - 1) / effort));
        ms_surface_set_alpha(s, i, a);
    }
}

#ifdef __AVX2__

void ms_plot_avx2(const ms_plane *p, ms_surface *s, int32_t effort) {
    double transform_x = 1.0 / (p->scale * p->zoom);
    double transform_y = -1.0 / (p->scale * p->zoom);
    __m256d inv_sxz_vec = _mm256_set1_pd(transform_x);
    __m256d inv_syz_vec = _mm256_set1_pd(transform_y);

#pragma omp parallel for schedule(guided)
    for (size_t i = 0; i < s->size; i += 4) {
        int32_t x = i % p->screen.x;
        int32_t y = i / p->screen.x;
        double x_vec[4] __attribute((aligned(32)));
        double y_vec[4] __attribute((aligned(32)));

        x_vec[0] = x;
        x_vec[1] = x + 1;
        x_vec[2] = x + 2;
        x_vec[3] = x + 3;

        y_vec[0] = y;
        y_vec[1] = y;
        y_vec[2] = y;
        y_vec[3] = y;

        __m256d cx = _mm256_load_pd(x_vec);
        __m256d cy = _mm256_load_pd(y_vec);
        cx = _mm256_mul_pd(cx, inv_sxz_vec);
        cx = _mm256_sub_pd(cx, _mm256_set1_pd(p->offset.x));
        cy = _mm256_mul_pd(cy, inv_syz_vec);
        cy = _mm256_sub_pd(cy, _mm256_set1_pd(p->offset.y));

        __m256d zx = _mm256_setzero_pd();
        __m256d zy = _mm256_setzero_pd();
        __m256d iter = _mm256_setzero_pd();

        for (int64_t i = 0; i < effort; ++i) {
            __m256d zx2 = _mm256_mul_pd(zx, zx);
            __m256d zy2 = _mm256_mul_pd(zy, zy);
            __m256d zabs = _mm256_add_pd(zx2, zy2);
            __m256d four = _mm256_set1_pd(4);
            __m256d cmp = _mm256_cmp_pd(zabs, four, _CMP_LE_OQ);
            int32_t escape_mask = _mm256_movemask_pd(cmp);

            if (escape_mask == 0) {
                break;
            }

            __m256d zxzy = _mm256_mul_pd(zx, zy);
            __m256d nzx = _mm256_add_pd(_mm256_sub_pd(zx2, zy2), cx);
            __m256d nzy = _mm256_add_pd(_mm256_add_pd(zxzy, zxzy), cy);
            __m256d one = _mm256_set1_pd(1);
            zx = _mm256_blendv_pd(zx, nzx, cmp);
            zy = _mm256_blendv_pd(zy, nzy, cmp);
            cmp = _mm256_and_pd(cmp, one);
            iter = _mm256_add_pd(iter, cmp);
        }

        iter = _mm256_div_pd(iter, _mm256_set1_pd(effort));
        iter = _mm256_mul_pd(iter, _mm256_set1_pd(255));
        iter = _mm256_round_pd(iter, 0);

        double iter_res[4] __attribute((aligned(32)));
        _mm256_store_pd(iter_res, iter);

        ms_surface_set_alpha(s, i, iter_res[0]);
        ms_surface_set_alpha(s, i + 1, iter_res[1]);
        ms_surface_set_alpha(s, i + 2, iter_res[2]);
        ms_surface_set_alpha(s, i + 3, iter_res[3]);
    }
}

#endif

#ifdef __SSE4_1__

void ms_plot_sse4(const ms_plane *p, ms_surface *s, int32_t effort) {
    double transform_x = 1.0 / (p->scale * p->zoom);
    double transform_y = -1.0 / (p->scale * p->zoom);
    __m128d inv_sxz_vec = _mm_set1_pd(transform_x);
    __m128d inv_syz_vec = _mm_set1_pd(transform_y);
    __m128d one = _mm_set1_pd(1);
    __m128d four = _mm_set1_pd(4);

#pragma omp parallel for schedule(guided)
    for (size_t i = 0; i < s->size; i += 2) {
        int32_t x = i % p->screen.x;
        int32_t y = i / p->screen.x;
        double x_vec[2] __attribute((aligned(16)));
        double y_vec[2] __attribute((aligned(16)));

        x_vec[0] = x;
        x_vec[1] = x + 1;

        y_vec[0] = y;
        y_vec[1] = y;

        __m128d cx = _mm_load_pd(x_vec);
        __m128d cy = _mm_load_pd(y_vec);
        cx = _mm_mul_pd(cx, inv_sxz_vec);
        cx = _mm_sub_pd(cx, _mm_set1_pd(p->offset.x));
        cy = _mm_mul_pd(cy, inv_syz_vec);
        cy = _mm_sub_pd(cy, _mm_set1_pd(p->offset.y));

        __m128d zx = _mm_setzero_pd();
        __m128d zy = _mm_setzero_pd();
        __m128d iter = _mm_setzero_pd();

        for (int64_t i = 0; i < effort; ++i) {
            __m128d zx2 = _mm_mul_pd(zx, zx);
            __m128d zy2 = _mm_mul_pd(zy, zy);
            __m128d zabs = _mm_add_pd(zx2, zy2);
            __m128d cmp = _mm_cmple_pd(zabs, four);
            int32_t escape_mask = _mm_movemask_pd(cmp);

            if (escape_mask == 0) {
                break;
            }

            __m128d zxzy = _mm_mul_pd(zx, zy);
            zx = _mm_add_pd(_mm_sub_pd(zx2, zy2), cx);
            zy = _mm_add_pd(_mm_add_pd(zxzy, zxzy), cy);
            cmp = _mm_and_pd(cmp, one);
            iter = _mm_add_pd(iter, cmp);
        }

        iter = _mm_div_pd(iter, _mm_set1_pd(effort));
        iter = _mm_mul_pd(iter, _mm_set1_pd(255));
        iter = _mm_round_pd(iter, 0);

        double iter_res[2] __attribute((aligned(16)));
        _mm_store_pd(iter_res, iter);

        ms_surface_set_alpha(s, i, iter_res[0]);
        ms_surface_set_alpha(s, i + 1, iter_res[1]);
    }
}

#endif
