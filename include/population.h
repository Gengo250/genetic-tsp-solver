#ifndef POPULATION_H
#define POPULATION_H

#include "individual.h"
#include "point.h"

typedef struct {
    Individual *path;
    int size;
    int num_points;
    int *route_block;
} Population;

Population *population_create(int pop_size, int num_points);
void population_free(Population *pop);

double route_distance(const int *route, const Point *points, int num_points);
void individual_evaluate(Individual *ind, const Point *points, int num_points);
void population_evaluate_all(Population *pop, const Point *points);

/* Returns 1 on success, 0 when the population or its routes are invalid. */
int evaluate_population(Population *pop, const Point *points, int num_points);

const Individual *population_best(const Population *pop);
void individual_copy(Individual *dst, const Individual *src, int num_points);

#endif /* POPULATION_H */