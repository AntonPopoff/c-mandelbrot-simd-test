#include <omp.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#include "mandelbrot.h"
#include "plane.h"

#define WIDTH 1920
#define HEIGHT 1080

typedef struct {
    ms_vec2i mouse;
    ms_vec2i mouse_dxdy;
    double mouse_wheel_v;
    bool plane_drag;
    bool zoom_down;
    bool zoom_up;
    bool reset;
} ms_input;

typedef struct {
    int32_t effort;
    ms_impl impl;
    ms_plot plot_f;
} ms_config;

void handle_input(ms_input *input, ms_config *config) {
    double mouse_x = GetMouseX();
    double mouse_y = GetMouseY();
    input->mouse_dxdy.x = mouse_x - input->mouse.x;
    input->mouse_dxdy.y = mouse_y - input->mouse.y;
    input->mouse.x = mouse_x;
    input->mouse.y = mouse_y;
    input->plane_drag = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    input->zoom_down = (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsKeyUp(KEY_LEFT_SHIFT)) ||
                       (GetMouseWheelMove() > 0);
    input->zoom_up = (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsKeyDown(KEY_LEFT_SHIFT)) ||
                     (GetMouseWheelMove() < 0);
    input->reset = IsKeyPressed(KEY_R);

    if (IsKeyPressed(KEY_F1)) {
        config->impl = SCALAR;
        config->plot_f = ms_plot_scalar;
    } else if (IsKeyPressed(KEY_F2)) {
        config->impl = SSE4;
        config->plot_f = ms_plot_sse4;
    } else if (IsKeyPressed(KEY_F3)) {
        config->impl = AVX2;
        config->plot_f = ms_plot_avx2;
    }
}

void update_plane(const ms_input *input, ms_plane *p, ms_config *c) {
    // TODO: Add drag bounds
    if (input->plane_drag == true) {
        ms_vec2d dxdy = plane_from_screen_no_offset(p, input->mouse_dxdy.x, input->mouse_dxdy.y);
        p->offset.x += dxdy.x;
        p->offset.y += dxdy.y;
    }

    if (input->zoom_down) {
        double z = p->zoom * (1 + 0.5 * GetFrameTime());
        z = z <= 150000000000 ? z : 150000000000;
        plane_zoom_around(p, input->mouse.x, input->mouse.y, z);
    }
    if (input->zoom_up) {
        double z = p->zoom / (1 + 0.5 * GetFrameTime());
        z = z >= 0.25 ? z : 0.25;
        plane_zoom_around(p, input->mouse.x, input->mouse.y, z);
    }

    if (input->reset) {
        p->zoom = 1;
        p->offset.x = 2.5;
        p->offset.y = -1;
    }

    if (input->mouse_wheel_v != 0) {
        plane_zoom_around(p, input->mouse.x, input->mouse.y, input->mouse_wheel_v * 5);
    }

    p->screen.x = GetScreenWidth();
    p->screen.y = GetScreenHeight();
    c->effort = zoom_effort(p->zoom);
}

void render(const ms_plane *p, const ms_input *i, const ms_config *c, const Texture2D *set_texture,
            const ms_surface *s) {
    UpdateTexture(*set_texture, s->surface.data);
    BeginDrawing();
    ClearBackground(BLACK);

    DrawTexture(*set_texture, 0, 0, WHITE);

    DrawFPS(0, 0);

    switch (c->impl) {
    case SCALAR:
        DrawText("Implementation: Scalar", 0, 25, 20, RAYWHITE);
        break;
    case SSE4:
        DrawText("Implementation: SSE4", 0, 25, 20, RAYWHITE);
        break;
    case AVX2:
        DrawText("Implementation: AVX2", 0, 25, 20, RAYWHITE);
        break;
    }

    DrawText(TextFormat("Threads: %d", omp_get_max_threads()), 0, 50, 20, RAYWHITE);
    DrawText(TextFormat("Zoom: x%.2f", p->zoom), 0, 75, 20, RAYWHITE);
    DrawText(TextFormat("Effort: %d Iter/p", c->effort), 0, 100, 20, RAYWHITE);

    EndDrawing();
}

int main(void) {
    SetTraceLogLevel(LOG_NONE);
    InitWindow(WIDTH, HEIGHT, "M");
    // SetTargetFPS(60);
    ToggleFullscreen();

    ms_plane plane = {0};
    plane.screen.x = WIDTH;
    plane.screen.y = HEIGHT;
    plane.scale = plane.screen.x / 3.5;
    plane.zoom = 1;
    plane.offset.x = 2.5;
    plane.offset.y = -1;

    ms_config config = {0};
    config.impl = SCALAR;
    config.plot_f = ms_plot_scalar;

    ms_surface surface = {0};
    ms_surface_init(&surface, WIDTH, HEIGHT);

    ms_input input = {0};
    Texture2D render_texture = LoadTextureFromImage(surface.surface);

    while (!WindowShouldClose()) {
        handle_input(&input, &config);
        update_plane(&input, &plane, &config);
        config.plot_f(&plane, &surface, config.effort);
        render(&plane, &input, &config, &render_texture, &surface);
    }

    UnloadTexture(render_texture);
    CloseWindow();
    ms_surface_free(&surface);

    return 0;
}
