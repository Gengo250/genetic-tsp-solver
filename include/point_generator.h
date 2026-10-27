#ifndef POINT_GENERATOR_H
#define POINT_GENERATOR_H

#include "point.h"

/* Gera pontos distribuidos uniformemente no plano */
Point *generate_uniform_points(int num_points, double min_val, double max_val);

/* Gera pontos na borda de uma circunferencia (benchmark circular) */
Point *generate_circle_points(int num_points, double radius);
void print_points(const Point *points, int count);

#endif
