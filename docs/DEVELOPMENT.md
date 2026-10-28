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
│   ├── population.h       # Struct Population + criacao, memoria e avaliacao
│   ├── genetic.h          # Assinaturas do motor genetico
│   └── gui.h              # Assinatura de run_gui()
├── src/
│   ├── point_generator.c  # Implementacao dos geradores de pontos
│   ├── population.c       # Populacao (bloco unico), Fisher-Yates e avaliacao
│   ├── genetic.c          # Motor do algoritmo genetico (operadores prontos)
│   ├── gui.c              # Interface grafica (Raylib): AG rodando ao vivo
│   └── main.c             # Entrada do programa (GUI sem args, console com args)
├── tests/
│   └── test_population.c  # Testes da Pessoa 2 (make test, ASan + UBSan)
├── resources/
│   ├── DejaVuSans.ttf     # Fonte da interface (carregada em runtime)
│   └── DejaVuSans-LICENSE.txt
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
Na implementação a comparação é feita direto pelo **menor `distance`** — como
`fitness = 1 / distance`, o resultado é idêntico a comparar o maior fitness, e
o torneio passa a funcionar mesmo que o campo `fitness` ainda não tenha sido
preenchido.

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
- `include/population.h` — Struct `Population {path, size, num_points}`
- `include/point_generator.h` — Assinaturas dos geradores
- `src/point_generator.c` — Implementação dos geradores
- `src/gui.c` + `include/gui.h` — Interface gráfica Raylib
- `src/main.c` — Entry point chamando `run_gui()`



### Etapa 1 — Motor Genético (console) 🔄 EM ANDAMENTO
**Objetivo:** Implementar o AG completo sem GUI, com saída no terminal.  
**Arquivos:**
- `include/genetic.h` — Assinaturas do AG ✅ criado
- `src/genetic.c` — Operadores genéticos (seleção, crossover, mutação) ✅ criados
- `include/population.h` + `src/population.c` — População, memória e avaliação de fitness ✅ criados
- Loop principal (próxima geração + critério de parada) 🔲 pendente

**Funções planejadas:**

| Função | Responsabilidade | Status |
|--------|-----------------|--------|
| `tournament_select()` | Seleciona um pai via torneio (k=3) | ✅ |
| `order_crossover()` | Gera um filho via Order Crossover (OX1) entre dois pais | ✅ |
| `inversion_mutation()` | Aplica mutação por inversão com probabilidade p | ✅ |
| `route_is_valid()` | Checagem defensiva: a rota é permutação de 0..N-1? | ✅ |
| `population_create()` | Aloca Population e P Individuals com rotas embaralhadas | ✅ |
| `population_free()` | Libera toda a memória da população | ✅ |
| `route_distance()` | Distância do ciclo, incluindo a aresta de volta | ✅ |
| `individual_evaluate()` | Calcula `distance` e `fitness` de um indivíduo | ✅ |
| `population_evaluate_all()` | Avalia todos os indivíduos da população | ✅ |
| `population_best()` | Retorna ponteiro para o indivíduo de menor `distance` (= maior fitness) | ✅ |
| `individual_copy()` | Cópia profunda de um indivíduo (para o elitismo) | ✅ |
| `population_next_gen()` | Gera a próxima geração completa (elitismo + crossover + mutação) | 🔲 |
| `ga_run()` | Loop principal do AG com critério de parada duplo | 🔲 |

#### Operadores genéticos entregues (Pessoa 1)

Assinaturas em `include/genetic.h`:

```c
const Individual *tournament_select(const Population *population, int tournament_size);
void order_crossover(const int *parent_a, const int *parent_b, int *child, int num_points);
void inversion_mutation(int *route, int num_points, double rate);
int  route_is_valid(const int *route, int num_points);
```

**`tournament_select()`** — sorteia `tournament_size` indivíduos com reposição e
devolve o de menor `distance` (caminho mais curto vence). Exige população já
avaliada. `tournament_size` é ajustado para `[1, population->size]`, então valor
0, negativo ou maior que a população não estoura índice. Retorna `NULL` para
população inválida/vazia; o ponteiro aponta para dentro da população e não deve
ser liberado.

**`order_crossover()`** — OX1 em três passos:
1. Sorteia `start <= end` em `[0, num_points-1]` e copia `parent_a[start..end]`
   para a mesma posição do filho.
2. Varre `parent_b` de forma circular a partir de `end + 1`, escrevendo também
   circularmente a partir de `end + 1`, pulando genes já usados.
3. Rede de segurança: se o pai B vier corrompido (gene fora de
   `[0, num_points)` ou repetido), as posições que sobraram são fechadas com os
   genes ainda não usados.

A tabela auxiliar `used[gene]` é indexada pelo próprio gene — como o gene é o
índice do ponto, a checagem de repetição é O(1) em vez de varrer o filho a cada
inserção, deixando o crossover em O(N). O filho é **sempre** uma permutação
válida: sem cidade repetida, sem cidade faltando e sem índice fora de
`num_points`.

**`inversion_mutation()`** — com probabilidade `rate` sorteia dois índices em
`[0, num_points-1]`, ordena o par e inverte o sub-vetor entre eles. Por só
reordenar genes existentes, a rota continua sendo permutação válida por
construção. `num_points < 2`, `rate <= 0.0` e ponteiro `NULL` são no-ops.

**Controle de limites:** todo sorteio de índice passa pelo helper interno
`random_range(min, max)` (intervalo fechado, com `max <= min` devolvendo `min`),
que é o único ponto do módulo que gera índices. Isso concentra a garantia de que
nenhum índice escapa de `[0, num_points-1]`.

**Verificação executada** (harness fora do repo, `gcc -Wall -Wextra`, limpo sob
ASan/UBSan — 29 verificações, 0 falhas):

| Verificação | Resultado |
|-------------|-----------|
| OX1: n=2..16, 1500 pares cada → filho é permutação válida | 0 inválidos |
| OX1: filho sempre explicável por algum corte `(s,e)` da semântica OX1 | 0 divergências |
| OX1: pais idênticos devolvem cópia exata do pai | 0 divergências |
| OX1: pai B com gene fora dos limites, negativo e duplicado | 0 filhos inválidos |
| Mutação: `rate = 0.0` nunca altera / `rate = 1.0` altera 19008/20000 | ok |
| Mutação: resultado é sempre o original com um único bloco contíguo invertido | 0 violações |
| Mutação: taxa observada com `MUTATION_RATE = 0.05` | 0.0472 |
| Torneio k=3: P(melhor vencer) teórico 0.271 / P(pior) 0.001 | 0.2712 / 0.0012 |
| Torneio: frequência de vitória decresce com a distância (monotonia) | ok |
| Torneio: k=1 vira sorteio uniforme; k > população é ajustado | ok |

#### Ambiente, memória e avaliação entregues (Pessoa 2)

Assinaturas em `include/population.h`:

```c
typedef struct {
    Individual *path;      /* vetor com os `size` individuos */
    int size;              /* quantidade de individuos */
    int num_points;        /* pontos do mapa = tamanho de cada rota (NOVO) */
    int *route_block;      /* inicio do bloco de rotas; unico ponteiro que deve ser liberado (NOVO) */
} Population;

Population *population_create(int pop_size, int num_points);
void        population_free(Population *pop);
double      route_distance(const int *route, const Point *points, int num_points);
void        individual_evaluate(Individual *ind, const Point *points, int num_points);
void        population_evaluate_all(Population *pop, const Point *points);
const Individual *population_best(const Population *pop);
void        individual_copy(Individual *dst, const Individual *src, int num_points);
```

**`population_create()`** — valida `pop_size >= 1` e `num_points >= 1`, aloca a
população e preenche cada rota com `0..N-1` embaralhada por Fisher-Yates.
`distance` e `fitness` começam em 0. **Não chama `srand`**: a semente é
decidida por quem chama (main/GUI), o que permite execuções reproduzíveis.
Retorna `NULL` se a entrada for inválida ou faltar memória, sem vazar nada.

**`population_free()`** — libera `route_block`, depois `path`, depois a struct.
`population_free(NULL)` é no-op. O bloco é liberado pelo ponteiro guardado em
`route_block`, e **não** por `path[0].route`. Depois de um `qsort` em `path`,
o `path[0]` aponta para o meio do bloco, e `free` nesse endereço é
comportamento indefinido (o ASan acusa *bad-free*).

**`route_distance()`** — soma `hypot(dx, dy)` entre pontos consecutivos **e a
aresta de volta `route[N-1] → route[0]`**, porque o caminho do caixeiro é um
ciclo. `N < 2` retorna 0. Os pontos são indexados por `points[route[i]]`,
assumindo `points[k].id == k` (garantido pelos geradores).

**`individual_evaluate()`** — `distance = route_distance(...)` e
`fitness = 1 / distance`. Se `distance < 1e-12` (N < 2 ou todos os pontos no
mesmo lugar), usa `fitness = 0`: nesse caso toda rota tem o mesmo tamanho, e 0
evita divisão por zero e valores infinitos em médias e gráficos.

**`population_evaluate_all()`** — chama `individual_evaluate` para cada
indivíduo, usando `pop->num_points`.

**`population_best()`** — busca linear O(P) pelo **menor `distance`**, o mesmo
critério do `tournament_select`. Retorna `NULL` para população inválida.

**`individual_copy()`** — cópia **profunda**: `memcpy` do conteúdo da rota +
`distance` + `fitness`. Nunca copia o ponteiro `route`. Se copiasse, dois
indivíduos dividiriam a mesma memória (alterar um alteraria o outro) e o bloco
seria liberado duas vezes (double free). Usada no elitismo (Pessoa 3).

**Decisão: bloco único para as rotas.** A população faz só 3 alocações:

```
pop ──> Population { path, size, num_points, route_block }
           │                                  │
           v                                  │
         path[0]  path[1]  path[2]  ...       │   (1 malloc: P Individuals)
           │        │        │                │
           v        v        v                v
route_block: [ N ints | N ints | N ints | ... ]   (1 malloc: P*N ints)
```

Na criação, `path[i].route = route_block + i * num_points`. Motivos:
- 3 mallocs e 3 frees em vez de P + 2;
- se um malloc falhar, há no máximo 2 coisas para desfazer, então não existe
  vazamento parcial de "metade das rotas";
- os dados ficam contíguos na memória, aproveitando melhor o cache quando a
  avaliação percorre a população inteira.

> **A população pode ser reordenada com segurança.** Como o bloco é liberado
> por `route_block`, a ordem do vetor `path` e os ponteiros `route` de cada
> indivíduo podem mudar à vontade (ex.: `qsort` por `distance` para seleção
> por Ranking, ou trocar `route` entre indivíduos). A única regra é que toda
> rota continue apontando para dentro do bloco: nunca atribua a `route` memória
> alocada por fora, e nunca chame `free` em um `route` individual.

**Decisão: Fisher-Yates.** Para `i` de `N-1` até 1: `j = rand() % (i+1)` e troca
`route[i]` com `route[j]`. A posição N-1 tem N escolhas, a N-2 tem N-1, …, a
posição 1 tem 2 escolhas. São N! sequências de sorteio, cada uma gerando uma
permutação diferente, ou seja, cada permutação sai com probabilidade exata 1/N!.
"Sortear dois índices e trocar N vezes" tem N^(2N) sequências, que não é
múltiplo de N! (N=3: 729 sequências para 6 permutações), então algumas
permutações saem mais que outras.

**Verificação executada** (`make test`: `tests/test_population.c`,
`-std=c17 -Wall -Wextra` sem warnings, ASan + UBSan limpos, 38 verificações,
0 falhas):

| Verificação | Resultado |
|-------------|-----------|
| Rotas criadas passam em `route_is_valid` (N = 1, 2, 8, 50, 500; 100 rotas cada) | 0 inválidas |
| Fisher-Yates, N=4, 240000 rotas: 24 permutações, esperado 10000 cada | 24 distintas, maior desvio 2.43% (9832 a 10243) |
| Círculo R=100, rota `0..N-1` vs `2·N·R·sin(π/N)` (N = 8, 50, 500) | erro máx. 4.5e-13 (tolerância 1e-9) |
| Quadrado unitário, rota 0,1,2,3 | 4.000000000000 |
| Quadrado unitário, rota cruzada 0,2,1,3 | 4.828427124746 = 2 + 2√2 |
| `individual_evaluate` no quadrado / distância zero | fitness 0.25 / fitness 0 |
| `population_best` (200 indivíduos, N=30) | nenhum menor; coincide com o maior fitness |
| `individual_copy`: ponteiros diferentes, alterar a cópia não altera o original | ok |
| 1000 ciclos create/free + `population_free(NULL)` + entradas inválidas → `NULL` | ASan limpo |
| Vazamento proposital (sem `population_free`) é acusado pelo LeakSanitizer | "236 bytes leaked in 3 allocation(s)" |
| `qsort` de `path` por `distance` (100 indivíduos, N=25): rotas saem da ordem do bloco | ordenado; `population_best == &path[0]`; `population_free` limpo no ASan |
| Mesmo cenário liberando via `path[0].route` (comportamento antigo) | ASan: *bad-free* ("attempting free on address which was not malloc()-ed") |
| Integração: `tournament_select` sobre população avaliada (10000 torneios) | média dos vencedores 2387.27 < média da pop. 2612.79 |
| Mini-AG de integração (torneio + OX1 + inversão + elitismo), círculo N=20, 500 gerações | 1840.07 → 625.74 (ótimo 625.74, gap 0.00%), o melhor nunca piora |

> O ASan não linka no gcc MinGW do Windows. O `make test` foi executado no
> WSL (Ubuntu 24.04, gcc 13.3).

#### Função de aptidão (texto para o relatório)

A aptidão de um indivíduo é definida como `fitness = 1 / distância`, em que a
distância é o comprimento euclidiano total da rota. Como o objetivo do problema
é **minimizar** a distância, a inversa transforma esse objetivo em uma
**maximização**: quanto menor o caminho, maior a aptidão. Na prática, os
operadores comparam os indivíduos diretamente pelo campo `distance` (vence o
menor). Isso produz exatamente a mesma ordem que comparar o maior `fitness`,
já que `1/x` é estritamente decrescente para `x > 0`, e ainda evita uma
divisão. O cálculo da distância inclui a **aresta de retorno** do último ponto
ao primeiro, pois o caminho do caixeiro é um ciclo fechado: sem ela estaríamos
otimizando um caminho aberto, que é outro problema. Por fim, o cenário circular
tem ótimo conhecido: com N pontos igualmente espaçados num círculo de raio R, o
melhor ciclo é o perímetro do polígono regular, `2·N·R·sin(π/N)`. Isso permite
medir o **gap** do AG em relação ao ótimo, `gap = (melhor_AG − ótimo) / ótimo`,
e avaliar objetivamente a qualidade da solução encontrada.

---

### Etapa 2 — Integração com GUI ✅
**Objetivo:** Conectar o motor genético à interface Raylib para visualizar
a evolução do caminho época a época.

`src/gui.c` agora inclui `genetic.h` e `population.h` e roda o AG dentro do
laço de desenho. O estado da execução vive num `Solver` local:

```c
typedef struct {
    Population *population;
    int generation;
    int max_generations;
    int stagnant;
    double best_distance;
    /* ... */
} Solver;
```

A cada frame, se a evolução estiver ativa, o laço chama `solver_step()` de 1 a
50 vezes (controle "Velocidade: N ger/frame") e depois desenha a rota do
`population_best()` da geração atual. Como o elitismo garante que o melhor
nunca piora, o indivíduo desenhado é também o melhor de toda a execução — não
é preciso guardar uma cópia à parte.

**Controles:** `Gerar pontos` (novo mapa, reinicia a evolução), `Iniciar` /
`Pausar` / `Continuar` (também na barra de espaço), `Resetar` e os botões de
velocidade. O painel esquerdo acompanha época, melhor distância, época em que
o melhor apareceu, tempo decorrido, taxa de mutação atual e status. No cenário
circular ele mostra também o ótimo `2·N·R·sin(π/N)` e o **gap** em tempo real.

O critério de parada é o mesmo do modo console — por isso `STAGNATION_LIMIT`,
`GENERATIONS_PER_POINT` e `MIN_MAX_GENERATIONS` saíram dos literais de
`main.c` para `include/genetic.h`, com `max_generations_for()` em
`src/genetic.c`. GUI e console descrevem o mesmo algoritmo.

**Fonte:** a interface usa DejaVu Sans (`resources/DejaVuSans.ttf`, licença em
`resources/DejaVuSans-LICENSE.txt`) carregada com `LoadFontEx` em 32 px e
filtro bilinear, no lugar da fonte bitmap embutida do Raylib. O arquivo é
procurado em `resources/`, `../resources/`, `../../resources/` e ao lado do
executável; se nenhum existir, a fonte padrão do Raylib assume e a interface
continua legível. O CMake copia `resources/` para junto do binário no
`POST_BUILD`, então rodar de dentro de `build/` também funciona.

---

### Etapa 3 — Relatório de desempenho 🔲 PENDENTE
**Objetivo:** Gerar saída formatada comparando cenário uniforme vs circular,
com tabela de distância por época para apresentação.

---

## Parâmetros do Algoritmo

Valores conforme `include/genetic.h` e `src/main.c`:

| Parâmetro | Valor | Justificativa |
|-----------|-------|---------------|
| Tamanho da população | 100 (`POPULATION_SIZE`) | Balanceia diversidade e velocidade |
| Taxa de crossover | 1.00 → 0.75 ao longo da execução | Começa misturando sempre; no fim preserva mais os bons esquemas já encontrados |
| Taxa de mutação | 10% → 2% ao longo da execução | Explora muito no início e refina no fim, sem destruir o crossover |
| Tamanho do torneio | 3 (`TOURNAMENT_SIZE`) | Pressão seletiva moderada |
| Max épocas | `max(N·20, 1000)` (`max_generations_for`) | Teto proporcional ao tamanho do espaço de busca, com piso para mapas pequenos |
| Estagnação | 150 épocas (`STAGNATION_LIMIT`) | Critério de parada antecipada |
| Elitismo | 1 indivíduo (`ELITISM_COUNT`) | Garante monotonia (melhor nunca piora) |
| Min pontos | 8 | Requisito do enunciado |

As taxas adaptativas são interpoladas linearmente pela fração
`geração / max_épocas` em `rate_for_generation()` (`src/genetic.c`).

---

## Como Compilar e Executar

```bash
# Na pasta raiz do projeto
mkdir -p build && cd build
cmake ..
make
```

**Modo grafico** (sem argumentos) — escolha os pontos e o cenario, clique em
`Gerar pontos` e depois em `Iniciar` para ver a rota evoluindo:

```bash
./genetic_tsp_solver
```

**Modo console** (com argumentos) — imprime o desempenho a cada `PRINT_EVERY`
epocas, util para gerar a tabela do relatorio:

```bash
# <cenario: 1=uniforme, 2=circular> <N> <PRINT_EVERY>
./genetic_tsp_solver 1 20 50     # uniforme, 20 pontos, log a cada 50 epocas
./genetic_tsp_solver 2 50 100    # circular, 50 pontos, log a cada 100 epocas
```

```bash
# Testes da populacao/avaliacao (sem Raylib, com ASan + UBSan; Linux/WSL)
make test
```
