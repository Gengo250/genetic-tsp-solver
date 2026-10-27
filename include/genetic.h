#ifndef GENETIC_H
#define GENETIC_H

#include "population.h"

#define POPULATION_SIZE 100
#define TOURNAMENT_SIZE 3
#define ELITISM_COUNT 1
#define CROSSOVER_RATE_INITIAL 1.0
#define CROSSOVER_RATE_FINAL 0.75
#define MUTATION_RATE_INITIAL 0.1
#define MUTATION_RATE_FINAL 0.02
#define MUTATION_RATE 0.05

const Individual *tournament_select(const Population *population, int tournament_size);

void order_crossover(const int *parent_a, const int *parent_b, int *child, int num_points);
void inversion_mutation(int *route, int num_points, double rate);
int route_is_valid(const int *route, int num_points);

double crossover_rate_for_generation(int current_generation, int max_generations);
double mutation_rate_for_generation(int current_generation, int max_generations);
Population *evolve_population(const Population *current_pop, const Point *points,
                              int num_points, int current_generation, int max_generations);

#endif /* GENETIC_H */