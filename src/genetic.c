#include <stdlib.h>

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
