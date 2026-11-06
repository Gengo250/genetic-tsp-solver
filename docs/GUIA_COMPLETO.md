# Guia completo: algoritmo genético para o caixeiro viajante

Este guia descreve a implementação presente no repositório. Seu objetivo é servir tanto para estudar a teoria quanto para acompanhar o código e apresentar o trabalho. Os valores aqui são os definidos em `include/genetic.h`; comentários e trechos antigos de `docs/DEVELOPMENT.md` podem apresentar outros parâmetros. As justificativas abaixo explicam consequências técnicas das escolhas; quando não há experimento registrado suficiente, elas não devem ser confundidas com prova de que um parâmetro é o melhor possível.

## 1. O problema

Dado um conjunto de N pontos no plano, busca-se um **ciclo** que visite cada ponto exatamente uma vez e retorne ao ponto inicial, com a menor distância total. Para os pontos `a=(xa, ya)` e `b=(xb, yb)`, a distância euclidiana é:

```text
d(a,b) = sqrt((xb - xa)² + (yb - ya)²)
D(rota) = soma de d(rota[i], rota[(i+1) mod N]), para i = 0 até N-1
```

O módulo fecha o ciclo: a última cidade também se conecta à primeira. A rota `[0, 2, 1, 3]` representa `0 → 2 → 1 → 3 → 0`.

O número de permutações cresce como `N!`. Para um ciclo simétrico com pontos distintos, rotações e inversões representam o mesmo percurso, reduzindo o número de ciclos distintos para `(N-1)!/2` quando N > 2. Mesmo assim, a busca exaustiva cresce rapidamente. O algoritmo genético (AG) é uma **meta-heurística**: procura boas soluções por amostragem e evolução, mas não garante o ótimo global.

## 2. Conceitos do algoritmo genético

- **Gene:** índice inteiro de um ponto no vetor de pontos.
- **Cromossomo/indivíduo:** permutação dos índices, representando uma rota completa.
- **População:** conjunto de rotas candidatas.
- **Aptidão (fitness):** medida de qualidade; aqui, para distância positiva, `1 / distância`. Quanto menor a distância, maior o fitness.
- **Seleção:** escolha de candidatos para reprodução, favorecendo rotas menores.
- **Crossover:** combinação de partes de dois pais para formar um filho.
- **Mutação:** alteração aleatória que ajuda a explorar outras soluções.
- **Geração ou época:** uma população produzida a partir da anterior. A população inicial é a geração 0.
- **Elitismo:** cópia da melhor rota para a próxima geração, impedindo que a melhor distância piore enquanto as entradas forem válidas.

O programa compara diretamente distâncias na seleção. Para distâncias estritamente positivas isso equivale a maximizar `1/D`; para distância inferior a `1e-12`, o fitness é definido como zero, portanto essa equivalência tem uma exceção deliberada. O torneio continua usando a distância também nesse caso.

## 3. Fluxo completo

```text
Entrada: cenário e N
  → gerar pontos
  → criar P rotas aleatórias válidas
  → validar e avaliar cada rota
  → repetir:
      copiar o melhor indivíduo (elitismo)
      selecionar dois pais por torneio
      criar filho por OX1 e aplicar possível inversão
      repetir até completar a população
      validar e avaliar a nova população
      descartar a população antiga
      registrar melhoria e verificar parada
  → mostrar a melhor rota e suas métricas
```

Cada indivíduo armazena os índices de todos os pontos. Isso permite acesso direto, embaralhamento e inversão simples. Em contrapartida, crossover genérico de vetores pode repetir cidades; por isso o projeto usa um operador próprio para permutações. Índices representam posições no vetor `points`, embora os geradores também atribuam `id = índice`.

## 4. Cenários e referência matemática

### Distribuição uniforme

`generate_uniform_points` sorteia as coordenadas `x` e `y` separadamente no intervalo de 0 a 100. No modelo ideal isso distribui pontos no quadrado, não numa circunferência. `rand()` fornece valores pseudoaleatórios discretos, inclusive podendo produzir os extremos. Não há ótimo conhecido calculado para uma instância arbitrária; a menor distância encontrada é apenas a melhor observada.

### Círculo regular

`generate_circle_points` posiciona o ponto `i` no ângulo `2πi/N`, com raio `R=100`:

```text
xᵢ = R cos(2πi/N)      yᵢ = R sin(2πi/N)
```

Os pontos formam um polígono regular. Visitar os vértices em ordem angular, em qualquer sentido, dá o perímetro ótimo. Cada lado mede `2R sin(π/N)`; portanto:

```text
ótimo = 2 N R sin(π/N)
gap (%) = 100 × (distância encontrada − ótimo) / ótimo
```

Exemplo: para N=8 e R=100, o ótimo é aproximadamente 612,293. Um gap próximo de zero indica resultado próximo da referência; pequenos desvios negativos podem ocorrer por arredondamento numérico. A GUI calcula e mostra ótimo e gap; o console não os calcula automaticamente. Os pontos circulares são determinísticos, mas a evolução depende da semente aleatória.

## 5. Parâmetros reais e consequências

| Escolha | Valor atual | Efeito e justificativa |
|---|---:|---|
| População | `max(100, 2N)` | Piso de diversidade em problemas pequenos e crescimento com N; mais indivíduos aumentam exploração e custo. |
| Torneio | 7 participantes | Pressão seletiva: rotas curtas tendem a vencer; pressão maior também pode reduzir diversidade. Sorteios são com reposição, então um indivíduo pode aparecer mais de uma vez. |
| Elitismo | 1 | Preserva a melhor solução anterior. |
| Crossover OX1 | 100% por filho não elite | Combina pais preservando permutações válidas quando os pais são válidos. |
| Mutação por inversão | 30% por filho não elite | Introduz variação; um segmento de tamanho 1 pode deixar a rota igual mesmo quando a mutação é acionada. |
| Estagnação | `max(150, 2N)` gerações | Interrompe após esse número de gerações sem melhoria estrita da melhor distância. |
| Limite máximo | `max(1000, 20N)` gerações | Limita o esforço mesmo quando ainda há melhorias. |

Por exemplo, N=100 resulta em população 200, limite de estagnação 200 e máximo 2000 gerações. As funções de taxa suportam interpolação linear entre valores inicial e final, mas atualmente ambos são iguais: **as taxas são constantes**. Comentários no código relatam medições que motivaram o crescimento da população e a retirada do decaimento; não há neste guia dados suficientes para afirmar robustez estatística ou superioridade universal dessas escolhas. Para justificá-las experimentalmente, compare várias sementes, tamanhos e configurações, registrando gap, tempo, gerações e dispersão.

Parar por estagnação significa que não se observou melhora no intervalo definido; não prova convergência ao ótimo global. A comparação é estrita, sem tolerância de ponto flutuante.

## 6. Passo a passo dos operadores

### População inicial: Fisher–Yates

Para cada indivíduo, inicia-se com `[0, 1, ..., N-1]`. Do fim para o começo, sorteia-se `j` entre 0 e `i` e trocam-se as posições `i` e `j`. Toda rota continua sendo uma permutação. Sob sorteios perfeitamente uniformes, cada permutação tem probabilidade `1/N!`; o uso de `rand() % (i+1)` introduz possível viés de módulo na implementação real.

### Avaliação

Para cada par consecutivo de índices, `route_distance` calcula `hypot(dx, dy)` e soma também a volta. `individual_evaluate` armazena distância e fitness. A validação antes da avaliação verifica limites e unicidade, evitando acessar `points` com índices inválidos. A função `route_distance` isoladamente pressupõe índices válidos.

### Seleção por torneio

`tournament_select` sorteia k indivíduos da população, com reposição, e devolve o de menor distância. Sorteia-se independentemente um torneio para cada pai; ambos podem ser o mesmo indivíduo. O tamanho pedido é limitado ao intervalo de 1 até o tamanho da população, mas isso não impede repetições nos sorteios.

### Crossover ordenado OX1

1. Sorteie `start` e `end` com `start ≤ end`; copie o segmento inclusivo do pai A para as mesmas posições do filho.
2. Marque os genes copiados no vetor auxiliar `used`.
3. Percorra o pai B circularmente a partir de `end+1`, inserindo no filho, também circularmente a partir de `end+1`, somente genes ainda não usados.
4. Se o pai B estiver incompleto ou contiver valores inválidos, preencha posições restantes com genes ainda não usados, em ordem crescente.

Exemplo com corte nas posições 1 a 2:

```text
pai A:  [0, 1, 2, 3, 4]
pai B:  [3, 4, 2, 0, 1]
segmento:  [_, 1, 2, _, _]
leitura circular de B a partir de 3: 0, 1, 3, 4, 2
filho:  [4, 1, 2, 0, 3]
```

O segmento pode ocupar toda a rota, produzindo uma cópia de A. A tabela auxiliar fornece verificação de presença em tempo constante e custo total O(N). A recuperação para B corrompido **não valida nem corrige o segmento de A**; não há garantia de filho válido se A também estiver inválido. Se a alocação de `used` falhar, copia-se A. A validação posterior da população detecta rotas inválidas.

### Mutação por inversão

Com probabilidade 0,30, sorteiam-se dois índices, ordenam-se os extremos e inverte-se o segmento inclusivo. Exemplo: `[0, 1, 2, 3, 4]` com extremos 1 e 3 vira `[0, 3, 2, 1, 4]`. Isso preserva os genes e pode alterar as arestas da rota. Em TSP euclidiano, uma inversão adequada pode remover cruzamentos e melhorar uma rota, mas uma inversão aleatória não necessariamente melhora e pode não alterar nada.

### Nova geração

`evolve_population` cria uma população de mesmo tamanho, copia o melhor indivíduo para a posição de elite, gera os demais por torneio, crossover e mutação e valida/avalia a nova população. A população anterior permanece sob responsabilidade do chamador, que a libera após o sucesso. No código atual a função de criação também embaralha rotas que serão sobrescritas: é trabalho extra e consome números aleatórios.

## 7. Estruturas e memória

| Estrutura | Campos | Papel |
|---|---|---|
| `Point` | `id`, `x`, `y` | Identificação e coordenadas. |
| `Individual` | `route`, `distance`, `fitness` | Rota e resultado da avaliação. |
| `Population` | `path`, `size`, `num_points`, `route_block` | Indivíduos e bloco que armazena todas as rotas. |
| `Solver` (privado da GUI) | população, geração, melhor, tempos, status e flags | Estado da execução gráfica. |

`population_create` faz três alocações: estrutura, vetor de indivíduos e bloco contíguo de `P×N` inteiros. Cada rota aponta para uma fatia do bloco. `route_block` guarda o endereço original para liberação mesmo se `path` for reordenado. `individual_copy` copia o conteúdo da rota para memória já alocada, não apenas o ponteiro. `population_free` libera bloco, indivíduos e estrutura; ponteiros para indivíduos tornam-se inválidos após essa liberação.

## 8. Referência das funções

### `src/point_generator.c`

| Função | Entrada, resultado e decisão |
|---|---|
| `generate_uniform_points(count, min, max)` | Aloca e retorna `count` pontos pseudoaleatórios no quadrado; retorna `NULL` para contagem inválida ou falha de alocação. O chamador libera o vetor. Não valida a ordem dos limites. |
| `generate_circle_points(count, radius)` | Aloca pontos igualmente espaçados em ângulo e retorna o vetor; `NULL` para contagem inválida ou falha. O chamador libera. |
| `print_points(points, count)` | Imprime id e coordenadas; pressupõe ponteiro válido quando `count > 0`. |

### `src/population.c`

| Função | Entrada, resultado e decisão |
|---|---|
| `shuffle_route` | Função interna: Fisher–Yates in-place, mantendo a permutação. |
| `population_create(pop_size, num_points)` | Aloca população e rotas aleatórias; inicializa distância e fitness em zero. Retorna `NULL` em parâmetros inválidos ou falha; verifica overflow do tamanho do bloco de rotas. É preciso avaliar antes de selecionar. |
| `population_free(pop)` | Libera toda a população; aceita `NULL`. |
| `route_distance(route, points, num_points)` | Soma as distâncias do ciclo; retorna zero para ponteiros nulos ou N < 2. Pressupõe índices válidos. |
| `individual_evaluate(ind, points, num_points)` | Atualiza distância e fitness; distância < `1e-12` recebe fitness zero para evitar divisão por zero. |
| `population_evaluate_all(pop, points)` | Avalia cada indivíduo sem validar a permutação; pressupõe rotas e pontos válidos. |
| `evaluate_population(pop, points, num_points)` | Confere estrutura, dimensões e todas as permutações antes de avaliar; retorna 1 em sucesso e 0 em erro, inclusive falta de memória. Reutiliza um vetor de marcação. |
| `population_best(pop)` | Retorna ponteiro para o indivíduo de menor distância ou `NULL` para população inválida/vazia; em empate mantém o primeiro encontrado. Ponteiro pertence à população. |
| `individual_copy(dst, src, num_points)` | Copia rota e métricas; destino precisa ter rota alocada e capacidade suficiente. |

### `src/genetic.c`

| Função | Entrada, resultado e decisão |
|---|---|
| `random_range(min, max)` | Interna: inteiro pseudoaleatório no intervalo fechado; retorna `min` se `max ≤ min`; usa módulo, sujeito a viés. |
| `random_unit()` | Interna: real pseudoaleatório em `[0,1)`, usado nas probabilidades. |
| `tournament_select(population, tournament_size)` | Retorna ponteiro emprestado para o vencedor, ou `NULL` para população inválida; usa menor distância. |
| `rate_for_generation` | Interna: interpola taxas e limita extremos; se máximo ≤ 0 devolve taxa final. |
| `crossover_rate_for_generation` / `mutation_rate_for_generation` | Aplicam a interpolação aos parâmetros globais; atualmente retornam 1,0 e 0,30, respectivamente. |
| `floor_plus_share` | Interna: calcula o maior entre piso e parcela proporcional; para N < 1 retorna o piso. |
| `population_size_for`, `stagnation_limit_for`, `max_generations_for` | Calculam os três limites descritos na tabela de parâmetros. |
| `order_crossover(parent_a, parent_b, child, num_points)` | Escreve OX1 em memória fornecida pelo chamador; detalhes e condições na seção 6. |
| `inversion_mutation(route, num_points, rate)` | Altera rota in-place conforme a taxa; taxa ≤ 0 não altera; taxa ≥ 1 aciona sempre, embora possa sortear segmento unitário. |
| `route_is_valid(route, num_points)` | Retorna 1 para permutação de `0..N-1`, 0 para entrada inválida ou falha de alocação. |
| `evolve_population(current_pop, points, num_points, current_generation, max_generations)` | Cria, valida e retorna próxima população; retorna `NULL` em erro. O chamador conserva e depois libera a população anterior. |

### `src/main.c`: console

| Função | Responsabilidade |
|---|---|
| `main` | Sem argumentos abre a GUI; com três argumentos valida cenário e N, inicializa aleatoriedade, gera pontos, chama `run_scenario` e libera pontos. Usa `atoi`, que não fornece validação rigorosa de texto ou overflow. |
| `run_scenario` | Cria e avalia a população, executa gerações, registra melhor distância e tempo de CPU, imprime periodicamente e para por estagnação ou limite. Libera populações em sucesso e nos caminhos tratados de erro. |
| `print_route` | Imprime os índices e repete o primeiro no fim para mostrar o fechamento do ciclo. |

### `src/gui.c`: visualização

| Função | Responsabilidade |
|---|---|
| `load_ui_font` | Procura DejaVu Sans em caminhos relativos e junto ao executável; se não encontrar, usa fonte padrão do Raylib. |
| `draw_text`, `text_width` | Desenham e medem texto com a fonte selecionada. |
| `button`, `radio` | Desenham controles e informam clique do mouse; botão desabilitado não responde. |
| `view_transform` | Calcula limites, centro e escala uniforme com margem para acomodar os pontos na área de desenho. |
| `to_screen` | Converte coordenadas matemáticas em pixels e inverte o eixo Y. |
| `draw_route`, `draw_points` | Desenham ciclo fechado e pontos; rótulos aparecem até 60 pontos. |
| `solver_reset` | Libera população e reinicia estado, mantendo os pontos. |
| `solver_elapsed` | Retorna tempo acumulado de execução sem pausas, usando relógio da GUI. |
| `solver_start` | Reinicia estado, cria e avalia população e começa a evolução. |
| `solver_pause`, `solver_resume`, `solver_stop` | Controlam execução, tempo acumulado e estado final. |
| `solver_step` | Executa uma geração, troca populações, atualiza melhor e estagnação e aplica critérios de parada. |
| `generate_points` | Encaminha para o gerador do cenário selecionado. |
| `circle_optimum` | Calcula o perímetro teórico do polígono regular, para N ≥ 3. |
| `run_gui` | Inicializa janela, semente e fonte; processa controles e gerações por frame; desenha métricas e rota; libera recursos ao fechar. |

## 9. Como compilar e executar

O CMake exige compilador C, CMake 3.16 ou superior, Raylib e bibliotecas de sistema indicadas em `CMakeLists.txt`. O CMake usa C17, `-Wall -Wextra`, recolhe os arquivos `.c` de `src/` e copia `resources/` para junto do executável.

```bash
cmake -S . -B build
cmake --build build
./build/genetic_tsp_solver
```

A interface inicia com 20 pontos uniformes. Escolha quantidade entre 8 e 300 e cenário, clique em **Gerar pontos** e depois em **Iniciar**. Pausar/Continuar ou espaço controlam a execução; Resetar descarta a população atual. A velocidade controla 1 a 50 gerações por frame: acima de uma geração por frame, a tela pode pular gerações intermediárias. Após mudar quantidade ou cenário, gere os pontos novamente para aplicar a configuração. A GUI exibe época, melhor distância, época da melhor solução, tempo ativo, taxa de mutação, status e, no círculo, ótimo e gap.

Modo console:

```bash
./build/genetic_tsp_solver 1 20 1    # uniforme, imprime cada geração
./build/genetic_tsp_solver 2 50 100  # círculo, imprime a cada 100 gerações
```

Argumentos: cenário `1` ou `2`, N mínimo 8, intervalo de impressão. Intervalos menores que 1 são ajustados para 1. O console não aplica o limite de 300 da GUI; valores muito grandes podem exceder recursos ou limites numéricos. O resultado final informa distância, época da melhor solução, época de parada e tempo de CPU; a rota aparece nas linhas periódicas, que podem não coincidir com a última geração. Para acompanhar todas, use intervalo 1.

Alternativamente, `make` produz `./tsp` usando C11 e requer Raylib; `make test` compila e executa os testes sem Raylib, com AddressSanitizer e UndefinedBehaviorSanitizer. A GUI inicializa `srand(42)` uma vez por execução; repetir a mesma sequência de ações favorece reprodução. O console usa `time(NULL)`, portanto não oferece argumento de semente fixa. O tempo do console é tempo de CPU via `clock()`, enquanto o da GUI é tempo decorrido ativo via `GetTime()`; não devem ser comparados diretamente como a mesma métrica.

## 10. Testes, limites e leitura dos resultados

`tests/test_population.c` verifica, entre outros pontos, validade das rotas iniciais, distribuição observada do embaralhamento para N=4, fórmula do círculo, distância de um quadrado, cópia independente, memória, seleção e um pequeno fluxo de integração com elitismo. Os testes são amostrais e não provam qualidade global para todos os tamanhos ou sementes. O fluxo de integração do teste constrói seu próprio mini-AG; isso não equivale a testar todos os caminhos da interface e de `evolve_population`.

O espaço de rotas da população ocupa O(PN), e criar, validar e avaliar uma geração custa aproximadamente O(PN), além de O(Pk) para torneios e dos custos de alocação. Há duas populações simultâneas durante a evolução. Como P cresce proporcionalmente a N acima do piso, o custo por geração e a memória crescem aproximadamente como O(N²), para k fixo. O número efetivo de gerações depende da estagnação e do limite; desempenho real também depende de alocações e desenho.

Para apresentar o experimento circular, anote N, semente ou condição de execução, parâmetros, épocas intermediárias, melhor distância, ótimo, gap, época da melhor solução, motivo da parada e tempo com sua unidade e tipo. Repita execuções com sementes diferentes para avaliar variabilidade. Uma única execução com gap pequeno é evidência daquela execução, não garantia de desempenho para todos os mapas. O cenário uniforme não possui referência ótima calculada pelo programa.
