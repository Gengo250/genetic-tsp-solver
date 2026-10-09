#ifndef POPULATION_H
#define POPULATION_H

#include "individual.h"
/* path = caminho feito por um indiviuo de uma população para chegar em um ponto
   size =  tamanho total do caminho que um indivuo representa para chegar em um ponto*/
typedef struct {
    Individual *path;
    int size;

} Population;

#endif 
