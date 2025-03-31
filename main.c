#include "raylib.h"
#include <immintrin.h>
#include <math.h>
#include <mm_malloc.h>
#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define WIDTH 1280
#define HEIGHT 720

typedef struct {
    double x;
    double y;
} vec2d;

typedef struct {
    int64_t x;
    int64_t y;
} vec2i;

typedef struct {
    double r;
    double i;
} complx;

typedef struct {
    vec2i mouse;
    vec2i mouse_dxdy;
    double mouse_wheel_v;
    bool down;
    bool drag;
    bool up;
} input;

typedef struct {
    vec2i screen;
    vec2d scale;
    vec2d offset;
    double zoom;
} plane;

typedef struct {
    double *x;
    double *y;
    int64_t *i;
} mandelbrot_avx_t;

void m_init(mandelbrot_avx_t *m) {
    m->x = _mm_malloc(sizeof(double) * 4, 32);
    m->y = _mm_malloc(sizeof(double) * 4, 32);
    m->i = _mm_malloc(sizeof(int64_t) * 4, 32);
}

void m_free(mandelbrot_avx_t *m) {
    _mm_free(m->x);
    _mm_free(m->y);
    _mm_free(m->i);
}

complx complx_mul(const complx *c1, const complx *c2) {
    return (complx){
        .r = c1->r * c2->r - c1->i * c2->i,
        .i = c1->r * c2->i + c2->r * c1->i,
    };
}

complx complx_add(const complx *c1, const complx *c2) {
    return (complx){
        .r = c1->r + c2->r,
        .i = c1->i + c2->i,
    };
}

double complx_abs(const complx *c) { return sqrt(c->r * c->r + c->i * c->i); }

void plane_init(plane *p, int64_t screen_width, int64_t screen_height, double unit_scale,
                double zoom) {
    p->screen.x = screen_width;
    p->screen.y = screen_height;
    p->scale.x = screen_width / unit_scale;
    p->scale.y = screen_width / unit_scale;
    p->offset.x = (screen_width / p->scale.x) / 2;
    p->offset.y = -(screen_height / p->scale.y) / 2;
    p->zoom = zoom;
}

vec2d plane_to_screen(const plane *plane, double x, double y) {
    return (vec2d){
        .x = ((x + plane->offset.x) * plane->scale.x * plane->zoom),
        .y = -((y + plane->offset.y) * plane->scale.y * plane->zoom),
    };
}

vec2d plane_from_screen_no_offset(const plane *plane, double x, double y) {
    return (vec2d){
        .x = x / plane->scale.x / plane->zoom,
        .y = -y / plane->scale.y / plane->zoom,
    };
}

vec2d plane_from_screen(const plane *plane, double x, double y) {
    return (vec2d){
        .x = (x / plane->scale.x / plane->zoom) - plane->offset.x,
        .y = -(y / plane->scale.y / plane->zoom) - plane->offset.y,
    };
}

void plane_zoom_around(plane *plane, int64_t mouse_x, int64_t mouse_y, double wheel) {
    vec2d mouse_scr = plane_from_screen_no_offset(plane, mouse_x, mouse_y);
    plane->zoom += wheel;
    vec2d new_mouse_scr = plane_from_screen_no_offset(plane, mouse_x, mouse_y);
    plane->offset.x += new_mouse_scr.x - mouse_scr.x;
    plane->offset.y += new_mouse_scr.y - mouse_scr.y;
}

void handle_input(input *input) {
    double mouse_x = GetMouseX();
    double mouse_y = GetMouseY();
    input->mouse_dxdy.x = mouse_x - input->mouse.x;
    input->mouse_dxdy.y = mouse_y - input->mouse.y;
    input->mouse.x = mouse_x;
    input->mouse.y = mouse_y;
    input->mouse_wheel_v = GetMouseWheelMove();
    input->drag = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    input->down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsKeyUp(KEY_LEFT_SHIFT);
    input->up = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsKeyDown(KEY_LEFT_SHIFT);
}

void update_plane(plane *plane, const input *input) {
    if (input->drag == true) {
        vec2d dxdy = plane_from_screen_no_offset(plane, input->mouse_dxdy.x, input->mouse_dxdy.y);
        plane->offset.x += dxdy.x;
        plane->offset.y += dxdy.y;
    }
    if (input->down) {
        plane_zoom_around(plane, input->mouse.x, input->mouse.y, 5);
    }
    if (input->up) {
        plane_zoom_around(plane, input->mouse.x, input->mouse.y, -5);
    }
    if (input->mouse_wheel_v != 0) {
        plane_zoom_around(plane, input->mouse.x, input->mouse.y, input->mouse_wheel_v * 5);
    }
    plane->screen.x = GetScreenWidth();
    plane->screen.y = GetScreenHeight();
}

int64_t mandelbrot_check_point(double x, double y) {
    unsigned int step = 0;
    complx z = {0};
    complx c = {x, y};

    while (step < 200 && complx_abs(&z) <= 2) {
        complx zsqr = complx_mul(&z, &z);
        z = complx_add(&zsqr, &c);
        step++;
    }

    return step;
}

void mandelbrot_plot(const plane *plane, int64_t width, int64_t height, Color *set) {
#pragma omp parallel for
    for (int64_t y = 0; y < height; ++y) {
#pragma omp parallel for
        for (int64_t x = 0; x < width; ++x) {
            vec2d plane_dot = plane_from_screen(plane, x, y);
            int64_t steps = mandelbrot_check_point(plane_dot.x, plane_dot.y);
            set[y * width + x].r = 255;
            set[y * width + x].g = 255;
            set[y * width + x].b = 255;
            set[y * width + x].a = 255 * (steps / 200.0);
        }
    }
}

void mandelbrot_plot_avx(const plane *p, int64_t width, int64_t height, Color *set) {
#pragma omp parallel for
    for (int64_t y = 0; y < height; ++y) {
#pragma omp parallel for
        for (int64_t x = 0; x < width - 4; x += 4) {
            double x_vec[] = {x, x + 1, x + 2, x + 3};
            double y_vec[] = {y, y, y, y};
            __m256d cx = _mm256_loadu_pd(x_vec);
            __m256d cy = _mm256_loadu_pd(y_vec);

            cx = _mm256_div_pd(cx, _mm256_set1_pd(p->scale.x));
            cx = _mm256_div_pd(cx, _mm256_set1_pd(p->zoom));
            cx = _mm256_sub_pd(cx, _mm256_set1_pd(p->offset.x));

            cy = _mm256_mul_pd(cy, _mm256_set1_pd(-1.0));
            cy = _mm256_div_pd(cy, _mm256_set1_pd(p->scale.y));
            cy = _mm256_div_pd(cy, _mm256_set1_pd(p->zoom));
            cy = _mm256_sub_pd(cy, _mm256_set1_pd(p->offset.y));

            __m256d zx = _mm256_setzero_pd();
            __m256d zy = _mm256_setzero_pd();
            __m256i iter = _mm256_set1_epi64x(0);

            for (int64_t i = 0; i < 500; ++i) {
                __m256i one = _mm256_set1_epi64x(1);
                __m256d four = _mm256_set1_pd(4);
                __m256d zx2 = _mm256_mul_pd(zx, zx);
                __m256d zy2 = _mm256_mul_pd(zy, zy);
                __m256d zabs = _mm256_add_pd(zx2, zy2);
                __m256d cmp = _mm256_cmp_pd(zabs, four, _CMP_LE_OQ);
                int32_t mask = _mm256_movemask_pd(cmp);

                if (mask == 0) {
                    break;
                }

                __m256i mask_iter = _mm256_castpd_si256(cmp);
                mask_iter = _mm256_and_si256(mask_iter, one);
                iter = _mm256_add_epi64(iter, mask_iter);

                __m256d zxzy = _mm256_mul_pd(zx, zy);
                zx = _mm256_add_pd(_mm256_sub_pd(zx2, zy2), cx);
                zy = _mm256_add_pd(_mm256_add_pd(zxzy, zxzy), cy);
            }

            int64_t iter_res[4];
            int64_t p = y * width + x;

            _mm256_storeu_si256((__m256i_u *)iter_res, iter);

            set[p].a = 255 * (iter_res[0] / 500.0);
            set[p + 1].a = 255 * (iter_res[1] / 500.0);
            set[p + 2].a = 255 * (iter_res[2] / 500.0);
            set[p + 3].a = 255 * (iter_res[3] / 500.0);
        }
    }
}

void render(const Texture2D *set_texture, const Color *set) {
    UpdateTexture(*set_texture, set);
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(*set_texture, 0, 0, WHITE);
    DrawFPS(0, 0);
    EndDrawing();
}

int main(void) {
    plane plane = {0};
    input input = {0};
    Color *set = malloc(sizeof(Color) * WIDTH * HEIGHT);

    for (int64_t y = 0; y < HEIGHT; ++y) {
        for (int64_t x = 0; x < WIDTH; ++x) {
            set[y * WIDTH + x] = (Color){255, 255, 255, 255};
        }
    }

    Image screenImage = {.data = set,
                         .width = WIDTH,
                         .height = HEIGHT,
                         .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                         .mipmaps = 1};

    plane_init(&plane, WIDTH, HEIGHT, 3.5, 1);

    SetTraceLogLevel(LOG_NONE);
    InitWindow(WIDTH, HEIGHT, "Mandelbrot");
    SetTargetFPS(60);

    Texture2D mandelbrot_texture = LoadTextureFromImage(screenImage);

    while (!WindowShouldClose()) {
        handle_input(&input);
        update_plane(&plane, &input);
        mandelbrot_plot_avx(&plane, WIDTH, HEIGHT, set);
        render(&mandelbrot_texture, set);
    }

    free(set);
    UnloadTexture(mandelbrot_texture);
    CloseWindow();

    return 0;
}
