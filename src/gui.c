#include <stdlib.h>

#include <stdio.h>
#include <string.h>

#include "raylib.h"

#include "gui.h"
#include "point.h"
#include "point_generator.h"

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 640

#define MIN_POINTS 8
#define MAX_POINTS 300

#define UNIFORM_MIN 0.0
#define UNIFORM_MAX 100.0
#define CIRCLE_RADIUS 100.0

#define SCENARIO_UNIFORM 0
#define SCENARIO_CIRCLE 1

static bool button(Rectangle bounds, const char *label, int font_size)
{
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);

    DrawRectangleRec(bounds, hover ? SKYBLUE : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 1.0f, DARKGRAY);

    const int text_width = MeasureText(label, font_size);
    DrawText(label,
             (int)(bounds.x + (bounds.width - (float)text_width) / 2.0f),
             (int)(bounds.y + (bounds.height - (float)font_size) / 2.0f),
             font_size, DARKGRAY);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool radio(Vector2 position, const char *label, bool selected)
{
    const Rectangle bounds = {position.x, position.y - 4.0f, 180.0f, 26.0f};
    const Vector2 center = {position.x + 9.0f, position.y + 9.0f};

    DrawCircleLines((int)center.x, (int)center.y, 9.0f, DARKGRAY);
    if (selected)
    {
        DrawCircleV(center, 5.0f, MAROON);
    }
    DrawText(label, (int)(position.x + 26.0f), (int)position.y, 18, DARKGRAY);

    return CheckCollisionPointRec(GetMousePosition(), bounds) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* Converte as coordenadas matematicas dos pontos para a area de desenho. */
static void draw_points(Rectangle area, const Point *points, int count)
{
    if (points == NULL || count <= 0)
    {
        return;
    }

    double min_x = points[0].x;
    double max_x = points[0].x;
    double min_y = points[0].y;
    double max_y = points[0].y;

    for (int i = 1; i < count; i++)
    {
        if (points[i].x < min_x)
            min_x = points[i].x;
        if (points[i].x > max_x)
            max_x = points[i].x;
        if (points[i].y < min_y)
            min_y = points[i].y;
        if (points[i].y > max_y)
            max_y = points[i].y;
    }

    double span_x = max_x - min_x;
    double span_y = max_y - min_y;
    if (span_x < 1e-9)
        span_x = 1.0;
    if (span_y < 1e-9)
        span_y = 1.0;

    const double padding = 45.0;
    const double scale_x = ((double)area.width - 2.0 * padding) / span_x;
    const double scale_y = ((double)area.height - 2.0 * padding) / span_y;
    const double scale = (scale_x < scale_y) ? scale_x : scale_y;

    const double center_x = (min_x + max_x) / 2.0;
    const double center_y = (min_y + max_y) / 2.0;
    const double origin_x = (double)area.x + (double)area.width / 2.0;
    const double origin_y = (double)area.y + (double)area.height / 2.0;

    for (int i = 0; i < count; i++)
    {
        const float screen_x = (float)(origin_x + (points[i].x - center_x) * scale);
        const float screen_y = (float)(origin_y - (points[i].y - center_y) * scale);

        DrawCircle((int)screen_x, (int)screen_y, 5.0f, MAROON);
        DrawText(TextFormat("%d", points[i].id),
                 (int)screen_x + 8, (int)screen_y - 16, 14, DARKGRAY);
    }
}

static Point *generate_points(int scenario, int count)
{
    if (scenario == SCENARIO_UNIFORM)
    {
        return generate_uniform_points(count, UNIFORM_MIN, UNIFORM_MAX);
    }
    return generate_circle_points(count, CIRCLE_RADIUS);
}

void run_gui(void)
{
    srand(42);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Genetic TSP Solver - geracao de pontos");
    SetTargetFPS(60);

    const Rectangle view_area = {260.0f, 70.0f, 680.0f, 540.0f};
    const Rectangle count_box = {20.0f, 106.0f, 110.0f, 36.0f};

    int count = 20;
    char count_text[4] = "20";
    bool editing_count = false;
    int scenario = SCENARIO_UNIFORM;

    int point_count = count;
    Point *points = generate_points(scenario, count);

    while (!WindowShouldClose())
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(GetMousePosition(), count_box))
            {
                editing_count = true;
            }
            else if (editing_count)
            {
                editing_count = false;

                int value = atoi(count_text);

                if (value < MIN_POINTS)
                    value = MIN_POINTS;
                if (value > MAX_POINTS)
                    value = MAX_POINTS;

                count = value;
                snprintf(count_text, sizeof(count_text), "%d", count);
            }
        }

        if (editing_count)
        {
            int key = GetCharPressed();

            while (key > 0)
            {
                if (key >= '0' && key <= '9')
                {
                    int length = (int)strlen(count_text);

                    if (length < 3)
                    {
                        count_text[length] = (char)key;
                        count_text[length + 1] = '\0';
                    }
                }

                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE))
            {
                int length = (int)strlen(count_text);

                if (length > 0)
                {
                    count_text[length - 1] = '\0';
                }
            }

            if (IsKeyPressed(KEY_ENTER))
            {
                int value = atoi(count_text);

                if (value < MIN_POINTS)
                    value = MIN_POINTS;
                if (value > MAX_POINTS)
                    value = MAX_POINTS;

                count = value;
                snprintf(count_text, sizeof(count_text), "%d", count);

                editing_count = false;
            }
        }
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Genetic TSP Solver", 20, 22, 22, DARKBLUE);
        DrawLine(20, 56, WINDOW_WIDTH - 20, 56, LIGHTGRAY);

        DrawText("Quantidade de pontos:", 20, 80, 18, DARKGRAY);

        DrawRectangleRec(count_box, WHITE);
        DrawRectangleLinesEx(count_box,
                             editing_count ? 2.0f : 1.0f,
                             editing_count ? DARKBLUE : DARKGRAY);
        DrawText(count_text, 32, 115, 20, DARKGRAY);

        /* Cursor simples para indicar que o campo esta em edicao. */
        if (editing_count)
        {
            DrawText("_", 32 + MeasureText(count_text, 20) + 2, 115, 20, DARKBLUE);
        }

        bool count_changed = false;

        if (button((Rectangle){140.0f, 106.0f, 42.0f, 36.0f}, "-", 20) ||
            IsKeyPressed(KEY_DOWN))
        {
            count--;
            count_changed = true;
        }
        if (button((Rectangle){190.0f, 106.0f, 42.0f, 36.0f}, "+", 20) ||
            IsKeyPressed(KEY_UP))
        {
            count++;
            count_changed = true;
        }

        if (count < MIN_POINTS)
            count = MIN_POINTS;
        if (count > MAX_POINTS)
            count = MAX_POINTS;

        /* Mantem o texto do campo em sincronia com os botoes e as setas. */
        if (count_changed)
        {
            snprintf(count_text, sizeof(count_text), "%d", count);
        }

        DrawText(TextFormat("minimo: %d", MIN_POINTS), 20, 150, 14, GRAY);

        DrawText("Cenario:", 20, 185, 18, DARKGRAY);
        if (radio((Vector2){20.0f, 215.0f}, "Uniforme", scenario == SCENARIO_UNIFORM))
        {
            scenario = SCENARIO_UNIFORM;
        }
        if (radio((Vector2){20.0f, 247.0f}, "Circulo", scenario == SCENARIO_CIRCLE))
        {
            scenario = SCENARIO_CIRCLE;
        }

        if (button((Rectangle){20.0f, 295.0f, 212.0f, 42.0f}, "Gerar pontos", 20))
        {
            free(points);
            points = generate_points(scenario, count);
            point_count = count;
        }

        DrawText(scenario == SCENARIO_UNIFORM ? "area: 0 a 100 em x e y"
                                              : "raio: 100",
                 20, 350, 14, GRAY);
        DrawText(TextFormat("pontos na tela: %d", point_count), 20, 372, 14, GRAY);

        DrawRectangleLinesEx(view_area, 1.0f, LIGHTGRAY);
        draw_points(view_area, points, point_count);

        EndDrawing();
    }

    free(points);
    CloseWindow();
}
