#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "point.h"
#include "point_generator.h"

/* M_PI nao existe em C estrito (-std=c17 sem extensoes GNU). */
#define TSP_PI 3.14159265358979323846

Point *generate_uniform_points(int count, double min, double max) {


  Point *points = malloc((size_t)count * sizeof(Point));


  const double span = max - min;
  for (int i = 0; i < count; i++) {
    points[i].id = i;
    points[i].x = min + span * ((double)rand() / (double)RAND_MAX);
    points[i].y = min + span * ((double)rand() / (double)RAND_MAX);
  }

  return points;
}

Point *generate_circle_points(int count, double radius) {

  Point *points = malloc((size_t)count * sizeof(Point));

  for (int i = 0; i < count; i++) {
    const double angle = 2.0 * TSP_PI * (double)i / (double)count;
    points[i].id = i;
    points[i].x = radius * cos(angle);
    points[i].y = radius * sin(angle);
  }

  return points;
}

void print_points(const Point *points, int count) {
  for (int i = 0; i < count; i++) {
    printf("%d\t%.4f\t%.4f\n", points[i].id, points[i].x, points[i].y);
  }
}
