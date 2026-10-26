#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "genetic.h"
#include "gui.h"
#include "point_generator.h"

#define UNIFORM_POINT_MIN 0.0
#define UNIFORM_POINT_MAX 100.0
#define CIRCLE_RADIUS 100.0
#define BENCHMARK_POINT_COUNT 20

static void print_route(const int *route, int num_points)
{
    for (int i = 0; i < num_points; i++)
    {
        printf("%s%d", i == 0 ? "" : " -> ", route[i]);
    }
    printf(" -> %d", route[0]);
}

static int run_scenario(const char *name, const Point *points, int num_points)
{
    Population *population = population_create(POPULATION_SIZE, num_points);
    if (population == NULL)
    {
        fprintf(stderr, "Erro: nao foi possivel criar a populacao inicial.\n");
        return 0;
    }

    if (!evaluate_population(population, points, num_points))
    {
        fprintf(stderr, "Erro: nao foi possivel avaliar a populacao inicial.\n");
        population_free(population);
        return 0;
    }

    const Individual *best = population_best(population);
    double best_distance = best->distance;
    int stagnant_generations = 0;
    const clock_t started = clock();
    if (started == (clock_t)-1)
    {
        fprintf(stderr, "Erro: nao foi possivel iniciar a medicao de tempo.\n");
        population_free(population);
        return 0;
    }

    printf("\nCenario: %s | pontos: %d | populacao: %d\n",
           name, num_points, POPULATION_SIZE);

    int generations_completed = 0;
    int stopped_by_stagnation = 0;
    for (int generation = 1; generation <= MAX_GENERATIONS; generation++)
    {
        Population *next = evolve_population(population, points, num_points,
                                             generation, MAX_GENERATIONS);
        if (next == NULL)
        {
            fprintf(stderr, "Erro: falha ao criar a geracao %d.\n", generation);
            population_free(population);
            return 0;
        }

        population_free(population);
        population = next;
        best = population_best(population);
        if (best->distance < best_distance)
        {
            best_distance = best->distance;
            stagnant_generations = 0;
        }
        else
        {
            stagnant_generations++;
        }

        const clock_t elapsed_ticks = clock() - started;
        if (elapsed_ticks == (clock_t)-1)
        {
            fprintf(stderr, "Erro: falha ao medir tempo na geracao %d.\n", generation);
            population_free(population);
            return 0;
        }

        printf("[%s] epoca %4d | melhor distancia: %.6f | crossover: %5.1f%%"
               " | mutacao: %5.1f%% | tempo: %.3fs | rota: ",
               name, generation, best_distance,
               crossover_rate_for_generation(generation, MAX_GENERATIONS) * 100.0,
               mutation_rate_for_generation(generation, MAX_GENERATIONS) * 100.0,
               (double)elapsed_ticks / (double)CLOCKS_PER_SEC);
        print_route(best->route, num_points);
        putchar('\n');
        fflush(stdout);
        generations_completed = generation;

        if (stagnant_generations >= STAGNATION_LIMIT)
        {
            stopped_by_stagnation = 1;
            break;
        }
    }

    const clock_t total_ticks = clock() - started;
    if (total_ticks == (clock_t)-1)
    {
        fprintf(stderr, "Erro: falha ao medir o tempo total.\n");
        population_free(population);
        return 0;
    }

    best = population_best(population);
    printf("%s (%s): epoca %d, distancia %.6f, tempo %.3fs"
           " (%d epocas sem melhora).\n",
           stopped_by_stagnation ? "Convergencia" : "Limite de epocas atingido",
           name, generations_completed, best->distance,
           (double)total_ticks / (double)CLOCKS_PER_SEC,
           stagnant_generations);

    population_free(population);
    return 1;
}

static int run_benchmark(void)
{
    srand(42);

    Point *uniform_points = generate_uniform_points(
        BENCHMARK_POINT_COUNT, UNIFORM_POINT_MIN, UNIFORM_POINT_MAX);
    if (uniform_points == NULL)
    {
        fprintf(stderr, "Erro: nao foi possivel gerar pontos uniformes.\n");
        return EXIT_FAILURE;
    }

    Point *circle_points = generate_circle_points(BENCHMARK_POINT_COUNT, CIRCLE_RADIUS);
    if (circle_points == NULL)
    {
        fprintf(stderr, "Erro: nao foi possivel gerar pontos circulares.\n");
        free(uniform_points);
        return EXIT_FAILURE;
    }

    printf("AG para TSP: torneio k=%d, elitismo=%d, max=%d epocas,"
           " parada por estagnacao=%d epocas.\n",
           TOURNAMENT_SIZE, ELITISM_COUNT, MAX_GENERATIONS, STAGNATION_LIMIT);
    printf("Taxas lineares: crossover %.0f%% -> %.0f%%; mutacao %.0f%% -> %.0f%%.\n",
           CROSSOVER_RATE_INITIAL * 100.0, CROSSOVER_RATE_FINAL * 100.0,
           MUTATION_RATE_INITIAL * 100.0, MUTATION_RATE_FINAL * 100.0);

    const int uniform_ok = run_scenario("uniforme", uniform_points,
                                        BENCHMARK_POINT_COUNT);
    const int circle_ok = uniform_ok &&
                          run_scenario("circular", circle_points, BENCHMARK_POINT_COUNT);

    free(uniform_points);
    free(circle_points);
    return uniform_ok && circle_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--benchmark") == 0)
    {
        return run_benchmark();
    }

    if (argc != 1)
    {
        fprintf(stderr, "Uso: %s [--benchmark]\n", argv[0]);
        return EXIT_FAILURE;
    }

    run_gui();
    return EXIT_SUCCESS;
}
