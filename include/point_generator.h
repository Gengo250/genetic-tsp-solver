#ifndef POINT_GENERATOR_H
#define POINT_GENERATOR_H

#include "point.h"

Point *generate_uniform_points(int count, double min, double max);
Point *generate_circle_points(int count, double radius);
void print_points(const Point *points, int count);

#endif
