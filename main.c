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
    ms_impl implementation;
    bool down;
    bool drag;
    bool up;
} input;

void plane_init(ms_plane *p, int64_t screen_width, int64_t screen_height, double unit_scale,
                double zoom) {
    p->screen.x = screen_width;
    p->screen.y = screen_height;
    p->scale.x = screen_width / unit_scale;
    p->scale.y = screen_width / unit_scale;
    p->offset.x = (screen_width / p->scale.x) / 2 + 1;
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

    if (IsKeyPressed(KEY_F1)) {
        input->implementation = SCALAR;
    }
    if (IsKeyPressed(KEY_F2)) {
        input->implementation = SSE4;
    }
    if (IsKeyPressed(KEY_F3)) {
        input->implementation = AVX2;
    }
}

void update_plane(ms_plane *plane, const input *input) {
    if (input->drag == true) {
        ms_vec2d dxdy =
            plane_from_screen_no_offset(plane, input->mouse_dxdy.x, input->mouse_dxdy.y);
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

void render(const input *i, const Texture2D *set_texture, const ms_surface *s) {
    UpdateTexture(*set_texture, s->surface.data);
    BeginDrawing();
    ClearBackground(BLACK);

    DrawTexture(*set_texture, 0, 0, WHITE);

    DrawFPS(0, 0);

    switch (i->implementation) {
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
    // TODO: Add iterations number information
    // TODO: Add zoom level information

    EndDrawing();
}

int main(void) {
    ms_plane plane = {0};
    input input = {0};
    ms_surface surface = {0};

    input.implementation = AVX2;

    plane_init(&plane, WIDTH, HEIGHT, 3.5, 1);
    ms_surface_init(&surface, WIDTH, HEIGHT);

    SetTraceLogLevel(LOG_NONE);
    InitWindow(WIDTH, HEIGHT, "M");
    SetTargetFPS(60);
    ToggleFullscreen();

    Texture2D mandelbrot_texture = LoadTextureFromImage(surface.surface);

    while (!WindowShouldClose()) {
        handle_input(&input);
        update_plane(&plane, &input);
        ms_plot(&plane, &surface, input.implementation);
        render(&input, &mandelbrot_texture, &surface);
    }

    UnloadTexture(mandelbrot_texture);
    CloseWindow();
    ms_surface_free(&surface);

    return 0;
}
