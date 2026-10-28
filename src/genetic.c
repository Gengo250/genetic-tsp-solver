#include <stdlib.h>

#include "genetic.h"
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

static double rate_for_generation(int current_generation, int max_generations,
                                  double initial_rate, double final_rate)
{
    if (max_generations <= 0)
    {
        return final_rate;
    }

    if (current_generation <= 0)
    {
        return initial_rate;
    }
    if (current_generation >= max_generations)
    {
        return final_rate;
    }

    const double progress = (double)current_generation / (double)max_generations;
    return initial_rate + progress * (final_rate - initial_rate);
}

double crossover_rate_for_generation(int current_generation, int max_generations)
{
    return rate_for_generation(current_generation, max_generations,
                               CROSSOVER_RATE_INITIAL, CROSSOVER_RATE_FINAL);
}

double mutation_rate_for_generation(int current_generation, int max_generations)
{
    return rate_for_generation(current_generation, max_generations,
                               MUTATION_RATE_INITIAL, MUTATION_RATE_FINAL);
}

/* Piso mais uma fatia por ponto. O piso serve aos mapas pequenos, onde o
   termo proporcional seria baixo demais para formar diversidade. */
static int floor_plus_share(int minimum, int per_point, int num_points)
{
    if (num_points < 1)
    {
        return minimum;
    }

    const int proportional = num_points * per_point;

    return (proportional > minimum) ? proportional : minimum;
}

/* Populacao proporcional ao numero de pontos. Com populacao fixa o AG perde
   diversidade antes de varrer o espaco: medido no benchmark circular, 100
   individuos fixos dao gap de 67% em N=300, e 2*N da gap 0%. */
int population_size_for(int num_points)
{
    return floor_plus_share(POPULATION_MIN, POPULATION_PER_POINT, num_points);
}

/* Paciencia antes de declarar convergencia. Mapa maior demora mais para dar o
   proximo passo de melhora; cortar cedo congela uma rota ainda ruim. */
int stagnation_limit_for(int num_points)
{
    return floor_plus_share(STAGNATION_MIN, STAGNATION_PER_POINT, num_points);
}

/* Teto de epocas proporcional ao tamanho do mapa: mais pontos, mais espaco de
   busca. O piso evita que um mapa pequeno encerre antes de o AG trabalhar. */
int max_generations_for(int num_points)
{
    return floor_plus_share(MIN_MAX_GENERATIONS, GENERATIONS_PER_POINT, num_points);
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

Population *evolve_population(const Population *current_pop, const Point *points,
                              int num_points, int current_generation, int max_generations)
{
    if (current_pop == NULL || current_pop->path == NULL || current_pop->size < 1 ||
        points == NULL || num_points < 1 || current_pop->num_points != num_points)
    {
        return NULL;
    }

    Population *next_pop = population_create(current_pop->size, num_points);
    if (next_pop == NULL)
    {
        return NULL;
    }

    const Individual *best = population_best(current_pop);
    if (best == NULL || best->route == NULL)
    {
        population_free(next_pop);
        return NULL;
    }

    int elite_count = ELITISM_COUNT;
    if (elite_count > next_pop->size)
    {
        elite_count = next_pop->size;
    }
    for (int i = 0; i < elite_count; i++)
    {
        individual_copy(&next_pop->path[i], best, num_points);
    }

    const double crossover_rate =
        crossover_rate_for_generation(current_generation, max_generations);
    const double mutation_rate =
        mutation_rate_for_generation(current_generation, max_generations);

    for (int i = elite_count; i < next_pop->size; i++)
    {
        const Individual *parent_a = tournament_select(current_pop, TOURNAMENT_SIZE);
        const Individual *parent_b = tournament_select(current_pop, TOURNAMENT_SIZE);
        if (parent_a == NULL || parent_b == NULL ||
            parent_a->route == NULL || parent_b->route == NULL)
        {
            population_free(next_pop);
            return NULL;
        }

        if (random_unit() < crossover_rate)
        {
            order_crossover(parent_a->route, parent_b->route,
                            next_pop->path[i].route, num_points);
        }
        else
        {
            for (int point = 0; point < num_points; point++)
            {
                next_pop->path[i].route[point] = parent_a->route[point];
            }
        }
        inversion_mutation(next_pop->path[i].route, num_points, mutation_rate);
    }

    if (!evaluate_population(next_pop, points, num_points))
    {
        population_free(next_pop);
        return NULL;
    }

    return next_pop;
}

void inversion_mutation(int *route, int num_points, double rate)
{
    if (route == NULL || num_points < 2 || rate <= 0.0)
    {
        return;
    }

    if (rate < 1.0 && random_unit() >= rate)
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
