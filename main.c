#include "raylib.h"
#include <emmintrin.h>
#include <immintrin.h>
#include <mm_malloc.h>
#include <omp.h>
#include <smmintrin.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "plane.h"
#include "mandelbrot.h"

#define WIDTH 1280
#define HEIGHT 720

typedef struct {
    vec2i mouse;
    vec2i mouse_dxdy;
    double mouse_wheel_v;
    bool down;
    bool drag;
    bool up;
} input;

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

void render(const Texture2D *set_texture, mandelbrot *pixels) {
    UpdateTexture(*set_texture, pixels->data);
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(*set_texture, 0, 0, WHITE);
    DrawFPS(0, 0);
    EndDrawing();
}

void mandelbrot_init(mandelbrot *pixels, uint64_t width, uint64_t height) {
    size_t memlen = sizeof(Color) * width * height;
    pixels->data = malloc(sizeof(Color) * width * height);
    memset(pixels->data, 255, memlen);
}

void mandelbrot_free(mandelbrot *pixels) {
    free(pixels->data);
    pixels->data = NULL;
}

int main(void) {
    plane plane = {0};
    input input = {0};
    mandelbrot pixels = {0};

    plane_init(&plane, WIDTH, HEIGHT, 3.5, 1);
    mandelbrot_init(&pixels, WIDTH, HEIGHT);
    // omp_set_num_threads(4);

    InitWindow(WIDTH, HEIGHT, "M");
    SetTraceLogLevel(LOG_NONE);
    // SetTargetFPS(30);

    Image screenImage = {.data = pixels.data,
                         .width = WIDTH,
                         .height = HEIGHT,
                         .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                         .mipmaps = 1};

    Texture2D mandelbrot_texture = LoadTextureFromImage(screenImage);

    while (!WindowShouldClose()) {
        handle_input(&input);
        update_plane(&plane, &input);
        mandelbrot_plot_scalar(&plane, &pixels);
        // mandelbrot_plot_avx2(&plane, &pixels);
        // mandelbrot_plot_sse4(&plane, &pixels);
        render(&mandelbrot_texture, &pixels);
    }

    CloseWindow();

    mandelbrot_free(&pixels);
    // UnloadImage(screenImage);
    // UnloadTexture(mandelbrot_texture);

    return 0;
}
