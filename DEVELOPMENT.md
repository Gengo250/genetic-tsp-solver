# Genetic TSP Solver — Documentação de Desenvolvimento

> Algoritmo Genético para o Problema do Caixeiro Viajante em C  
> Build: CMake + Raylib | Linguagem: C17

---

## Visão Geral da Arquitetura

```
genetic-tsp-solver/
├── include/
│   ├── point.h            # Struct Point (coordenada 2D)
│   ├── point_generator.h  # Assinaturas: gera pontos uniforme/circulo
│   ├── individual.h       # Struct Individual (solucao candidata)
│   ├── population.h       # Struct Population (geracao atual)
│   ├── genetic.h          # [PROXIMO] Assinaturas do motor genetico
│   └── gui.h              # Assinatura de run_gui()
├── src/
│   ├── point_generator.c  # Implementacao dos geradores de pontos
│   ├── genetic.c          # [PROXIMO] Motor do algoritmo genetico
│   ├── gui.c              # Interface grafica (Raylib)
│   └── main.c             # Entrada do programa
└── DEVELOPMENT.md         # Este arquivo
```

---

## Fluxo do Algoritmo Genético

```
Gerar Mapa (N pontos)
        |
        v
[1] Populacao Inicial
    - P individuos com rotas embaralhadas (Fisher-Yates)
        |
        v
[2] Avaliacao (Fitness)
    - distancia = soma euclidiana do ciclo
    - fitness   = 1.0 / distancia
        |
        v
[3] Nova Geracao  <─────────────────────────────────┐
    ├─ Elitismo: melhor individuo copiado direto     |
    ├─ Selecao por Torneio (k=3) → Pai1, Pai2       |
    ├─ Crossover OX1 → Filho                        |
    └─ Mutacao por Inversao (taxa 5%) → Filho final  |
        |                                            |
        v                                            |
[4] Criterio de Parada?                             |
    ├─ NAO → substitui populacao ────────────────────┘
    └─ SIM → exibe melhor solucao
```

**Criterio de parada (duplo):**
- Maximo de épocas atingido (ex: 2000), **OU**
- Melhor distancia nao melhora por 100 epocas consecutivas (estagnacao)

---

## Decisões de Design

### Representação do Gene
Cada indivíduo é um **array de inteiros** de tamanho N, onde cada inteiro é o
índice de um ponto no mapa. Ex: `[3, 0, 4, 1, 2]` significa "vá ao ponto 3,
depois 0, depois 4, depois 1, depois 2, e volte ao 3".

**Por que array de inteiros?**  
- Acesso O(1) a qualquer posição (necessário no OX1)  
- Verificação de presença via array auxiliar de booleanos em O(N)  
- Simples de embaralhar e inverter subsegmentos

### Cenário Circular (Benchmark)
Os pontos são distribuídos **uniformemente pelo ângulo** (não aleatório),
formando um polígono regular. Isso garante que a solução ótima seja
perfeitamente conhecida (percorrer os pontos em ordem sequencial = menor
perímetro possível), permitindo medir o quão próximo o AG chega do ótimo.

### Geração da População Inicial
Rotas **completamente aleatórias** via Fisher-Yates shuffle.  
**Não** se usa heurísticas (tipo vizinho mais próximo) na geração inicial pois
isso elimina a diversidade genética — o AG precisa de caos inicial para
explorar o espaço de soluções.

### Seleção por Torneio (k=3)
Sorteia 3 indivíduos aleatórios; o de maior fitness vence.  
**Vantagem sobre roleta:** não é dominado por um único indivíduo com fitness
muito superior, mantendo pressão seletiva balanceada.

### Crossover OX1 (Order Crossover)
Preserva um segmento do Pai1 e preenche o restante com a ordem do Pai2,
garantindo que todo filho seja uma permutação válida (sem cidades repetidas
ou faltando). Ideal para TSP.

### Mutação por Inversão
Inverte um subsegmento aleatório da rota.  
**Por que inversão?** Desfaz cruzamentos de arestas no mapa (fenômeno
geométrico que sempre indica sub-otimalidade), sendo altamente eficiente
para TSP.

### Taxa de Mutação
Taxa de **5% (0.05)** por indivíduo gerado.  
- Baixa o suficiente para não destruir boas soluções encontradas pelo crossover  
- Alta o suficiente para manter diversidade e escapar de ótimos locais

### Tamanho da População
**100 indivíduos** por geração.  
Balanceia velocidade de convergência (populações menores convergem rápido
mas ficam presas em ótimos locais) com qualidade de exploração.

---

## Log de Desenvolvimento por Etapas

### Etapa 0 — Infraestrutura inicial ✅
**Data:** Início do projeto  
**Arquivos criados:**
- `include/point.h` — Struct `Point {id, x, y}`
- `include/individual.h` — Struct `Individual {route, distance, fitness}`
- `include/population.h` — Struct `Population {individuals, size}`
- `include/point_generator.h` — Assinaturas dos geradores
- `src/point_generator.c` — Implementação dos geradores
- `src/gui.c` + `include/gui.h` — Interface gráfica Raylib
- `src/main.c` — Entry point chamando `run_gui()`



### Etapa 1 — Motor Genético (console) 🔄 EM ANDAMENTO
**Objetivo:** Implementar o AG completo sem GUI, com saída no terminal.  
**Arquivos a criar:**
- `include/genetic.h` — Assinaturas de todas as funções do AG
- `src/genetic.c` — Implementação completa

**Funções planejadas:**

| Função | Responsabilidade |
|--------|-----------------|
| `population_create()` | Aloca Population e N Individuals com rotas embaralhadas |
| `population_free()` | Libera toda a memória da população |
| `individual_evaluate()` | Calcula `distance` e `fitness` de um indivíduo |
| `population_evaluate_all()` | Avalia todos os indivíduos da população |
| `population_best()` | Retorna ponteiro para o indivíduo com maior fitness |
| `tournament_select()` | Seleciona um pai via torneio (k=3) |
| `crossover_ox1()` | Gera um filho via Order Crossover entre dois pais |
| `mutate_inversion()` | Aplica mutação por inversão com probabilidade p |
| `population_next_gen()` | Gera a próxima geração completa (elitismo + crossover + mutação) |
| `ga_run()` | Loop principal do AG com critério de parada duplo |

---

### Etapa 2 — Integração com GUI 🔲 PENDENTE
**Objetivo:** Conectar o motor genético à interface Raylib para visualizar
a evolução do caminho época a época.

---

### Etapa 3 — Relatório de desempenho 🔲 PENDENTE
**Objetivo:** Gerar saída formatada comparando cenário uniforme vs circular,
com tabela de distância por época para apresentação.

---

## Parâmetros do Algoritmo

| Parâmetro | Valor | Justificativa |
|-----------|-------|---------------|
| Tamanho da população | 100 | Balanceia diversidade e velocidade |
| Taxa de mutação | 5% | Explora sem destruir crossover |
| Tamanho do torneio | 3 | Pressão seletiva moderada |
| Max épocas | 2000 | Limite superior de tempo |
| Estagnação | 100 épocas | Critério de parada antecipada |
| Elitismo | 1 indivíduo | Garante monotonia (melhor nunca piora) |
| Min pontos | 8 | Requisito do enunciado |

---

## Como Compilar e Executar

```bash
# Na pasta raiz do projeto
mkdir -p build && cd build
cmake ..
make
./genetic_tsp_solver
```
