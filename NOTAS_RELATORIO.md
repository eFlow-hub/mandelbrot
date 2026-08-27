# Notas para o relatório — Conjunto de Mandelbrot

> Documento de trabalho. Vai acumulando decisões, medições, erros e evidências ao
> longo do desenvolvimento, para o relatório final não depender de memória.
> O relatório deve seguir o formato do *Guia de Relatórios* da disciplina.
>
> **Regra:** nenhum dia termina sem escrever aqui. Cada experimento entra no
> instante em que acontece, com três coisas — o comando rodado, a saída errada
> colada, e o entendimento do porquê.

---

## 0. Identificação

- **Aluno:** *(preencher)*
- **Login de entrega:** `mrbm`
- **Repositório:** *(link do GitHub)*
- **Ambiente de execução:** WSL2 Ubuntu — *(versão do kernel, `gcc --version`)*
- **Hardware:** *(modelo da CPU, núcleos físicos e lógicos — `lscpu`)*

O número de núcleos importa: é a referência contra a qual o speedup medido deve ser
lido. Um speedup de 4× em uma máquina de 4 núcleos é o teto teórico; em uma de 8,
indica que metade da capacidade ficou ociosa.

---

## 1. Especificação decodificada

Esta seção registra o que foi **inferido dos gabaritos oficiais**, não do enunciado.
O enunciado descreve a região do plano e a normalização em prosa; a forma exata das
fórmulas foi determinada por engenharia reversa dos três arquivos de teste e depois
verificada contra os três.

### 1.1 Mapeamento pixel para o plano complexo

```c
c_re = -2.0 + x * (3.0 / largura);    /* x = 0 .. largura-1 */
c_im = -1.5 + y * (3.0 / altura);     /* y = 0 .. altura-1  */
```

O divisor é `largura` / `altura`, **não** `largura-1` / `altura-1`. A diferença é
observável: com `largura-1`, o último pixel da linha cairia exatamente em
`c_re = 1.0` em vez de `0.25`, e nenhum dos três gabaritos bateria.

Confirmação independente: no teste 1 (`4 4 50 1`), a linha `y=2` mapeia para
`c_im = 0.0` e sai inteiramente com valor 255 — coerente, porque o intervalo real
`[-2, 0.25]` pertence ao conjunto. No teste 2 (`6 6 30 3`), as linhas 2 e 4 são
idênticas, assim como as linhas 1 e 5: é a simetria conjugada do conjunto em torno
do eixo real, que só aparece se o mapeamento estiver correto.

### 1.2 Laço de escape

```c
int iter = 0;
double zr = 0.0, zi = 0.0;
while (zr*zr + zi*zi <= 4.0 && iter < max_iter) {
    double t = zr*zr - zi*zi + c_re;
    zi = 2.0*zr*zi + c_im;
    zr = t;
    iter++;
}
```

Dois detalhes que mudam o resultado:

- O teste de escape usa `<= 4.0` (raio ao quadrado), avaliado **antes** de cada
  iteração. Trocar por `< 4.0` ou mover o teste para depois desloca a contagem em 1.
- O quadrado é calculado em `t` antes de sobrescrever `zr`. Atribuir `zr` primeiro
  corrompe o cálculo de `zi`, que ainda precisa do `zr` antigo.

Verificação pontual: para `c = 0.5 + 0i`, a sequência é 0,5 → 0,75 → 1,0625 →
1,6289 → 3,153, escapando em `iter = 5`. O gabarito do teste 2 traz 42 nessa
posição, e `(5 * 255) / 30 = 42`.

### 1.3 Normalização

```c
int pixel = (iter * 255) / max_iter;   /* divisão inteira, trunca */
```

Pontos dentro do conjunto atingem `iter == max_iter` e resultam em 255. A divisão é
inteira e **trunca** — não arredonda. Nos três gabaritos:

| Teste | `max_iter` | `iter` | Valor esperado | `(iter*255)/max_iter` |
|---|---|---|---|---|
| 1 | 50 | 1 | 5 | 5,1 → 5 |
| 1 | 50 | 2 | 10 | 10,2 → 10 |
| 2 | 30 | 1 | 8 | 8,5 → 8 |
| 2 | 30 | 5 | 42 | 42,5 → 42 |
| 3 | 40 | 2 | 12 | 12,75 → 12 |

O caso `iter=1, max_iter=30` é o discriminante: truncar dá 8, arredondar daria 9.
O gabarito traz 8.

### 1.4 Formato dos arquivos de saída

Apesar da extensão `.pgm`, o arquivo **não** tem cabeçalho de formato — o enunciado
pede apenas os valores de intensidade. Um valor por pixel, separados por um espaço
simples, uma linha por linha da imagem, terminador de linha Unix, **sem espaço à
direita**. Verificado por hexdump do gabarito: a linha aparece como `255 255 255 255$`.

### 1.5 Formato do `times.txt`

```
Serial: 0.000001s
OpenMP: 0.002340s
Pthreads1: 0.000028s
Pthreads2: 0.000016s
```

Formato `%s: %.6fs`. A medição cobre apenas o **cálculo** — a escrita dos arquivos
fica fora do cronômetro, senão o custo de I/O domina e o comparativo perde sentido.

---

## 2. Decisões de implementação

Preencher no formato usado na Implementação 1: a decisão, a justificativa, e uma
tabela das alternativas descartadas com o motivo de cada descarte.

### 2.1 Estrutura de dados da imagem

*(preencher — buffer linear contíguo versus vetor de ponteiros para linhas; impacto
em número de alocações e em localidade de cache)*

### 2.2 Estratégia do Pthreads-1: divisão estática por blocos

*(preencher — como o resto da divisão é distribuído; por que blocos contíguos
favorecem a localidade de cache; por que essa estratégia sofre com desbalanceamento
no Mandelbrot)*

### 2.3 Estratégia do Pthreads-2: fila dinâmica de linhas

*(preencher — contador global sob mutex; por que a região crítica contém apenas o
incremento e o cálculo fica fora dela; custo do lock contra o ganho de balanceamento)*

### 2.4 Escolha da diretiva OpenMP

*(preencher — `parallel for` e a política de `schedule` escolhida; por que as
variáveis do laço interno são declaradas dentro do escopo em vez de listadas em
`private`)*

### 2.5 Instrumentação de tempo

*(preencher — `clock_gettime(CLOCK_MONOTONIC)`; por que não `clock()`, por que não
`gettimeofday`)*

### 2.6 Validação de argumentos

*(preencher — `strtol` com `endptr` e `errno`; limites aceitos para cada parâmetro)*

---

## 3. Experimentos: erros instrumentados

Cada experimento é um erro real, cometido de propósito ou naturalmente, deixado
rodar até produzir evidência, e só então corrigido. O par de commits
(`experimento N` seguido de `bloco X`) documenta a transição no histórico do
repositório.

Template a seguir em todos: **contexto, código do erro, comando, saída observada,
diagnóstico, correção, commits, o que ficou aprendido.**

---

### Experimento 1 — mapeamento com `largura-1` e normalização com arredondamento

- **Dia:** 0 (27/08) · **Prioridade:** oportunista
- **Hipótese ingênua que leva ao erro:** distribuir os pontos de forma que o último
  pixel caia exatamente no limite superior da região, e arredondar a intensidade em
  vez de truncar.

**Código do erro:**

```c
/* preencher */
```

**Comando executado:**

```
(preencher)
```

**Saída observada contra a esperada:**

```
(preencher — colar o diff contra o gabarito do teste 1)
```

**Diagnóstico:** *(preencher)*

**Correção aplicada:** *(preencher)*

**Commits:** `experimento 1: ...` seguido de `bloco 1: ...`

**O que ficou aprendido:** *(preencher)*

---

### Experimento 2 — `&i` compartilhado no `pthread_create`

- **Dia:** 1 (28/08) · **Prioridade:** essencial
- **Hipótese ingênua que leva ao erro:** passar o endereço da variável de controle
  do laço como argumento da thread, assumindo que cada thread recebe o valor daquele
  instante.

**Código do erro:**

```c
for (int i = 0; i < num_threads; i++)
    pthread_create(&t[i], NULL, worker, &i);   /* todas leem a MESMA variável */
```

**Comando executado:** *(preencher — incluir o laço de 20 execuções repetidas)*

**Saída observada:** *(preencher — quantas execuções divergiram, e como)*

**Relatório do ThreadSanitizer (`-fsanitize=thread`):**

```
(preencher)
```

**Diagnóstico:** *(preencher — o laço avança enquanto as threads ainda leem o
endereço; o valor lido depende do escalonamento, então algumas faixas são
processadas duas vezes e outras nenhuma)*

**Correção aplicada:** *(preencher — vetor de structs, um por thread, com o id e a
faixa já resolvidos antes da criação)*

**Commits:** `experimento 2: ...` seguido de `bloco 3: ...`

**O que ficou aprendido:** *(preencher)*

---

### Experimento 3 — resto da divisão inteira descartado

- **Dia:** 1 (28/08) · **Prioridade:** essencial
- **Por que é o mais valioso do conjunto:** o próprio gabarito da disciplina expõe o
  bug. O teste 3 é `./mandelbrot 10 6 40 4` — 6 linhas divididas por 4 threads.

**Código do erro:**

```c
int linhas_por_thread = altura / num_threads;   /* 6 / 4 = 1 */
```

Quatro threads processando uma linha cada cobrem 4 das 6 linhas. As duas últimas
nunca são calculadas e permanecem com o conteúdo inicial do buffer.

**Comando executado:**

```
./mandelbrot 10 6 40 4
```

**Saída observada contra a esperada:** *(preencher — colar o diff contra o gabarito
do teste 3, destacando as duas linhas finais)*

**Diagnóstico:** *(preencher)*

**Correção aplicada:** *(preencher — distribuir o resto entre as primeiras
`altura % num_threads` threads, e tratar o caso `num_threads > altura`)*

**Commits:** `experimento 3: ...` seguido de `bloco 4: ...`

**O que ficou aprendido:** *(preencher — por que um teste com dimensões que não
dividem exatamente pelo número de threads vale mais que dez testes com dimensões
redondas)*

---

### Experimento 4 — contador da fila sem mutex

- **Dia:** 2 (29/08) · **Prioridade:** essencial
- **Hipótese ingênua que leva ao erro:** tratar `proxima_linha++` como operação
  atômica, já que é uma única instrução no código-fonte.

**Código do erro:**

```c
/* preencher — incremento do contador global sem lock */
```

**Comando executado:** *(preencher)*

**Saída observada:** *(preencher — linhas puladas e linhas duplicadas; frequência
com que o erro se manifesta em execuções repetidas)*

**Relatório do ThreadSanitizer:**

```
(preencher)
```

**Diagnóstico:** *(preencher — o incremento compila para leitura, soma e escrita;
duas threads podem ler o mesmo valor antes de qualquer escrita)*

**Correção aplicada:** *(preencher — `pthread_mutex_lock` em volta do incremento; a
thread pega a linha, solta o lock, e só então calcula, mantendo o cálculo fora da
região crítica)*

**Commits:** `experimento 4: ...` seguido de `bloco 5: ...`

**O que ficou aprendido:** *(preencher — por que este é o pior tipo de bug: não
trava, não gera erro, só produz resultado errado de vez em quando)*

---

### Experimento 5 — OpenMP sem `private`

- **Dia:** 2 (29/08) · **Prioridade:** essencial
- **Hipótese ingênua que leva ao erro:** declarar as variáveis de trabalho antes do
  laço, como se faria em código sequencial, e paralelizar só com o `pragma`.

**Código do erro:**

```c
double zr, zi; int iter;                  /* declaradas FORA do laço */
#pragma omp parallel for
for (int y = 0; y < altura; y++) { ... }  /* compartilhadas entre as threads */
```

**Comando executado:** *(preencher)*

**Saída observada:** *(preencher — imagem corrompida, e diferente a cada execução)*

**Diagnóstico:** *(preencher — variáveis declaradas fora da região paralela são
compartilhadas por padrão; cada thread sobrescreve o estado das outras a cada
iteração do laço de escape)*

**Correção aplicada:** *(preencher — declarar dentro do laço, de modo que o escopo
já as torne privadas; discutir a alternativa `private(zr,zi,iter)` explícita e por
que a primeira forma é preferível)*

**Commits:** `experimento 5: ...` seguido de `bloco 6: ...`

**O que ficou aprendido:** *(preencher)*

---

### Experimento 6 — `atoi` engolindo entrada inválida

- **Dia:** 3 (30/08) · **Prioridade:** média
- **Hipótese ingênua que leva ao erro:** usar `atoi` para converter os argumentos,
  assumindo que entrada inválida seria sinalizada de alguma forma.

**Código do erro:**

```c
int num_threads = atoi(argv[4]);   /* atoi("abc") devolve 0, sem sinalizar erro */
```

**Comando executado:**

```
./mandelbrot 4 4 50 abc
```

**Saída observada:** *(preencher — `num_threads = 0`, nenhuma thread criada, arquivo
de saída vazio ou divisão por zero)*

**Diagnóstico:** *(preencher — `atoi` não distingue "converteu zero" de "não
converteu nada"; não há como checar o erro depois do fato)*

**Correção aplicada:** *(preencher — `strtol` com verificação de `endptr` e de
`errno == ERANGE`, mais validação de faixa para cada parâmetro)*

**Commits:** `experimento 6: ...` seguido de `bloco 7: ...`

**O que ficou aprendido:** *(preencher — relação direta com o aviso do enunciado de
que outros casos de teste serão usados na correção)*

---

### Experimento 7 — `clock()` mede errado em código paralelo

- **Dia:** 3 (30/08) · **Prioridade:** alta
- **Hipótese ingênua que leva ao erro:** usar `clock()` para cronometrar, como se
  faz em programas sequenciais.

**Código do erro:**

```c
clock_t ini = clock();
/* ... cálculo paralelo ... */
double t = (double)(clock() - ini) / CLOCKS_PER_SEC;
```

**Comando executado:** *(preencher)*

**Saída observada:** *(preencher — colar o `times.txt` produzido, em que as versões
paralelas aparecem várias vezes mais lentas que a serial)*

**Diagnóstico:** *(preencher — `clock()` devolve tempo de CPU somado sobre todas as
threads; com N threads ocupadas, o valor cresce por um fator próximo de N mesmo
quando o tempo de parede cai)*

**Correção aplicada:** *(preencher — `clock_gettime(CLOCK_MONOTONIC)`, que mede
tempo de parede e é imune a ajustes do relógio do sistema)*

**Commits:** `experimento 7: ...` seguido de `bloco 7: ...`

**O que ficou aprendido:** *(preencher — como esta medição errada inverteria
completamente a conclusão do relatório sobre o ganho da paralelização)*

**Gancho para a discussão de desempenho:** o `times.txt` de referência da disciplina
traz `Serial: 0.000001s` e `OpenMP: 0.002340s` — o OpenMP mais de 2000 vezes mais
lento. Não é erro de medição: é o custo de criar e sincronizar threads dominando uma
imagem de 6×6 pixels, onde o cálculo em si é desprezível. Vale reproduzir esse caso
e contrastá-lo com uma imagem grande, onde a relação se inverte.

---

## 4. Medições de desempenho

Cada linha é a mediana de *(preencher — número)* execuções, para reduzir o ruído de
escalonamento do sistema operacional.

| Dimensões | `max_iter` | Threads | Serial | OpenMP | Pthreads 1 | Pthreads 2 |
|---|---|---|---|---|---|---|
| 6 × 6 | 30 | 3 | | | | |
| 500 × 500 | 1000 | 4 | | | | |
| 2000 × 2000 | 1000 | 4 | | | | |
| 2000 × 2000 | 1000 | 8 | | | | |

**Speedup observado:** *(preencher)*

**Pthreads 1 contra Pthreads 2, onde a estratégia dinâmica ganha:** *(preencher — o
Mandelbrot é desbalanceado por natureza, porque as linhas que cruzam o interior do
conjunto custam `max_iter` iterações por pixel enquanto as das bordas escapam em uma
ou duas; medir o desequilíbrio de carga entre as threads na versão estática)*

**Onde a estratégia dinâmica perde:** *(preencher — custo do mutex quando as linhas
são baratas e a contenção domina)*

---

## 5. Dificuldades encontradas

*(preencher ao longo do desenvolvimento — o que custou tempo e por quê)*

---

## 6. Limitações conhecidas

*(preencher — o que o programa assume, o que não trata, e por que a decisão foi
consciente)*

---

## 7. O que não foi implementado

*(preencher, se houver; declarar explicitamente é melhor que omitir)*

---

## 8. Diário de bordo

Registro corrido, um bloco por dia. Serve de matéria-prima para a seção de
metodologia do relatório e garante que a ordem dos acontecimentos não se perca.

### 27/08 — quinta

*(preencher)*

### 28/08 — sexta

*(preencher)*

### 29/08 — sábado

*(preencher)*

### 30/08 — domingo

*(preencher)*

### 31/08 — segunda

*(preencher)*
