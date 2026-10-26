#ifndef GENETIC_H
#define GENETIC_H

#include "individual.h"
#include "point.h"
#include "population.h"

/* Parametros do motor genetico (justificativa em DEVELOPMENT.md). */
#define TOURNAMENT_SIZE 3
#define POPULATION_SIZE 100
#define MAX_GENERATIONS 1000
#define STAGNATION_LIMIT 50
#define ELITISM_COUNT 1

#define CROSSOVER_RATE_INITIAL 0.80
#define CROSSOVER_RATE_FINAL 0.95
#define MUTATION_RATE_INITIAL 0.10
#define MUTATION_RATE_FINAL 0.02

double crossover_rate_for_generation(int generation, int max_generations);
double mutation_rate_for_generation(int generation, int max_generations);

/* Cria uma populacao com rotas aleatorias e libera toda a memoria associada. */
Population *population_create(int population_size, int num_points);
void population_free(Population *population);

/* Avalia rotas como ciclos usando os pontos indexados por cada gene. */
int evaluate_population(Population *population, const Point *points, int num_points);

/* Retorna o individuo de menor distancia, ou NULL para populacao invalida. */
const Individual *population_best(const Population *population);

/* Cria e avalia a proxima geracao a partir de uma populacao ja avaliada. */
Population *evolve_population(const Population *current, const Point *points,
                              int num_points, int generation, int max_generations);

/* Selecao por torneio.
   Sorteia `tournament_size` individuos ao acaso (com reposicao) e devolve o que
   tiver o caminho mais curto, ou seja, o menor `distance`. Exige que a populacao
   ja tenha sido avaliada (`distance` preenchido).
   `tournament_size` e ajustado para o intervalo [1, population->size].
   Retorna NULL se a populacao for invalida ou vazia. O ponteiro devolvido aponta
   para dentro da populacao: nao deve ser liberado nem modificado. */
const Individual *tournament_select(const Population *population, int tournament_size);

/* Order Crossover (OX1).
   Copia um segmento aleatorio [start, end] de `parent_a` para a mesma posicao em
   `child` e completa as posicoes restantes com a ordem de `parent_b`, varrendo
   de forma circular a partir de end + 1 e pulando genes ja usados.
   O filho e sempre uma permutacao valida de 0..num_points-1: sem cidade repetida
   e sem cidade faltando. `child` deve ter espaco para `num_points` inteiros. */
void order_crossover(const int *parent_a, const int *parent_b, int *child, int num_points);

/* Mutacao por inversao.
   Com probabilidade `rate` (0.0 a 1.0), sorteia dois indices dentro de
   [0, num_points-1] e inverte o sub-vetor entre eles. Como a operacao apenas
   reordena genes existentes, a rota continua sendo uma permutacao valida. */
void inversion_mutation(int *route, int num_points, double rate);

/* Validacao defensiva de gene: 1 quando `route` e uma permutacao de
   0..num_points-1 (sem duplicados e sem indice fora dos limites), 0 caso
   contrario. Util em testes e asserts durante o desenvolvimento. */
int route_is_valid(const int *route, int num_points);

#endif /* GENETIC_H */
