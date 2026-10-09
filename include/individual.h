#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

 /*route =  Array de N indices (ex: [3, 0, 4, 1, 2]), Cada inteiro e o id de um Point no mapa. */
/*distance = Distancia euclidiana total do ciclo completo, Calculada como soma de dist(route[i], route[i+1]) mais dist(route[N-1], route[0]). */
/*fitness = Aptidão (1.0 / distancia), usaremos aptidão como principal critério de escolha para escolha de individuo em uma população*/
typedef struct {
    int    *route;           
    double  distance; 
    double  fitness;  
} Individual;


#endif /* INDIVIDUAL_H */

