#ifndef POPULATION_H
#define POPULATION_H

#include "individual.h"
#include "point.h"

/* path        = vetor com os `size` individuos da populacao (path[0..size-1])
   size        = quantidade de individuos na populacao
   num_points  = quantidade de pontos do mapa, ou seja, o tamanho de cada rota
   route_block = inicio do bloco de rotas; e o unico ponteiro que deve ser liberado */
typedef struct {
    Individual *path;
    int size;
    int num_points;
    int *route_block;
} Population;

/* Cria uma populacao com `pop_size` individuos, cada um com uma rota aleatoria
   (Fisher-Yates) sobre os pontos 0..num_points-1. `distance` e `fitness`
   comecam em 0: chame population_evaluate_all antes de selecionar.
   Todas as rotas ficam num unico bloco contiguo de memoria.
   Nao chama srand: a semente e responsabilidade de quem chama.
   Retorna NULL se pop_size < 1, num_points < 1 ou se faltar memoria. */
Population *population_create(int pop_size, int num_points);

/* Libera a populacao inteira (route_block, vetor path e a struct).
   Aceita NULL sem fazer nada. Continua correto mesmo se `path` tiver sido
   reordenado (ex.: qsort) ou se ponteiros `route` tiverem sido trocados. */
void population_free(Population *pop);

/* Distancia euclidiana total do ciclo descrito por `route`, INCLUINDO a aresta
   de volta route[num_points-1] -> route[0]. Retorna 0 para num_points < 2.
   Assume points[k].id == k (garantido pelos geradores). */
double route_distance(const int *route, const Point *points, int num_points);

/* Preenche `distance` (via route_distance) e `fitness` = 1 / distance.
   Distancia praticamente zero (< 1e-12) gera fitness = 0. */
void individual_evaluate(Individual *ind, const Point *points, int num_points);

/* Avalia todos os individuos da populacao (usa pop->num_points). */
void population_evaluate_all(Population *pop, const Point *points);

/* Busca linear pelo individuo de menor `distance` (mesmo criterio do torneio).
   Exige populacao ja avaliada. Retorna NULL se a populacao for invalida.
   O ponteiro aponta para dentro da populacao: nao deve ser liberado. */
const Individual *population_best(const Population *pop);

/* Copia profunda: copia o CONTEUDO da rota (memcpy) e os campos distance e
   fitness. Nunca copia o ponteiro `route`: `dst` precisa ter sua propria rota
   com espaco para `num_points` inteiros (ex.: outro individuo da populacao). */
void individual_copy(Individual *dst, const Individual *src, int num_points);

#endif /* POPULATION_H */
