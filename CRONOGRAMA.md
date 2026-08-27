# Cronograma — Implementação 2 (Conjunto de Mandelbrot)

**Disciplina:** Infraestrutura de Software
**Login de entrega:** `mrbm`
**Entrega:** segunda, 31/08 — `mrbm.pdf` + `mrbm.tar` via Google Classroom
**Janela de trabalho:** quinta 27/08 → segunda 31/08 (5 dias)

---

## Regras de operação

1. `make check` verde antes de qualquer commit. Se falhar, não commita.
2. `NOTAS_RELATORIO.md` é alimentado **no dia**, nunca no dia 31. O relatório final é montagem, não redação.
3. Cada erro instrumentado gera **dois commits**: `experimento N: <sintoma observado>` e, em seguida, `bloco X: <correção>`. O `git log` conta a evolução sozinho.
4. A saída de cada experimento vai para `evidencias.log` na hora, junto com o comando exato que a produziu.
5. Experimentos vivem em `experimentos/expN_*.c` — arquivos isolados e reproduzíveis. A `main` nunca fica quebrada no histórico.

---

## Dia 0 — quinta 27/08 · fundação (~4h)

- [ ] Verificar ambiente no WSL Ubuntu: `gcc -fopenmp` e `-pthread` compilam
- [ ] **Bloco 0** — `git init`, remote no GitHub, `.gitignore`, `Makefile`, `NOTAS_RELATORIO.md`
      → commit `bloco 0: setup do projeto`
- [ ] **Bloco 1** — implementação serial: mapeamento, laço de escape, normalização, escrita do `.pgm`, `times.txt`
      → commit `bloco 1: implementacao serial`
- [ ] **Bloco 2** — `testes/check.sh`: roda os 3 gabaritos oficiais e compara os 4 `.pgm` entre si com `cmp`
      → commit `bloco 2: harness de teste`
- [ ] *(oportunista)* **Experimento 1** — se o mapeamento sair como `x * 3.0/(largura-1)` ou a normalização com `round()`, **não corrigir antes de rodar**. Deixar o teste 1 falhar, salvar a saída errada, aí corrigir.
- [ ] Registrar decisões do dia em `NOTAS_RELATORIO.md`

**Gate:** `make check` passa nos 3 testes oficiais com a implementação serial.

> O serial vale só 5% da nota, mas é o oráculo de correção dos outros 75%. Sem ele validado, nada mais é verificável.

---

## Dia 1 — sexta 28/08 · Pthreads 1, 25% (~5h)

- [ ] **Bloco 3** — Pthreads-1: divisão estática por blocos de linhas contíguas
- [ ] **Experimento 2** — `&i` compartilhado no `pthread_create`
- [ ] **Experimento 3** — resto da divisão inteira descartado (`altura / num_threads`)
- [ ] **Bloco 4** — correções + `make check` verde
      → commits `experimento 2: ...`, `experimento 3: ...`, `bloco 3: pthreads1 estatico`, `bloco 4: ...`
- [ ] Colar evidências (saídas divergentes, relatório do ThreadSanitizer) em `NOTAS_RELATORIO.md` e `evidencias.log`

**Gate:** `mandelbrot_mrbm_pthreads1.pgm` idêntico ao serial nos 3 testes oficiais.

---

## Dia 2 — sábado 29/08 · Pthreads 2 + OpenMP, 50% (~6h)

- [ ] **Bloco 5** — Pthreads-2: fila dinâmica de linhas, contador global protegido por mutex
- [ ] **Experimento 4** — `proxima_linha++` sem mutex
- [ ] **Bloco 6** — OpenMP no laço de cálculo
- [ ] **Experimento 5** — variáveis `zr`, `zi`, `iter` compartilhadas (faltou `private`)
- [ ] Rodar as medições de desempenho com imagem grande e preencher a tabela nas notas

**Gate:** os 4 `.pgm` idênticos entre si nos 3 testes oficiais. **75% da nota fechado.**

---

## Dia 3 — domingo 30/08 · robustez + medição + relatório (~5h)

- [ ] **Bloco 7** — validação de argumentos com `strtol` (`endptr` + `errno`), tratamento de falha em `malloc`, `fopen` e `pthread_create`
- [ ] **Experimento 6** — `atoi` engolindo lixo (`./mandelbrot 4 4 50 abc`)
- [ ] **Experimento 7** — `clock()` em vez de `clock_gettime` na medição de tempo
- [ ] **Bloco 8** — converter `NOTAS_RELATORIO.md` em rascunho do relatório no formato do *Guia de Relatórios*
- [ ] Gerar os gráficos de speedup (serial × OpenMP × P1 × P2)

**Gate:** rubrica 100% coberta, rascunho do relatório completo.

---

## Dia 4 — segunda 31/08 · fechamento (~3h)

- [ ] **Bloco 9** — relatório final: evidências, dificuldades, limitações, link do GitHub
- [ ] `relatorio.md` → `relatorio.pdf`
- [ ] `make clean`; conferir `evidencias.log`
- [ ] `tar -cf mrbm.tar mrbm/`
- [ ] Conferir nomes exatos: `mrbm.pdf` e `mrbm.tar`
- [ ] `git push` final
- [ ] Submeter no Classroom

**Buffer.** Se algum gate anterior escorregou, este dia absorve. Se nada escorregou, sobra para o Experimento 1 e polimento.

---

## Mapa dos experimentos

| # | Erro | Evidência de captura | Prioridade | Dia |
|---|---|---|---|---|
| 2 | `&i` compartilhado no `pthread_create` | 20 execuções divergentes + ThreadSanitizer | essencial | 1 |
| 3 | `altura / num_threads` sem tratar o resto | falha no teste 3 oficial (`10 6 40 4`) | essencial | 1 |
| 4 | contador da fila sem mutex | ThreadSanitizer aponta a corrida | essencial | 2 |
| 5 | OpenMP sem `private` | imagem corrompida e não-determinística | essencial | 2 |
| 7 | `clock()` em vez de `clock_gettime` | paralelo aparenta ser 4× mais lento | alta | 3 |
| 6 | `atoi` aceita `abc` como `0` | saída vazia / divisão por zero | média | 3 |
| 1 | mapeamento com `largura-1` + `round()` | falha no teste 1 oficial | oportunista | 0 |

Nenhum é inventado — todos são o erro que a implementação ingênua comete de verdade. Os quatro essenciais correspondem exatamente aos bugs das três implementações paralelas, que valem 75% da nota.

---

## Rubrica

| Item | Peso | Coberto no |
|---|---|---|
| Execução OpenMP | 25% | dia 2 |
| Execução Pthreads 1 | 25% | dia 1 |
| Execução Pthreads 2 | 25% | dia 2 |
| Erros + outras situações | 10% | dia 3 |
| Makefile | 5% | dia 0 |
| Relatório | 5% | contínuo |
| Execução serial | 5% | dia 0 |

---

## Checklist de entrega

- [ ] Diretório chamado exatamente `mrbm`
- [ ] Arquivo `mrbm.tar` (compactação do diretório)
- [ ] Arquivo `mrbm.pdf` (relatório)
- [ ] `Makefile` com alvo de compilação e `clean`
- [ ] Código-fonte em C
- [ ] Link do GitHub citado no relatório
- [ ] Commits atômicos alinhados às seções do relatório
- [ ] Os 4 `.pgm` gerados numa única execução e idênticos entre si
- [ ] `times.txt` com os 4 tempos
- [ ] Nada impresso em `stdout` durante a execução normal
