/* Testes da Pessoa 2: ambiente, memoria e avaliacao.
   Nao depende do Raylib: compila so com point_generator.c, population.c e
   genetic.c. Rodar com `make test` (ASan + UBSan). */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "genetic.h"
#include "individual.h"
#include "point.h"
#include "point_generator.h"
#include "population.h"

/* M_PI nao existe em C estrito (-std=c17 sem extensoes GNU). */
#define TEST_PI 3.14159265358979323846
#define EPS 1e-9

static int checks = 0;
static int failures = 0;

static void check(int ok, const char *what)
{
    checks++;

    if (!ok)
    {
        failures++;
    }

    printf("  [%s] %s\n", ok ? " OK " : "FALHA", what);
}

static void test_routes_are_valid(void)
{
    printf("\n1) Toda rota criada e uma permutacao valida\n");

    const int sizes[] = {1, 2, 8, 50, 500};
    char msg[128];

    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
    {
        const int n = sizes[s];
        Population *pop = population_create(100, n);
        int invalid = 0;

        if (pop == NULL)
        {
            check(0, "population_create devolveu NULL");
            continue;
        }

        for (int i = 0; i < pop->size; i++)
        {
            if (!route_is_valid(pop->path[i].route, n))
            {
                invalid++;
            }
        }

        snprintf(msg, sizeof(msg), "N=%d: 100 rotas, %d invalidas", n, invalid);
        check(invalid == 0 && pop->size == 100 && pop->num_points == n, msg);
        population_free(pop);
    }
}

static void test_fisher_yates_uniform(void)
{
    printf("\n2) Fisher-Yates sem vies (N=4, 24 permutacoes)\n");

    const int runs = 240000;
    const double expected = runs / 24.0;
    Population *pop = population_create(runs, 4);

    if (pop == NULL)
    {
        check(0, "population_create devolveu NULL");
        return;
    }

    /* Cada permutacao vira um codigo base 4 (0..255) para contar frequencias. */
    int count[256] = {0};

    for (int i = 0; i < runs; i++)
    {
        const int *r = pop->path[i].route;
        count[r[0] * 64 + r[1] * 16 + r[2] * 4 + r[3]]++;
    }

    int distinct = 0;
    double max_dev = 0.0;

    for (int code = 0; code < 256; code++)
    {
        if (count[code] == 0)
        {
            continue;
        }

        const double dev = fabs(count[code] - expected) / expected;

        if (dev > max_dev)
        {
            max_dev = dev;
        }

        printf("     %d%d%d%d: %5d", code / 64, (code / 16) % 4, (code / 4) % 4, code % 4,
               count[code]);
        distinct++;
        printf(distinct % 4 == 0 ? "\n" : "   ");
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "%d permutacoes distintas (esperado 24)", distinct);
    check(distinct == 24, msg);
    snprintf(msg, sizeof(msg), "esperado %.0f cada; maior desvio %.2f%% (limite 5%%)",
             expected, 100.0 * max_dev);
    check(max_dev < 0.05, msg);

    population_free(pop);
}

static void test_circle_optimum(void)
{
    printf("\n3) Circulo: rota 0..N-1 = perimetro 2*N*R*sin(pi/N)\n");

    const int sizes[] = {8, 50, 500};
    const double radius = 100.0;
    char msg[160];

    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
    {
        const int n = sizes[s];
        Point *points = generate_circle_points(n, radius);
        int *route = malloc((size_t)n * sizeof(int));

        if (points == NULL || route == NULL)
        {
            check(0, "falta de memoria");
            free(points);
            free(route);
            continue;
        }

        for (int i = 0; i < n; i++)
        {
            route[i] = i;
        }

        const double got = route_distance(route, points, n);
        const double optimum = 2.0 * n * radius * sin(TEST_PI / n);

        snprintf(msg, sizeof(msg), "N=%d R=%.0f: calculado %.12f, formula %.12f, erro %.1e",
                 n, radius, got, optimum, fabs(got - optimum));
        check(fabs(got - optimum) < EPS, msg);

        free(points);
        free(route);
    }
}

static void test_square(void)
{
    printf("\n4) Quadrado unitario\n");

    const Point square[4] = {{0, 0.0, 0.0}, {1, 1.0, 0.0}, {2, 1.0, 1.0}, {3, 0.0, 1.0}};
    const int in_order[4] = {0, 1, 2, 3};
    const int crossed[4] = {0, 2, 1, 3};
    char msg[128];

    const double d1 = route_distance(in_order, square, 4);
    snprintf(msg, sizeof(msg), "rota 0,1,2,3 = %.12f (esperado 4)", d1);
    check(fabs(d1 - 4.0) < EPS, msg);

    const double d2 = route_distance(crossed, square, 4);
    const double expected = 2.0 + 2.0 * sqrt(2.0);
    snprintf(msg, sizeof(msg), "rota 0,2,1,3 = %.12f (esperado 2+2*sqrt(2) = %.12f)", d2,
             expected);
    check(fabs(d2 - expected) < EPS, msg);

    /* Sem a aresta de volta, a rota em ordem daria 3: confirma que ela entra. */
    check(fabs(route_distance(in_order, square, 3) - (1.0 + 1.0 + sqrt(2.0))) < EPS,
          "aresta de volta incluida (N=3 sobre os 3 primeiros pontos = 2+sqrt(2))");

    check(route_distance(in_order, square, 1) == 0.0, "N=1 retorna 0");

    Individual ind = {(int *)in_order, 0.0, 0.0};
    individual_evaluate(&ind, square, 4);
    check(fabs(ind.fitness - 0.25) < EPS, "individual_evaluate: fitness = 1/4 = 0.25");

    individual_evaluate(&ind, square, 1);
    check(ind.distance == 0.0 && ind.fitness == 0.0, "distancia zero -> fitness 0");
}

static void test_population_best(void)
{
    printf("\n5) population_best devolve o menor distance\n");

    const int n = 30;
    Point *points = generate_uniform_points(n, 0.0, 1000.0);
    Population *pop = population_create(200, n);

    if (points == NULL || pop == NULL)
    {
        check(0, "falta de memoria");
        free(points);
        population_free(pop);
        return;
    }

    population_evaluate_all(pop, points);

    const Individual *best = population_best(pop);
    int ok = best != NULL;

    for (int i = 0; ok && i < pop->size; i++)
    {
        if (pop->path[i].distance < best->distance)
        {
            ok = 0;
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "melhor = %.4f, nenhum dos 200 e menor", best ? best->distance : -1.0);
    check(ok, msg);

    /* O minimo tambem e o maximo de fitness. */
    int same_as_fitness = 1;
    for (int i = 0; best && i < pop->size; i++)
    {
        if (pop->path[i].fitness > best->fitness)
        {
            same_as_fitness = 0;
        }
    }
    check(same_as_fitness, "menor distance = maior fitness");

    check(population_best(NULL) == NULL, "population_best(NULL) retorna NULL");

    population_free(pop);
    free(points);
}

static void test_individual_copy(void)
{
    printf("\n6) individual_copy gera rota independente\n");

    const int n = 10;
    Point *points = generate_uniform_points(n, 0.0, 100.0);
    Population *pop = population_create(2, n);

    if (points == NULL || pop == NULL)
    {
        check(0, "falta de memoria");
        free(points);
        population_free(pop);
        return;
    }

    population_evaluate_all(pop, points);

    Individual *src = &pop->path[0];
    Individual *dst = &pop->path[1];
    individual_copy(dst, src, n);

    int equal = dst->distance == src->distance && dst->fitness == src->fitness;
    for (int i = 0; i < n; i++)
    {
        equal = equal && dst->route[i] == src->route[i];
    }
    check(equal, "copia tem a mesma rota, distance e fitness");
    check(dst->route != src->route, "ponteiros de rota diferentes (nao copiou o ponteiro)");

    const int original_first = src->route[0];
    dst->route[0] = dst->route[1];
    dst->route[1] = original_first;
    check(src->route[0] == original_first, "alterar a copia nao altera o original");

    population_free(pop);
    free(points);
}

static void test_memory(void)
{
    printf("\n7) Memoria: create/free em loop e entradas invalidas\n");

    for (int i = 0; i < 1000; i++)
    {
        Population *pop = population_create(1 + i % 50, 1 + i % 40);
        population_free(pop);
    }
    check(1, "1000 ciclos create/free (vazamento seria acusado pelo ASan na saida)");

    population_free(NULL);
    check(1, "population_free(NULL) nao quebra");

    check(population_create(0, 10) == NULL, "pop_size = 0 -> NULL");
    check(population_create(10, 0) == NULL, "num_points = 0 -> NULL");
    check(population_create(-5, 10) == NULL, "pop_size negativo -> NULL");
    check(generate_uniform_points(0, 0.0, 1.0) == NULL, "generate_uniform_points(0) -> NULL");
    check(generate_circle_points(-1, 1.0) == NULL, "generate_circle_points(-1) -> NULL");
}

/* Ordem crescente de distance, para o qsort. */
static int compare_distance(const void *a, const void *b)
{
    const double da = ((const Individual *)a)->distance;
    const double db = ((const Individual *)b)->distance;

    return (da > db) - (da < db);
}

static void test_sorted_population(void)
{
    printf("\n8) Populacao reordenada com qsort (ex.: selecao por Ranking)\n");

    const int n = 25;
    Point *points = generate_uniform_points(n, 0.0, 1000.0);
    Population *pop = population_create(100, n);

    if (points == NULL || pop == NULL)
    {
        check(0, "falta de memoria");
        free(points);
        population_free(pop);
        return;
    }

    population_evaluate_all(pop, points);
    qsort(pop->path, (size_t)pop->size, sizeof(Individual), compare_distance);

    /* Confirma que o qsort realmente tirou as rotas da ordem do bloco: e
       exatamente o caso em que liberar via path[0].route quebraria. */
    int moved = 0;
    for (int i = 0; i < pop->size; i++)
    {
        if (pop->path[i].route != pop->route_block + (size_t)i * (size_t)n)
        {
            moved = 1;
        }
    }
    check(moved, "qsort moveu as rotas: path[0].route ja nao e o inicio do bloco");

    int sorted = 1;
    for (int i = 1; i < pop->size; i++)
    {
        sorted = sorted && pop->path[i - 1].distance <= pop->path[i].distance;
    }
    check(sorted, "path ordenado por distance");
    check(population_best(pop) == &pop->path[0], "population_best == &path[0] apos ordenar");

    population_free(pop);
    check(1, "population_free apos qsort (invalid free seria acusado pelo ASan)");
    free(points);
}

static void test_tournament_integration(void)
{
    printf("\n9) Integracao: torneio (Pessoa 1) sobre populacao avaliada (Pessoa 2)\n");

    const int n = 20;
    const double radius = 100.0;
    Point *points = generate_circle_points(n, radius);
    Population *pop = population_create(100, n);

    if (points == NULL || pop == NULL)
    {
        check(0, "falta de memoria");
        free(points);
        population_free(pop);
        return;
    }

    population_evaluate_all(pop, points);

    double pop_mean = 0.0;
    for (int i = 0; i < pop->size; i++)
    {
        pop_mean += pop->path[i].distance;
    }
    pop_mean /= pop->size;

    const int rounds = 10000;
    double winners_mean = 0.0;
    int inside = 1;

    for (int r = 0; r < rounds; r++)
    {
        const Individual *w = tournament_select(pop, TOURNAMENT_SIZE);

        if (w == NULL || w < pop->path || w >= pop->path + pop->size)
        {
            inside = 0;
            break;
        }

        winners_mean += w->distance;
    }
    winners_mean /= rounds;

    char msg[160];
    check(inside, "torneio sempre devolve um individuo de dentro da populacao");
    snprintf(msg, sizeof(msg), "media dos vencedores %.2f < media da populacao %.2f",
             winners_mean, pop_mean);
    check(winners_mean < pop_mean, msg);

    /* Mini-AG de demonstracao: torneio + OX1 + inversao + elitismo com
       individual_copy, usando duas populacoes que trocam de papel. */
    Population *next = population_create(pop->size, n);
    if (next == NULL)
    {
        check(0, "falta de memoria");
        population_free(pop);
        free(points);
        return;
    }

    const double optimum = 2.0 * n * radius * sin(TEST_PI / n);
    const double start_best = population_best(pop)->distance;
    double previous = start_best;
    int monotonic = 1;
    int all_valid = 1;

    for (int gen = 0; gen < 500; gen++)
    {
        individual_copy(&next->path[0], population_best(pop), n);

        for (int i = 1; i < next->size; i++)
        {
            const Individual *a = tournament_select(pop, TOURNAMENT_SIZE);
            const Individual *b = tournament_select(pop, TOURNAMENT_SIZE);

            order_crossover(a->route, b->route, next->path[i].route, n);
            inversion_mutation(next->path[i].route, n, MUTATION_RATE);
            all_valid = all_valid && route_is_valid(next->path[i].route, n);
        }

        population_evaluate_all(next, points);

        Population *swap = pop;
        pop = next;
        next = swap;

        const double best = population_best(pop)->distance;
        if (best > previous + EPS)
        {
            monotonic = 0;
        }
        previous = best;
    }

    check(all_valid, "500 geracoes: todo filho e permutacao valida");
    check(monotonic, "com elitismo o melhor nunca piora");
    /* fabs: quando o AG acha o otimo, o arredondamento pode dar -0.00%. */
    snprintf(msg, sizeof(msg), "N=%d circulo: inicial %.2f -> final %.2f, otimo %.2f, gap %.2f%%",
             n, start_best, previous, optimum, 100.0 * fabs(previous - optimum) / optimum);
    check(previous < start_best, msg);

    population_free(pop);
    population_free(next);
    free(points);
}

int main(void)
{
    /* Semente fixa: o teste e reproduzivel. */
    srand(12345);

    printf("=== Testes Pessoa 2: populacao, memoria e avaliacao ===\n");

    test_routes_are_valid();
    test_fisher_yates_uniform();
    test_circle_optimum();
    test_square();
    test_population_best();
    test_individual_copy();
    test_memory();
    test_sorted_population();
    test_tournament_integration();

    printf("\n=== %d verificacoes, %d falhas ===\n", checks, failures);

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
