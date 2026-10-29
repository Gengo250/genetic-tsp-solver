#ifndef GENETIC_H
#define GENETIC_H

#include "population.h"

/* Populacao: piso + uma fatia por ponto. Parametro fixo nao serve, porque o
   espaco de busca cresce com N: com 100 individuos fixos o AG chega ao otimo
   ate N=50 e empaca a partir de N=100. Use population_size_for(). */
#define POPULATION_MIN 150
#define POPULATION_PER_POINT 2

/* k = 5: pressao seletiva moderada. k menor explora demais (quase aleatorio);
   k maior explota demais e a populacao converge cedo. Elitismo 1: so o
   melhor passa direto, o suficiente para nunca perder a melhor rota. */
#define TOURNAMENT_SIZE 5
#define ELITISM_COUNT 1

/* Taxas constantes. O decaimento ao longo da execucao foi medido e piora o
   resultado nos dois operadores: ele reduz justamente a exploracao de que o
   AG precisa quando N e grande. A interpolacao continua existindo em
   rate_for_generation() para quem quiser reativar o decaimento.
   Cruzamento 100%: o OX1 sempre gera rota valida e o elitismo ja guarda o
   melhor. Mutacao 30%: alta para manter exploracao em mapas grandes. */
#define CROSSOVER_RATE_INITIAL 1.0
#define CROSSOVER_RATE_FINAL 1.0
#define MUTATION_RATE_INITIAL 0.30
#define MUTATION_RATE_FINAL 0.30
#define MUTATION_RATE 0.30

/* Criterio de parada duplo, compartilhado pelo console e pela GUI para que as
   duas execucoes descrevam o mesmo algoritmo. A estagnacao tambem escala com
   N: mapa maior precisa de mais paciencia antes de declarar convergencia.
   Para com max(150, 2*N) epocas sem melhora OU no teto max(1000, 20*N). */
#define STAGNATION_MIN 150
#define STAGNATION_PER_POINT 2
#define GENERATIONS_PER_POINT 20
#define MIN_MAX_GENERATIONS 1000

const Individual *tournament_select(const Population *population, int tournament_size);

void order_crossover(const int *parent_a, const int *parent_b, int *child, int num_points);
void inversion_mutation(int *route, int num_points, double rate);
int route_is_valid(const int *route, int num_points);

double crossover_rate_for_generation(int current_generation, int max_generations);
double mutation_rate_for_generation(int current_generation, int max_generations);

/* Parametros que dependem do tamanho do mapa. */
int population_size_for(int num_points);
int stagnation_limit_for(int num_points);
int max_generations_for(int num_points);
Population *evolve_population(const Population *current_pop, const Point *points,
                              int num_points, int current_generation, int max_generations);

#endif /* GENETIC_H */