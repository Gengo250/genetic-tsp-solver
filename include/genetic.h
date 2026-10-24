#ifndef GENETIC_H
#define GENETIC_H

#include "individual.h"
#include "population.h"

/* Parametros do motor genetico (justificativa em DEVELOPMENT.md). */
#define TOURNAMENT_SIZE 3
#define MUTATION_RATE 0.05

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
