#include <stdio.h>
#include <stdlib.h>

#include "point_generator.h"

int main(void)
{
    int count;
    int scenario;

    printf("Quantidade de pontos: ");
    scanf("%d", &count);

    printf("\n");
    printf("1 - Uniforme\n");
    printf("2 - Circulo\n");
    printf("\n");

    printf("Cenario: ");
    scanf("%d", &scenario);

    Point *points;

    srand(42);

    if (scenario == 1) {
        points = generate_uniform_points(count, 0.0, 100.0);
    }
    else {
        points = generate_circle_points(count, 50.0);
    }

    printf("\nPontos gerados:\n\n");

    print_points(points, count);

    free(points);

    return 0;
}