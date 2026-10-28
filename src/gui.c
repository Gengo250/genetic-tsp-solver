#include <math.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

#include "raylib.h"

#include "genetic.h"
#include "gui.h"
#include "point.h"
#include "point_generator.h"
#include "population.h"

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 640

#define MIN_POINTS 8
#define MAX_POINTS 300

#define UNIFORM_MIN 0.0
#define UNIFORM_MAX 100.0
#define CIRCLE_RADIUS 100.0

#define SCENARIO_UNIFORM 0
#define SCENARIO_CIRCLE 1

/* M_PI nao existe em C estrito (-std=c17 sem extensoes GNU). */
#define TSP_PI 3.14159265358979323846

/* A fonte e carregada em 32 px e reduzida na hora de desenhar: os glifos saem
   nitidos em qualquer tamanho usado no painel. */
#define FONT_BASE_SIZE 32
#define FONT_SPACING 1.0f

/* Quantas geracoes rodam por frame. Acima disso o frame comeca a travar em
   mapas grandes; abaixo a evolucao fica lenta demais para acompanhar. */
#define MIN_STEPS_PER_FRAME 1
#define MAX_STEPS_PER_FRAME 50

/* Acima deste numero de pontos os rotulos viram ruido visual. */
#define MAX_LABELLED_POINTS 60

static Font ui_font;
static bool ui_font_is_custom = false;

/* ------------------------------------------------------------------ fonte */

/* O binario pode ser executado da raiz do projeto ou de dentro de build/, e a
   fonte e um arquivo externo. Procura nos caminhos relativos usuais e tambem
   ao lado do executavel; se nada existir, a fonte embutida do raylib assume e
   a interface continua legivel. */
static Font load_ui_font(void)
{
    static const char *candidates[] = {
        "resources/DejaVuSans.ttf",
        "../resources/DejaVuSans.ttf",
        "../../resources/DejaVuSans.ttf",
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
    {
        if (FileExists(candidates[i]))
        {
            ui_font_is_custom = true;
            return LoadFontEx(candidates[i], FONT_BASE_SIZE, NULL, 0);
        }
    }

    const char *beside_binary =
        TextFormat("%sresources/DejaVuSans.ttf", GetApplicationDirectory());

    if (FileExists(beside_binary))
    {
        ui_font_is_custom = true;
        return LoadFontEx(beside_binary, FONT_BASE_SIZE, NULL, 0);
    }

    return GetFontDefault();
}

static void draw_text(const char *text, float x, float y, float size, Color color)
{
    DrawTextEx(ui_font, text, (Vector2){x, y}, size, FONT_SPACING, color);
}

static float text_width(const char *text, float size)
{
    return MeasureTextEx(ui_font, text, size, FONT_SPACING).x;
}

/* --------------------------------------------------------------- widgets */

static bool button(Rectangle bounds, const char *label, float font_size, bool enabled)
{
    const bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);

    DrawRectangleRec(bounds, enabled ? (hover ? SKYBLUE : LIGHTGRAY) : (Color){235, 235, 235, 255});
    DrawRectangleLinesEx(bounds, 1.0f, enabled ? DARKGRAY : LIGHTGRAY);

    draw_text(label,
              bounds.x + (bounds.width - text_width(label, font_size)) / 2.0f,
              bounds.y + (bounds.height - font_size) / 2.0f,
              font_size, enabled ? DARKGRAY : GRAY);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool radio(Vector2 position, const char *label, bool selected)
{
    const Rectangle bounds = {position.x, position.y - 4.0f, 180.0f, 24.0f};
    const Vector2 center = {position.x + 8.0f, position.y + 8.0f};

    DrawCircleLines((int)center.x, (int)center.y, 8.0f, DARKGRAY);
    if (selected)
    {
        DrawCircleV(center, 4.5f, MAROON);
    }
    draw_text(label, position.x + 24.0f, position.y, 16.0f, DARKGRAY);

    return CheckCollisionPointRec(GetMousePosition(), bounds) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* ------------------------------------------------------------ projecao 2D */

/* Converte as coordenadas matematicas dos pontos para a area de desenho.
   Calculada uma vez por frame e reaproveitada pela rota e pelos pontos, para
   que as linhas caiam exatamente em cima dos circulos. */
typedef struct
{
    double scale;
    double center_x;
    double center_y;
    double origin_x;
    double origin_y;
} ViewTransform;

static ViewTransform view_transform(Rectangle area, const Point *points, int count)
{
    ViewTransform view = {1.0, 0.0, 0.0,
                          (double)area.x + (double)area.width / 2.0,
                          (double)area.y + (double)area.height / 2.0};

    if (points == NULL || count <= 0)
    {
        return view;
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

    view.scale = (scale_x < scale_y) ? scale_x : scale_y;
    view.center_x = (min_x + max_x) / 2.0;
    view.center_y = (min_y + max_y) / 2.0;

    return view;
}

static Vector2 to_screen(const ViewTransform *view, const Point *point)
{
    /* O eixo y da tela cresce para baixo, por isso o sinal invertido. */
    return (Vector2){
        (float)(view->origin_x + (point->x - view->center_x) * view->scale),
        (float)(view->origin_y - (point->y - view->center_y) * view->scale)};
}

static void draw_route(const ViewTransform *view, const Point *points,
                       const int *route, int count)
{
    if (points == NULL || route == NULL || count < 2)
    {
        return;
    }

    for (int i = 0; i < count; i++)
    {
        /* A ultima aresta fecha o ciclo: route[count-1] -> route[0]. */
        const Vector2 a = to_screen(view, &points[route[i]]);
        const Vector2 b = to_screen(view, &points[route[(i + 1) % count]]);

        DrawLineEx(a, b, 2.0f, (Color){0, 121, 241, 190});
    }
}

static void draw_points(const ViewTransform *view, const Point *points, int count)
{
    if (points == NULL || count <= 0)
    {
        return;
    }

    const float radius = (count > 100) ? 3.0f : 5.0f;

    for (int i = 0; i < count; i++)
    {
        const Vector2 screen = to_screen(view, &points[i]);

        DrawCircleV(screen, radius, MAROON);

        if (count <= MAX_LABELLED_POINTS)
        {
            draw_text(TextFormat("%d", points[i].id),
                      screen.x + 8.0f, screen.y - 15.0f, 13.0f, DARKGRAY);
        }
    }
}

/* -------------------------------------------------------- motor na tela */

typedef struct
{
    Population *population;
    int generation;
    int max_generations;
    int stagnant;
    double best_distance;
    int best_generation;
    double best_time;
    double elapsed;  /* tempo acumulado de evolucao, sem contar pausas */
    double resumed_at;
    bool running;
    bool finished;
    const char *status;
} Solver;

static void solver_reset(Solver *solver)
{
    population_free(solver->population);

    solver->population = NULL;
    solver->generation = 0;
    solver->max_generations = 0;
    solver->stagnant = 0;
    solver->best_distance = 0.0;
    solver->best_generation = 0;
    solver->best_time = 0.0;
    solver->elapsed = 0.0;
    solver->resumed_at = 0.0;
    solver->running = false;
    solver->finished = false;
    solver->status = "parado";
}

static double solver_elapsed(const Solver *solver)
{
    if (solver->running)
    {
        return solver->elapsed + (GetTime() - solver->resumed_at);
    }

    return solver->elapsed;
}

static bool solver_start(Solver *solver, const Point *points, int count)
{
    solver_reset(solver);

    solver->population = population_create(population_size_for(count), count);
    if (solver->population == NULL)
    {
        solver->status = "erro: sem memoria";
        return false;
    }

    if (!evaluate_population(solver->population, points, count))
    {
        population_free(solver->population);
        solver->population = NULL;
        solver->status = "erro: populacao invalida";
        return false;
    }

    const Individual *best = population_best(solver->population);

    solver->best_distance = best->distance;
    solver->max_generations = max_generations_for(count);
    solver->resumed_at = GetTime();
    solver->running = true;
    solver->status = "evoluindo";

    return true;
}

static void solver_pause(Solver *solver)
{
    if (!solver->running)
    {
        return;
    }

    solver->elapsed += GetTime() - solver->resumed_at;
    solver->running = false;
    solver->status = "pausado";
}

static void solver_resume(Solver *solver)
{
    if (solver->running || solver->population == NULL || solver->finished)
    {
        return;
    }

    solver->resumed_at = GetTime();
    solver->running = true;
    solver->status = "evoluindo";
}

static void solver_stop(Solver *solver, const char *reason)
{
    solver_pause(solver);
    solver->finished = true;
    solver->status = reason;
}

/* Uma geracao: mesmo criterio de parada duplo do modo console. */
static void solver_step(Solver *solver, const Point *points, int count)
{
    Population *next = evolve_population(solver->population, points, count,
                                         solver->generation + 1,
                                         solver->max_generations);
    if (next == NULL)
    {
        solver_stop(solver, "erro: falha na geracao");
        return;
    }

    population_free(solver->population);
    solver->population = next;
    solver->generation++;

    const Individual *best = population_best(solver->population);

    if (best->distance < solver->best_distance)
    {
        solver->best_distance = best->distance;
        solver->best_generation = solver->generation;
        solver->best_time = solver_elapsed(solver);
        solver->stagnant = 0;
    }
    else
    {
        solver->stagnant++;
    }

    if (solver->stagnant >= stagnation_limit_for(count))
    {
        solver_stop(solver, "convergiu (estagnacao)");
    }
    else if (solver->generation >= solver->max_generations)
    {
        solver_stop(solver, "limite de epocas");
    }
}

/* ------------------------------------------------------------- cenarios */

static Point *generate_points(int scenario, int count)
{
    if (scenario == SCENARIO_UNIFORM)
    {
        return generate_uniform_points(count, UNIFORM_MIN, UNIFORM_MAX);
    }

    return generate_circle_points(count, CIRCLE_RADIUS);
}

/* No cenario circular o otimo e conhecido: o perimetro do poligono regular de
   N vertices inscrito no circulo. Serve de referencia para medir o gap. */
static double circle_optimum(int count)
{
    if (count < 3)
    {
        return 0.0;
    }

    return 2.0 * (double)count * CIRCLE_RADIUS * sin(TSP_PI / (double)count);
}

/* ------------------------------------------------------------------ loop */

void run_gui(void)
{
    srand(42);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Genetic TSP Solver");
    SetTargetFPS(60);

    ui_font = load_ui_font();
    if (ui_font_is_custom)
    {
        SetTextureFilter(ui_font.texture, TEXTURE_FILTER_BILINEAR);
    }

    const Rectangle view_area = {252.0f, 70.0f, 688.0f, 550.0f};
    const Rectangle count_box = {20.0f, 96.0f, 110.0f, 34.0f};

    int count = 20;
    char count_text[4] = "20";
    bool editing_count = false;
    int scenario = SCENARIO_UNIFORM;
    int steps_per_frame = 5;

    int point_count = count;
    Point *points = generate_points(scenario, count);

    Solver solver;
    memset(&solver, 0, sizeof(solver));
    solver_reset(&solver);

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

        /* Evolucao: roda antes do desenho para a tela mostrar a geracao atual. */
        if (solver.running && solver.population != NULL)
        {
            for (int step = 0; step < steps_per_frame && solver.running; step++)
            {
                solver_step(&solver, points, point_count);
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        draw_text("Genetic TSP Solver", 20.0f, 20.0f, 24.0f, DARKBLUE);
        DrawLine(20, 58, WINDOW_WIDTH - 20, 58, LIGHTGRAY);

        draw_text("Quantidade de pontos:", 20.0f, 72.0f, 16.0f, DARKGRAY);

        DrawRectangleRec(count_box, WHITE);
        DrawRectangleLinesEx(count_box,
                             editing_count ? 2.0f : 1.0f,
                             editing_count ? DARKBLUE : DARKGRAY);
        draw_text(count_text, 32.0f, 104.0f, 18.0f, DARKGRAY);

        /* Cursor simples para indicar que o campo esta em edicao. */
        if (editing_count)
        {
            draw_text("_", 32.0f + text_width(count_text, 18.0f) + 2.0f, 104.0f,
                      18.0f, DARKBLUE);
        }

        bool count_changed = false;

        if (button((Rectangle){138.0f, 96.0f, 42.0f, 34.0f}, "-", 18.0f, true) ||
            IsKeyPressed(KEY_DOWN))
        {
            count--;
            count_changed = true;
        }
        if (button((Rectangle){186.0f, 96.0f, 42.0f, 34.0f}, "+", 18.0f, true) ||
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

        draw_text(TextFormat("minimo: %d   maximo: %d", MIN_POINTS, MAX_POINTS),
                  20.0f, 136.0f, 13.0f, GRAY);

        draw_text("Cenario:", 20.0f, 160.0f, 16.0f, DARKGRAY);
        if (radio((Vector2){20.0f, 186.0f}, "Uniforme", scenario == SCENARIO_UNIFORM))
        {
            scenario = SCENARIO_UNIFORM;
        }
        if (radio((Vector2){20.0f, 214.0f}, "Circulo", scenario == SCENARIO_CIRCLE))
        {
            scenario = SCENARIO_CIRCLE;
        }

        if (button((Rectangle){20.0f, 246.0f, 208.0f, 36.0f}, "Gerar pontos", 17.0f, true))
        {
            /* Mapa novo invalida a evolucao em andamento. */
            solver_reset(&solver);
            free(points);
            points = generate_points(scenario, count);
            point_count = count;
        }

        const bool has_points = (points != NULL && point_count >= MIN_POINTS);
        const char *run_label = solver.running ? "Pausar" : (solver.population == NULL ? "Iniciar" : "Continuar");

        if (button((Rectangle){20.0f, 290.0f, 208.0f, 36.0f}, run_label, 17.0f,
                   has_points && !solver.finished) ||
            (IsKeyPressed(KEY_SPACE) && !editing_count && has_points && !solver.finished))
        {
            if (solver.running)
            {
                solver_pause(&solver);
            }
            else if (solver.population == NULL)
            {
                solver_start(&solver, points, point_count);
            }
            else
            {
                solver_resume(&solver);
            }
        }

        if (button((Rectangle){20.0f, 332.0f, 208.0f, 36.0f}, "Resetar", 17.0f,
                   solver.population != NULL))
        {
            solver_reset(&solver);
        }

        draw_text(TextFormat("Velocidade: %d ger/frame", steps_per_frame),
                  20.0f, 382.0f, 13.0f, GRAY);

        if (button((Rectangle){20.0f, 400.0f, 42.0f, 30.0f}, "-", 17.0f,
                   steps_per_frame > MIN_STEPS_PER_FRAME))
        {
            steps_per_frame--;
        }
        if (button((Rectangle){68.0f, 400.0f, 42.0f, 30.0f}, "+", 17.0f,
                   steps_per_frame < MAX_STEPS_PER_FRAME))
        {
            steps_per_frame++;
        }

        DrawLine(20, 448, 232, 448, LIGHTGRAY);

        /* Painel de acompanhamento: e o que o enunciado pede para mostrar a
           cada epoca. */
        const float stats_x = 20.0f;
        float stats_y = 460.0f;
        const float stats_step = 20.0f;
        const float stats_size = 14.0f;

        draw_text(TextFormat("Epoca: %d / %d", solver.generation, solver.max_generations),
                  stats_x, stats_y, stats_size, DARKGRAY);
        stats_y += stats_step;

        if (solver.population != NULL)
        {
            draw_text(TextFormat("Melhor distancia: %.3f", solver.best_distance),
                      stats_x, stats_y, stats_size, DARKGRAY);
            stats_y += stats_step;

            draw_text(TextFormat("Melhor na epoca: %d", solver.best_generation),
                      stats_x, stats_y, stats_size, DARKGRAY);
            stats_y += stats_step;

            if (scenario == SCENARIO_CIRCLE)
            {
                const double optimum = circle_optimum(point_count);

                draw_text(TextFormat("Otimo: %.3f", optimum),
                          stats_x, stats_y, stats_size, DARKGRAY);
                stats_y += stats_step;

                if (optimum > 0.0)
                {
                    draw_text(TextFormat("Gap: %.2f%%",
                                         100.0 * (solver.best_distance - optimum) / optimum),
                              stats_x, stats_y, stats_size, MAROON);
                    stats_y += stats_step;
                }
            }

            draw_text(TextFormat("Tempo: %.2fs", solver_elapsed(&solver)),
                      stats_x, stats_y, stats_size, DARKGRAY);
            stats_y += stats_step;

            draw_text(TextFormat("Mutacao: %.1f%%",
                                 100.0 * mutation_rate_for_generation(solver.generation,
                                                                      solver.max_generations)),
                      stats_x, stats_y, stats_size, GRAY);
            stats_y += stats_step;
        }

        draw_text(TextFormat("Status: %s", solver.status),
                  stats_x, stats_y, stats_size, DARKBLUE);

        DrawRectangleLinesEx(view_area, 1.0f, LIGHTGRAY);

        const ViewTransform view = view_transform(view_area, points, point_count);

        if (solver.population != NULL)
        {
            const Individual *best = population_best(solver.population);

            if (best != NULL)
            {
                draw_route(&view, points, best->route, point_count);
            }
        }

        draw_points(&view, points, point_count);

        draw_text(TextFormat("%s | %d pontos | populacao %d | torneio k=%d | elitismo %d | mutacao %.0f%%",
                             scenario == SCENARIO_UNIFORM ? "uniforme" : "circulo",
                             point_count, population_size_for(point_count), TOURNAMENT_SIZE,
                             ELITISM_COUNT, MUTATION_RATE_INITIAL * 100.0),
                  view_area.x + 8.0f, view_area.y + view_area.height + 6.0f, 13.0f, GRAY);

        EndDrawing();
    }

    population_free(solver.population);
    free(points);

    if (ui_font_is_custom)
    {
        UnloadFont(ui_font);
    }

    CloseWindow();
}
