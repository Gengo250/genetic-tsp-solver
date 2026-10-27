#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "genetic.h"
#include "gui.h"
#include "point_generator.h"

#define UNIFORM_POINT_MIN 0.0
#define UNIFORM_POINT_MAX 100.0
#define CIRCLE_RADIUS 100.0

static void print_route(const int *route, int num_points)
{
    for (int i = 0; i < num_points; i++)
    {
        printf("%s%d", i == 0 ? "" : " -> ", route[i]);
    }
    printf(" -> %d", route[0]);
}

static int run_scenario(const char *name, const Point *points, int num_points, int print_every)
{
    Population *population = population_create(POPULATION_SIZE , num_points);
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

    // Controle de estagnacao e limites dinamicos
    int stagnant_generations = 0;
    int stagnation_limit = 150;
    int max_generations = (num_points * 20 > 1000) ? (num_points * 20) : 1000;

    // Registro exato da melhoria (Epoca e Tempo CPU)
    int best_generation = 0;
    double best_cpu_time = 0.0;

    const clock_t started = clock();
    if (started == (clock_t)-1)
    {
        fprintf(stderr, "Erro: nao foi possivel iniciar a medicao de tempo.\n");
        population_free(population);
        return 0;
    }

    printf("\n=== Cenario: %s | N=%d | Populacao=%d | Max Epocas=%d ===\n",
           name, num_points, POPULATION_SIZE, max_generations);

    int stopped_by_stagnation = 0;
    int current_generation;

    for (current_generation = 1; current_generation <= max_generations; current_generation++)
    {
        Population *next = evolve_population(population, points, num_points,
                                             current_generation, max_generations);
        if (next == NULL)
        {
            fprintf(stderr, "Erro: falha ao criar a geracao %d.\n", current_generation);
            population_free(population);
            return 0;
        }

        population_free(population);
        population = next;
        best = population_best(population);

        // Registro de melhoria estrita
        if (best->distance < best_distance)
        {
            best_distance = best->distance;
            stagnant_generations = 0;
            best_generation = current_generation;
            best_cpu_time = (double)(clock() - started) / (double)CLOCKS_PER_SEC;
        }
        else
        {
            stagnant_generations++;
        }

        // Impressao controlada pelo PRINT_EVERY
        if (current_generation % print_every == 0)
        {
            const clock_t elapsed_ticks = clock() - started;
            printf("[%s] Epoca %4d | Melhor Distancia: %.6f | Tempo CPU: %.3fs | Rota: ",
                   name, current_generation, best_distance,
                   (double)elapsed_ticks / (double)CLOCKS_PER_SEC);
            print_route(best->route, num_points);
            putchar('\n');
            fflush(stdout);
        }

        if (stagnant_generations >= stagnation_limit)
        {
            stopped_by_stagnation = 1;
            break;
        }
    }

    const clock_t total_ticks = clock() - started;

    printf("\n--- RESULTADO FINAL (%s) ---\n", name);
    printf("Status: %s\n", stopped_by_stagnation ? "Convergencia atingida (estagnacao)" : "Limite maximo de epocas atingido");
    printf("Melhor distancia encontrada: %.6f\n", best_distance);
    printf("Encontrada na epoca: %d (Tempo CPU ate o otimo: %.3fs)\n", best_generation, best_cpu_time);
    printf("Epoca de encerramento: %d (Tempo CPU total: %.3fs)\n",
           stopped_by_stagnation ? current_generation : max_generations,
           (double)total_ticks / (double)CLOCKS_PER_SEC);

    population_free(population);
    return 1;
}

int main(int argc, char **argv)
{
    // Suporte para o modo grafico padrao
    if (argc == 1)
    {
        run_gui();
        return EXIT_SUCCESS;
    }

    // Leitura via linha de comando
    if (argc != 4)
    {
        fprintf(stderr, "Uso: %s <cenario: 1=Uniforme, 2=Circular> <N> <PRINT_EVERY>\n", argv[0]);
        fprintf(stderr, "Ou execute sem argumentos para abrir a interface GUI.\n");
        return EXIT_FAILURE;
    }

    int cenario = atoi(argv[1]);
    int num_points = atoi(argv[2]);
    int print_every = atoi(argv[3]);

    if (num_points < 8)
    {
        fprintf(stderr, "Erro: A quantidade de pontos (N) deve ser no minimo 8.\n");
        return EXIT_FAILURE;
    }

    if (print_every < 1)
    {
        print_every = 1;
    }

    srand((unsigned int)time(NULL));

    Point *points = NULL;
    const char *nome_cenario = "";

    if (cenario == 1)
    {
        points = generate_uniform_points(num_points, UNIFORM_POINT_MIN, UNIFORM_POINT_MAX);
        nome_cenario = "uniforme";
    }
    else if (cenario == 2)
    {
        points = generate_circle_points(num_points, CIRCLE_RADIUS);
        nome_cenario = "circular";
    }
    else
    {
        fprintf(stderr, "Erro: Cenario invalido. Escolha 1 (Uniforme) ou 2 (Circular).\n");
        return EXIT_FAILURE;
    }

    if (points == NULL)
    {
        fprintf(stderr, "Erro: falha ao gerar os pontos do cenario.\n");
        return EXIT_FAILURE;
    }

    printf("Iniciando AG | Torneio k=%d | Elitismo=%d | Mutacao inicial=%.0f%%\n",
           TOURNAMENT_SIZE, ELITISM_COUNT, MUTATION_RATE_INITIAL * 100.0);

    const int ok = run_scenario(nome_cenario, points, num_points, print_every);

    free(points);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}