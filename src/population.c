#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "individual.h"
#include "point.h"
#include "population.h"

/* Abaixo disso a distancia e tratada como zero (todos os pontos coincidem). */
#define MIN_DISTANCE 1e-12

/* Embaralhamento de Fisher-Yates (versao de Durstenfeld).
   Percorre a rota de tras para frente: na posicao i sorteia j em [0, i] e
   troca route[i] com route[j]. A posicao N-1 tem N escolhas, a N-2 tem N-1,
   ..., a posicao 1 tem 2 escolhas. Sao N * (N-1) * ... * 2 = N! sequencias de
   sorteio possiveis, e cada uma leva a uma permutacao diferente. Logo cada uma
   das N! permutacoes sai com probabilidade exatamente 1/N!.

   Por que nao "sortear dois indices quaisquer e trocar, N vezes"? Esse metodo
   tem N^(2N) sequencias de sorteio igualmente provaveis, e N^(2N) nao e
   multiplo de N! (ex.: N=3 -> 729 sequencias para 6 permutacoes). Nao da para
   dividir 729 igualmente por 6, entao algumas permutacoes saem mais que outras:
   o embaralhamento fica viciado.

   Observacao: rand() % (i+1) tem um vies minusculo quando RAND_MAX+1 nao e
   multiplo de i+1. Para as populacoes do trabalho isso e desprezivel. */
static void shuffle_route(int *route, int num_points)
{
    for (int i = num_points - 1; i >= 1; i--)
    {
        const int j = rand() % (i + 1);
        const int temp = route[i];

        route[i] = route[j];
        route[j] = temp;
    }
}

Population *population_create(int pop_size, int num_points)
{
    if (pop_size < 1 || num_points < 1)
    {
        return NULL;
    }

    /* Evita estouro em pop_size * num_points no tamanho do bloco. */
    if ((size_t)num_points > SIZE_MAX / sizeof(int) / (size_t)pop_size)
    {
        return NULL;
    }

    /* Memoria em 3 alocacoes: a struct, o vetor de individuos e UM bloco unico
       com todas as rotas (pop_size * num_points inteiros). Motivos:
       - sao 3 mallocs e 3 frees, em vez de pop_size + 2;
       - se um malloc falhar no meio, ha no maximo 2 coisas para desfazer, sem
         risco de deixar metade das rotas alocadas (vazamento parcial);
       - as rotas ficam contiguas na memoria, o que aproveita melhor o cache
         quando a avaliacao percorre a populacao inteira. */
    Population *pop = malloc(sizeof(Population));

    if (pop == NULL)
    {
        return NULL;
    }

    pop->path = malloc((size_t)pop_size * sizeof(Individual));

    if (pop->path == NULL)
    {
        free(pop);
        return NULL;
    }

    int *routes = malloc((size_t)pop_size * (size_t)num_points * sizeof(int));

    if (routes == NULL)
    {
        free(pop->path);
        free(pop);
        return NULL;
    }

    pop->size = pop_size;
    pop->num_points = num_points;
    pop->route_block = routes;

    for (int i = 0; i < pop_size; i++)
    {
        Individual *ind = &pop->path[i];

        /* O individuo i usa a fatia [i * num_points, (i+1) * num_points) do bloco. */
        ind->route = routes + (size_t)i * (size_t)num_points;
        ind->distance = 0.0;
        ind->fitness = 0.0;

        /* Comeca na ordem 0..N-1 e embaralha. */
        for (int k = 0; k < num_points; k++)
        {
            ind->route[k] = k;
        }

        shuffle_route(ind->route, num_points);
    }

    return pop;
}

void population_free(Population *pop)
{
    if (pop == NULL)
    {
        return;
    }

    /* Um free libera todas as rotas. Usamos route_block, e nao path[0].route,
       porque se `path` for reordenado (ex.: qsort na selecao por Ranking) o
       path[0] passa a apontar para o meio do bloco, e free nesse endereco e
       comportamento indefinido. */
    free(pop->route_block);
    free(pop->path);
    free(pop);
}


int evaluate_population(Population *pop, const Point *points, int num_points)
{
    if (pop == NULL || pop->path == NULL || points == NULL || pop->size <= 0 ||
        num_points <= 0 || pop->num_points != num_points)
    {
        return 0;
    }

    char *seen = calloc((size_t)num_points, sizeof(*seen));
    if (seen == NULL)
    {
        return 0;
    }

    for (int i = 0; i < pop->size; i++)
    {
        if (pop->path[i].route == NULL)
        {
            free(seen);
            return 0;
        }

        memset(seen, 0, (size_t)num_points * sizeof(*seen));
        for (int j = 0; j < num_points; j++)
        {
            const int point_index = pop->path[i].route[j];
            if (point_index < 0 || point_index >= num_points || seen[point_index])
            {
                free(seen);
                return 0;
            }
            seen[point_index] = 1;
        }
    }
    free(seen);

    for (int i = 0; i < pop->size; i++)
    {
        individual_evaluate(&pop->path[i], points, num_points);
    }

    return 1;
}

double route_distance(const int *route, const Point *points, int num_points)
{
    if (route == NULL || points == NULL || num_points < 2)
    {
        return 0.0;
    }

    double total = 0.0;

    for (int i = 0; i < num_points - 1; i++)
    {
        const Point *a = &points[route[i]];
        const Point *b = &points[route[i + 1]];

        total += hypot(b->x - a->x, b->y - a->y);
    }

    /* ATENCAO: aresta de volta route[N-1] -> route[0].
       O caminho do caixeiro e um CICLO: ele volta para a cidade de partida.
       Sem essa aresta estariamos medindo um caminho aberto e o AG otimizaria
       outro problema. */
    const Point *last = &points[route[num_points - 1]];
    const Point *first = &points[route[0]];

    total += hypot(first->x - last->x, first->y - last->y);

    return total;
}

void individual_evaluate(Individual *ind, const Point *points, int num_points)
{
    if (ind == NULL)
    {
        return;
    }

    ind->distance = route_distance(ind->route, points, num_points);

    /* Distancia ~0 so acontece com N < 2 ou com todos os pontos no mesmo
       lugar: ai toda rota tem o mesmo tamanho e nenhuma e melhor que outra.
       Usamos fitness = 0 em vez de um valor gigante para nao dividir por zero
       e nao gerar infinito/valores absurdos em medias ou graficos. */
    if (ind->distance < MIN_DISTANCE)
    {
        ind->fitness = 0.0;
    }
    else
    {
        ind->fitness = 1.0 / ind->distance;
    }
}

void population_evaluate_all(Population *pop, const Point *points)
{
    if (pop == NULL || pop->path == NULL)
    {
        return;
    }

    for (int i = 0; i < pop->size; i++)
    {
        individual_evaluate(&pop->path[i], points, pop->num_points);
    }
}

const Individual *population_best(const Population *pop)
{
    if (pop == NULL || pop->path == NULL || pop->size <= 0)
    {
        return NULL;
    }

    const Individual *best = &pop->path[0];

    for (int i = 1; i < pop->size; i++)
    {
        /* Mesmo criterio do torneio: vence o caminho mais curto. */
        if (pop->path[i].distance < best->distance)
        {
            best = &pop->path[i];
        }
    }

    return best;
}

void individual_copy(Individual *dst, const Individual *src, int num_points)
{
    if (dst == NULL || src == NULL || dst == src || num_points < 1)
    {
        return;
    }

    /* Copia o CONTEUDO da rota, nunca o ponteiro. Se fizessemos
       dst->route = src->route, os dois individuos dividiriam a mesma memoria:
       mudar um mudaria o outro (aliasing) e a rota seria liberada duas vezes
       (double free). */
    memcpy(dst->route, src->route, (size_t)num_points * sizeof(int));
    dst->distance = src->distance;
    dst->fitness = src->fitness;
}
