#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "genetic.h"
#include "individual.h"
#include "population.h"

/* Sorteia um inteiro no intervalo fechado [min, max].
   Centraliza o controle de limites: nenhum indice gerado aqui escapa do
   intervalo pedido, mesmo quando min e max chegam invertidos ou iguais. */
static int random_range(int min, int max)
{
    if (max <= min)
    {
        return min;
    }

    return min + rand() % (max - min + 1);
}

/* Sorteia um real em [0, 1). */
static double random_unit(void)
{
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

static Population *population_allocate(int population_size, int num_points)
{
    if (population_size <= 0 || num_points <= 0)
    {
        return NULL;
    }

    Population *population = malloc(sizeof(*population));
    if (population == NULL)
    {
        return NULL;
    }

    population->path = calloc((size_t)population_size, sizeof(*population->path));
    if (population->path == NULL)
    {
        free(population);
        return NULL;
    }
    population->size = population_size;

    for (int i = 0; i < population_size; i++)
    {
        population->path[i].route = malloc((size_t)num_points * sizeof(int));
        if (population->path[i].route == NULL)
        {
            population_free(population);
            return NULL;
        }
    }

    return population;
}

double crossover_rate_for_generation(int generation, int max_generations)
{
    double progress = 1.0;
    if (max_generations > 0)
    {
        progress = (double)generation / (double)max_generations;
        if (progress < 0.0)
            progress = 0.0;
        if (progress > 1.0)
            progress = 1.0;
    }

    return CROSSOVER_RATE_INITIAL +
           (CROSSOVER_RATE_FINAL - CROSSOVER_RATE_INITIAL) * progress;
}

double mutation_rate_for_generation(int generation, int max_generations)
{
    double progress = 1.0;
    if (max_generations > 0)
    {
        progress = (double)generation / (double)max_generations;
        if (progress < 0.0)
            progress = 0.0;
        if (progress > 1.0)
            progress = 1.0;
    }

    return MUTATION_RATE_INITIAL +
           (MUTATION_RATE_FINAL - MUTATION_RATE_INITIAL) * progress;
}

Population *population_create(int population_size, int num_points)
{
    Population *population = population_allocate(population_size, num_points);
    if (population == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < population_size; i++)
    {
        int *route = population->path[i].route;
        for (int point = 0; point < num_points; point++)
        {
            route[point] = point;
        }

        for (int point = num_points - 1; point > 0; point--)
        {
            const int other = random_range(0, point);
            const int temp = route[point];
            route[point] = route[other];
            route[other] = temp;
        }
    }

    return population;
}

void population_free(Population *population)
{
    if (population == NULL)
    {
        return;
    }

    if (population->path != NULL)
    {
        for (int i = 0; i < population->size; i++)
        {
            free(population->path[i].route);
        }
        free(population->path);
    }
    free(population);
}

int evaluate_population(Population *population, const Point *points, int num_points)
{
    if (population == NULL || population->path == NULL || population->size <= 0 ||
        points == NULL || num_points <= 0)
    {
        return 0;
    }

    for (int individual = 0; individual < population->size; individual++)
    {
        const int *route = population->path[individual].route;
        if (!route_is_valid(route, num_points))
        {
            return 0;
        }

        double distance = 0.0;
        for (int point = 0; point < num_points; point++)
        {
            const Point *from = &points[route[point]];
            const Point *to = &points[route[(point + 1) % num_points]];
            const double dx = to->x - from->x;
            const double dy = to->y - from->y;
            distance += sqrt(dx * dx + dy * dy);
        }

        population->path[individual].distance = distance;
        population->path[individual].fitness =
            distance > 0.0 ? 1.0 / distance : HUGE_VAL;
    }

    return 1;
}

const Individual *population_best(const Population *population)
{
    if (population == NULL || population->path == NULL || population->size <= 0)
    {
        return NULL;
    }

    const Individual *best = &population->path[0];
    for (int i = 1; i < population->size; i++)
    {
        if (population->path[i].distance < best->distance)
        {
            best = &population->path[i];
        }
    }

    return best;
}

Population *evolve_population(const Population *current, const Point *points,
                              int num_points, int generation, int max_generations)
{
    if (current == NULL || current->path == NULL || current->size <= 0 ||
        points == NULL || num_points <= 0)
    {
        return NULL;
    }

    const Individual *elite = population_best(current);
    if (elite == NULL)
    {
        return NULL;
    }

    Population *next = population_allocate(current->size, num_points);
    if (next == NULL)
    {
        return NULL;
    }

    const double crossover_rate = crossover_rate_for_generation(generation, max_generations);
    const double mutation_rate = mutation_rate_for_generation(generation, max_generations);
    int first_child = 0;

    for (int i = 0; i < ELITISM_COUNT && i < next->size; i++)
    {
        memcpy(next->path[i].route, elite->route, (size_t)num_points * sizeof(int));
        first_child++;
    }

    for (int i = first_child; i < next->size; i++)
    {
        const Individual *parent_a = tournament_select(current, TOURNAMENT_SIZE);
        const Individual *parent_b = tournament_select(current, TOURNAMENT_SIZE);
        if (parent_a == NULL || parent_b == NULL)
        {
            population_free(next);
            return NULL;
        }

        int *child = next->path[i].route;
        if (random_unit() < crossover_rate)
        {
            order_crossover(parent_a->route, parent_b->route, child, num_points);
        }
        else
        {
            memcpy(child, parent_a->route, (size_t)num_points * sizeof(int));
        }

        inversion_mutation(child, num_points, mutation_rate);
    }

    if (!evaluate_population(next, points, num_points))
    {
        population_free(next);
        return NULL;
    }

    return next;
}

const Individual *tournament_select(const Population *population, int tournament_size)
{
    if (population == NULL || population->path == NULL || population->size <= 0)
    {
        return NULL;
    }

    /* Torneio maior que a populacao nao faz sentido: so repetiria competidores. */
    if (tournament_size < 1)
    {
        tournament_size = 1;
    }
    if (tournament_size > population->size)
    {
        tournament_size = population->size;
    }

    const Individual *best = &population->path[random_range(0, population->size - 1)];

    for (int i = 1; i < tournament_size; i++)
    {
        const Individual *challenger =
            &population->path[random_range(0, population->size - 1)];

        /* Vence quem tiver o caminho mais curto. */
        if (challenger->distance < best->distance)
        {
            best = challenger;
        }
    }

    return best;
}

void order_crossover(const int *parent_a, const int *parent_b, int *child, int num_points)
{
    if (parent_a == NULL || parent_b == NULL || child == NULL || num_points <= 0)
    {
        return;
    }

    if (num_points == 1)
    {
        child[0] = parent_a[0];
        return;
    }

    /* used[gene] marca os genes que ja entraram no filho. Como o gene e o
       proprio indice do ponto (0..num_points-1), a checagem de repeticao fica
       O(1) em vez de varrer o filho inteiro a cada insercao. */
    char *used = calloc((size_t)num_points, sizeof(char));

    if (used == NULL)
    {
        /* Sem a tabela auxiliar nao da para garantir unicidade na mistura;
           copiar o pai A ainda devolve uma rota valida. */
        for (int i = 0; i < num_points; i++)
        {
            child[i] = parent_a[i];
        }
        return;
    }

    const int start = random_range(0, num_points - 1);
    const int end = random_range(start, num_points - 1);

    /* [1] Segmento do pai A: mesma posicao e mesma ordem. */
    for (int i = start; i <= end; i++)
    {
        const int gene = parent_a[i];

        child[i] = gene;

        if (gene >= 0 && gene < num_points)
        {
            used[gene] = 1;
        }
    }

    /* [2] Resto na ordem do pai B, lendo e escrevendo de forma circular a partir
       de end + 1 e pulando o que o segmento ja trouxe. */
    int write = (end + 1) % num_points;

    for (int step = 0; step < num_points; step++)
    {
        const int read = (end + 1 + step) % num_points;
        const int gene = parent_b[read];

        if (gene < 0 || gene >= num_points || used[gene])
        {
            continue;
        }

        child[write] = gene;
        used[gene] = 1;
        write = (write + 1) % num_points;
    }

    /* [3] Rede de seguranca: se o pai B vier corrompido (gene fora dos limites
       ou repetido), sobram posicoes sem preencher. Fecha a rota com os genes que
       ainda nao foram usados para que o filho nunca saia invalido. */
    for (int gene = 0; gene < num_points && write != start; gene++)
    {
        if (used[gene])
        {
            continue;
        }

        child[write] = gene;
        used[gene] = 1;
        write = (write + 1) % num_points;
    }

    free(used);
}

void inversion_mutation(int *route, int num_points, double rate)
{
    if (route == NULL || num_points < 2 || rate <= 0.0)
    {
        return;
    }

    if (random_unit() >= rate)
    {
        return;
    }

    int start = random_range(0, num_points - 1);
    int end = random_range(0, num_points - 1);

    if (start > end)
    {
        const int swap = start;
        start = end;
        end = swap;
    }

    /* Inverte o sub-vetor [start, end]. Isso desfaz cruzamentos de arestas no
       mapa, que sempre indicam sub-otimalidade no TSP. */
    while (start < end)
    {
        const int temp = route[start];

        route[start] = route[end];
        route[end] = temp;

        start++;
        end--;
    }
}

int route_is_valid(const int *route, int num_points)
{
    if (route == NULL || num_points <= 0)
    {
        return 0;
    }

    char *seen = calloc((size_t)num_points, sizeof(char));

    if (seen == NULL)
    {
        return 0;
    }

    int valid = 1;

    for (int i = 0; i < num_points; i++)
    {
        const int gene = route[i];

        if (gene < 0 || gene >= num_points || seen[gene])
        {
            valid = 0;
            break;
        }

        seen[gene] = 1;
    }

    free(seen);

    return valid;
}
