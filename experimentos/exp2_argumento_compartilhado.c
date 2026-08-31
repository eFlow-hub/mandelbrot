/*
 * Experimento 2 — o argumento passado ao pthread_create.
 *
 * Cada uma das N threads deveria receber um indice distinto, de 0 a N-1.
 *
 *   bug      pthread_create(..., &i)        todas apontam para a MESMA variavel
 *   correto  pthread_create(..., &ids[i])   cada uma aponta para a sua
 *
 * A thread le o valor apontado e conta quantas vezes cada indice foi entregue.
 * No modo correto, cada indice sai exatamente uma vez. No modo bug, o laco de
 * main continua incrementando i enquanto as threads ja estao rodando, entao o
 * valor lido depende de quem chega primeiro: alguns indices saem repetidos,
 * outros nao saem, e algumas threads chegam a ler N (o valor final do laco).
 *
 * Num programa real isso significa faixas de linhas calculadas duas vezes e
 * faixas nunca calculadas.
 *
 * Uso: ./exp2_argumento_compartilhado [bug|correto]
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define N 8

static int contagem[N];
static int fora_da_faixa;

static void *worker(void *bruto)
{
    int v = *(int *)bruto;

    if (v >= 0 && v < N)
        __atomic_fetch_add(&contagem[v], 1, __ATOMIC_RELAXED);
    else
        __atomic_fetch_add(&fora_da_faixa, 1, __ATOMIC_RELAXED);

    return NULL;
}

int main(int argc, char *argv[])
{
    int correto = (argc > 1 && strcmp(argv[1], "correto") == 0);
    pthread_t t[N];
    int ids[N];
    int i, repetidos = 0, ausentes = 0;

    memset(contagem, 0, sizeof contagem);
    fora_da_faixa = 0;

    for (i = 0; i < N; i++) {
        ids[i] = i;
        if (pthread_create(&t[i], NULL, worker, correto ? &ids[i] : &i) != 0) {
            fprintf(stderr, "erro: falha ao criar a thread %d\n", i);
            return 1;
        }
    }

    for (i = 0; i < N; i++)
        pthread_join(t[i], NULL);

    printf("modo: %s\n", correto ? "correto (&ids[i])" : "bug (&i)");
    for (i = 0; i < N; i++) {
        printf("  indice %d entregue a %d thread(s)%s\n",
               i, contagem[i], contagem[i] == 1 ? "" : "   <-- ERRADO");
        if (contagem[i] > 1) repetidos++;
        if (contagem[i] == 0) ausentes++;
    }
    if (fora_da_faixa)
        printf("  %d thread(s) leram um valor fora de [0,%d)\n",
               fora_da_faixa, N);

    printf("  resumo: %d indice(s) repetido(s), %d ausente(s)\n",
           repetidos, ausentes);

    return (repetidos || ausentes || fora_da_faixa) ? 1 : 0;
}
