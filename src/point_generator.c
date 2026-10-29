#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "point.h"
#include "point_generator.h"

/* M_PI nao existe em C estrito (-std=c17 sem extensoes GNU). */
#define TSP_PI 3.14159265358979323846

/* Cenario 1: pontos uniformes no quadrado [min, max]^2. Mapa sem estrutura,
   onde o otimo nao e conhecido de antemao. */
Point *generate_uniform_points(int count, double min, double max) {
  if (count <= 0) {
    return NULL;
  }

  Point *points = malloc((size_t)count * sizeof(Point));
  if (points == NULL) {
    return NULL;
  }

  const double span = max - min;
  for (int i = 0; i < count; i++) {
    points[i].id = i;
    points[i].x = min + span * ((double)rand() / (double)RAND_MAX);
    points[i].y = min + span * ((double)rand() / (double)RAND_MAX);
  }

  return points;
}

/* Cenario 2: pontos igualmente espacados na circunferencia. Serve de gabarito:
   a rota otima e percorrer o circulo em ordem, entao da para medir o erro. */
Point *generate_circle_points(int count, double radius) {
  if (count <= 0) {
    return NULL;
  }

  Point *points = malloc((size_t)count * sizeof(Point));
  if (points == NULL) {
    return NULL;
  }

  for (int i = 0; i < count; i++) {
    const double angle = 2.0 * TSP_PI * (double)i / (double)count;
    points[i].id = i;
    points[i].x = radius * cos(angle);
    points[i].y = radius * sin(angle);
  }

  return points;
}

/* Lista os pontos separados por tab, facil de colar numa planilha. */
void print_points(const Point *points, int count) {
  for (int i = 0; i < count; i++) {
    printf("%d\t%.4f\t%.4f\n", points[i].id, points[i].x, points[i].y);
  }
}
