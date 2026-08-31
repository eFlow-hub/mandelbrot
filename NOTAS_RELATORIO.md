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

- **Aluno:** Mateus Reinaux Batista Meira
- **Login de entrega:** `mrbm`
- **Repositório:** https://github.com/eFlow-hub/mandelbrot
- **Ambiente de execução:** WSL2 Ubuntu, kernel `6.6.87.2-microsoft-standard-WSL2`,
  `gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0`, GNU Make 4.4.1
- **Hardware:** Intel Core i7-13620H — 8 núcleos físicos, 16 threads lógicas
  (2 threads por núcleo)

Verificação feita no dia 0 (28/08): `-fopenmp` compila e a região paralela abre 16 threads
por padrão; `-pthread` e `-fsanitize=thread` também compilam e executam.

O número de núcleos é a referência contra a qual o speedup medido deve ser lido, e
aqui os dois números divergem. As 16 threads lógicas vêm de *hyperthreading*: dois
contextos compartilhando as mesmas unidades de execução de um núcleo físico. O
cálculo do Mandelbrot é aritmética de ponto flutuante em laço apertado, sem espera
por memória — exatamente o perfil que não se beneficia de *hyperthreading*, porque
não há bolhas de pipeline para o segundo contexto preencher.

**Previsão a confrontar com a medição:** o speedup deve crescer quase linearmente
até 8 threads e depois achatar, em vez de continuar até 16. Se a medição confirmar,
é o resultado mais interessante da seção de desempenho; se contrariar, o motivo
merece investigação.

> **A medição refutou esta previsão.** De 8 para 16 threads o speedup foi de 7,84×
> para 12,80×. O texto acima fica registrado como estava no dia 0; a explicação do
> porquê da previsão ter falhado está na seção 3.

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

## 2. Onde está o conteúdo final

Este documento foi a matéria-prima. O texto acabado está em:

- **`relatorio.html` / `relatorio.pdf`** — o relatório entregue, com as decisões de
  implementação, os sete experimentos, as medições e a análise.
- **`evidencias.log`** — saídas brutas de todos os testes e experimentos, com os
  comandos exatos que as produziram.
- **`git log`** — um commit por bloco de implementação e um por experimento, cada um
  com a justificativa técnica na mensagem.

A seção 1 acima foi mantida porque é a única parte que não cabia no relatório em
tamanho útil: a dedução completa das fórmulas a partir dos gabaritos.

---

## 3. Correções de previsão registradas

Três previsões feitas antes de medir foram desmentidas pela medição. Estão no
relatório, mas vale listá-las juntas — são o resultado mais honesto do trabalho.

**1. Hyperthreading não ajudaria.** Previsão da seção 0: o speedup acharia um teto em
8 threads, já que o Mandelbrot é aritmética pura sem espera por memória. Errado: de 8
para 16 threads o Pthreads 2 foi de 7,84× para 12,80×. O laço de escape tem uma cadeia
de dependências serial, e a latência das operações de ponto flutuante deixa bolhas no
pipeline que o segundo contexto do núcleo preenche.

**2. A corrida do contador pularia linhas.** Errado: zero linhas puladas, milhares
duplicadas. Um incremento não atômico só perde atualizações, nunca ganha, então o
contador avança devagar demais e nada fica para trás. No Mandelbrot real a imagem
sairia correta — o prejuízo é 22% de trabalho desperdiçado, não imagem errada.

**3. A corrida do OpenMP apareceria em qualquer compilação.** Errado: com `-O2` as
cinco execuções dão resultado idêntico ao da versão correta, porque o compilador
mantém as variáveis compartilhadas em registradores. A evidência precisou ser colhida
com `-O0`.

---

## 4. Diário de bordo

### 28/08 — sexta (dia 0)

**Blocos 0, 1 e 2 concluídos, mais o experimento 1.**

Ambiente verificado antes de escrever código: `gcc -fopenmp -pthread` compila e roda,
região paralela abre 16 threads, `-fsanitize=thread` compila.

Estrutura criada: `Makefile`, `.gitignore`, `.gitattributes`, `testes/oficiais` e
`testes/esperado`.

Duas decisões de infraestrutura que evitaram problema:

1. **`.gitattributes` com `* text=auto eol=lf`.** O repositório é editado no Windows e
   compilado no WSL. Sem isso o Git converteria tudo para CRLF: o `make` falharia ao
   ler o Makefile e o `diff` acusaria diferença em todas as linhas dos gabaritos, por
   um caractere invisível sem relação com o cálculo.
2. **Newline final nos gabaritos.** Normalizado uma vez ao criar `testes/esperado`, em
   vez de a cada comparação. A peculiaridade vira dado, não código.

Serial passou nos três gabaritos na primeira execução. As fórmulas da seção 1 foram
deduzidas dos arquivos de teste antes de escrever qualquer linha.

`atoi` e `clock()` ficaram deliberadamente no bloco 1 — são as formas ingênuas que os
experimentos 6 e 7 expõem depois. O `clock()` inclusive dá o resultado certo enquanto
só existe a versão serial; o erro só nasce quando as threads chegam.

**Correção de cronograma.** O plano inicial assumiu quinta 27/08 e previu cinco dias.
Os relógios confirmaram sexta 28/08: quatro.

### 29/08 e 30/08 — sábado e domingo

Sem trabalho.

### 31/08 — segunda (dia 3, entrega)

Dia único para 85% da nota. Ordem de ataque: código primeiro para travar a rubrica de
execução, depois os experimentos, depois relatório e entrega.

- **Bloco 3** (Pthreads 1, faixas estáticas) — o resto da divisão distribuído entre as
  primeiras `altura % num_threads` threads. `main` virou tabela de implementações, o
  que fez os blocos 5 e 6 custarem uma linha cada.
- **Bloco 5** (Pthreads 2, fila dinâmica sob mutex).
- **Bloco 6** (OpenMP com `schedule(dynamic)`).
- **Bloco 7** (validação com `strtol`, checagem de overflow, `clock_gettime`).
- **Experimentos 2 a 7**, todos com modo `bug` e modo `correto` no mesmo binário.
- **Medições** e **relatório**.

Dois erros de método apareceram e foram corrigidos durante o próprio trabalho:

- O benchmark do experimento 7 media o tempo de não fazer nada: o buffer era `static`
  e nunca lido, e o `gcc -O2` removeu o laço de cálculo inteiro. Dois `mulsd`/`addsd`
  no assembly do programa todo, e 1200×1200 pixels em 48 microssegundos.
- A primeira tentativa de benchmark rodou dez minutos sem terminar, porque gravar
  quatro imagens de 16 MB por execução através do `/mnt/c` domina o tempo. Movido para
  o sistema de arquivos nativo do Linux.

Verificações finais: 13 casos de erro rejeitados corretamente; os quatro arquivos
idênticos em 10 execuções seguidas de 200×200 com 16 threads; `num_threads` maior que
`altura` funciona; imagem 1×1 funciona.
