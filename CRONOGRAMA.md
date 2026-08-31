# Cronograma — Implementação 2 (Conjunto de Mandelbrot)

**Disciplina:** Infraestrutura de Software
**Login de entrega:** `mrbm`
**Entrega:** segunda, 31/08 — `mrbm.pdf` + `mrbm.tar` via Google Classroom
**Janela de trabalho:** sexta 28/08 (noite) → segunda 31/08

> **Correção de data.** A primeira versão deste cronograma foi montada assumindo
> que o dia 0 seria quinta 27/08, resultando em cinco dias. Os relógios do Windows
> e do WSL confirmaram sexta 28/08: são **quatro dias**. O Bloco 0 fechou em
> minutos em vez das 4h previstas, então o atraso de calendário foi absorvido —
> a fundação inteira já está pronta na noite de sexta.

---

## Regras de operação

1. `make check` verde antes de qualquer commit. Se falhar, não commita.
2. `NOTAS_RELATORIO.md` é alimentado **no dia**, nunca no dia 31. O relatório final
   é montagem, não redação.
3. Cada erro instrumentado gera **dois commits**: `experimento N: <sintoma
   observado>` e, em seguida, `bloco X: <correção>`. O `git log` conta a evolução
   sozinho.
4. A saída de cada experimento vai para `evidencias.log` na hora, junto com o
   comando exato que a produziu.
5. Experimentos vivem em `experimentos/expN_*.c` — arquivos isolados e
   reproduzíveis. A `main` nunca fica quebrada no histórico.

---

## Dia 0 — sexta 28/08, noite · fundação ✅ CONCLUÍDO

- [x] Ambiente verificado no WSL Ubuntu: `gcc -fopenmp`, `-pthread` e
      `-fsanitize=thread` compilam e executam
- [x] **Bloco 0** — `git init`, `.gitignore`, `.gitattributes`, `Makefile`,
      gabaritos oficiais em `testes/`
      → `c2fb695`
- [x] **Bloco 1** — implementação serial: mapeamento, laço de escape,
      normalização, escrita do `.pgm`, `times.txt`
      → `78421f9`
- [x] **Experimento 1** — mapeamento com `largura-1` e normalização arredondada
      → `0316cce`
- [x] **Bloco 2** — `testes/check.sh` e alvo `make check`
      → `2d89f24`
- [x] Criar o repositório no GitHub e adicionar o remote → https://github.com/eFlow-hub/mandelbrot

**Gate:** ✅ `make check` passa nos 3 testes oficiais com a implementação serial.

---

## Dia 1 — sábado 29/08 · Pthreads 1 e 2, 50% (~6h)

- [ ] **Bloco 3** — Pthreads-1: divisão estática por blocos de linhas contíguas
- [ ] **Experimento 2** — `&i` compartilhado no `pthread_create`
- [ ] **Experimento 3** — resto da divisão inteira descartado
- [ ] **Bloco 4** — correções + `make check` verde
- [ ] **Bloco 5** — Pthreads-2: fila dinâmica de linhas sob mutex
- [ ] **Experimento 4** — `proxima_linha++` sem mutex
- [ ] Evidências em `evidencias.log` e diário em `NOTAS_RELATORIO.md`

**Gate:** `pthreads1.pgm` e `pthreads2.pgm` idênticos ao serial nos 3 testes.

---

## Dia 2 — domingo 30/08 · OpenMP + robustez + medições (~6h)

- [ ] **Bloco 6** — OpenMP no laço de cálculo
- [ ] **Experimento 5** — variáveis compartilhadas por falta de `private`
- [ ] **Bloco 7** — validação de argumentos com `strtol`, tratamento de falha em
      `malloc`, `fopen` e `pthread_create`
- [ ] **Experimento 6** — `atoi` engolindo entrada inválida
- [ ] **Experimento 7** — `clock()` em vez de `clock_gettime`
- [ ] Medições de desempenho e gráficos de speedup
- [ ] **Bloco 8** — rascunho do relatório no formato do *Guia de Relatórios*

**Gate:** os 4 `.pgm` idênticos entre si, rubrica 100% coberta, rascunho pronto.

---

## Dia 3 — segunda 31/08 · fechamento e entrega (~3h)

- [ ] **Bloco 9** — relatório final: evidências, dificuldades, limitações, link do
      GitHub
- [ ] `relatorio.md` → `relatorio.pdf`
- [ ] `make clean`; conferir `evidencias.log`
- [ ] `tar -cf mrbm.tar mrbm/`
- [ ] Conferir nomes exatos: `mrbm.pdf` e `mrbm.tar`
- [ ] `git push` final
- [ ] Submeter no Classroom

**Buffer.** Segunda ficou deliberadamente leve. Se algum gate anterior escorregar,
este dia absorve sem comprometer a entrega.

---

## Mapa dos experimentos

| # | Erro | Evidência de captura | Prioridade | Dia | Estado |
|---|---|---|---|---|---|
| 1 | mapeamento com `largura-1` + `round()` | falha no teste 1 oficial | oportunista | 0 | ✅ `0316cce` |
| 2 | `&i` compartilhado no `pthread_create` | 5 execuções divergentes | essencial | 3 | concluído |
| 3 | `altura / num_threads` sem tratar o resto | falha no teste 3 oficial (`10 6 40 4`) | essencial | 3 | concluído |
| 4 | contador da fila sem mutex | ThreadSanitizer aponta a corrida | essencial | 3 | concluído |
| 5 | OpenMP sem `private` | 5 execuções, 5 imagens (só com `-O0`) | essencial | 3 | concluído |
| 6 | `atoi` aceita `abc` como `0` | `atoi("9999999999")` = 1410065407 | média | 3 | concluído |
| 7 | `clock()` em vez de `clock_gettime` | razão 15,89× com 16 threads | alta | 3 | concluído |

Nenhum é inventado — todos são o erro que a implementação ingênua comete de
verdade. Os quatro essenciais correspondem exatamente aos bugs das três
implementações paralelas, que valem 75% da nota.

---

## Rubrica

| Item | Peso | Coberto no | Estado |
|---|---|---|---|
| Execução OpenMP | 25% | dia 2 | ✅ |
| Execução Pthreads 1 | 25% | dia 1 | ✅ |
| Execução Pthreads 2 | 25% | dia 1 | ✅ |
| Erros + outras situações | 10% | dia 2 | ✅ |
| Makefile | 5% | dia 0 | ✅ |
| Relatório | 5% | contínuo | ✅ |
| Execução serial | 5% | dia 0 | ✅ |

---

## Checklist de entrega

- [x] Diretório chamado exatamente `mrbm`
- [ ] Arquivo `mrbm.tar` (compactação do diretório)
- [x] Arquivo `mrbm.pdf` (relatório)
- [x] `Makefile` com alvo de compilação e `clean`
- [x] Código-fonte em C com as quatro implementações
- [x] Link do GitHub citado no relatório
- [x] Commits atômicos alinhados às seções do relatório
- [x] Os 4 `.pgm` gerados numa única execução e idênticos entre si
- [x] `times.txt` com os 4 tempos
- [x] Nada impresso em `stdout` durante a execução normal
