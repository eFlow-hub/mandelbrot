/*
 * Experimento 4 — o contador da fila dinamica sem mutex.
 *
 * A estrategia 2 entrega linhas por um contador compartilhado. O incremento
 * parece uma operacao unica no codigo-fonte, mas compila para tres passos:
 * ler, somar, escrever. Duas threads podem ler o mesmo valor antes de qualquer
 * escrita, processar a mesma linha duas vezes, e deixar outra sem processar.
 *
 *   bug      y = proxima++;                       sem protecao
 *   correto  lock; y = proxima++; unlock;         incremento indivisivel
 *
 * O programa conta quantas vezes cada linha foi processada. O esperado e
 * exatamente uma vez para cada.
 *
 * Uso: ./exp4_contador_sem_mutex [bug|correto]
 * Compilar tambem com -fsanitize=thread para ver a corrida diagnosticada.
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define ALTURA   20000
#define NTHREADS     8

static int contagem[ALTURA];
static int proxima;
static int usa_mutex;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *worker(void *bruto)
{
    (void)bruto;

    for (;;) {
        int y;

        if (usa_mutex) pthread_mutex_lock(&mutex);
        y = proxima++;
        if (usa_mutex) pthread_mutex_unlock(&mutex);

        if (y >= ALTURA)
            break;

        /* Trabalho curto, so para as threads voltarem logo ao contador. */
        for (volatile int k = 0; k < 40; k++)
            ;

        __atomic_fetch_add(&contagem[y], 1, __ATOMIC_RELAXED);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t t[NTHREADS];
    int i, ausentes = 0, repetidas = 0, total = 0;

    usa_mutex = (argc > 1 && strcmp(argv[1], "correto") == 0);
    proxima = 0;
    memset(contagem, 0, sizeof contagem);

    for (i = 0; i < NTHREADS; i++) {
        if (pthread_create(&t[i], NULL, worker, NULL) != 0) {
            fprintf(stderr, "erro: falha ao criar a thread %d\n", i);
            return 1;
        }
    }
    for (i = 0; i < NTHREADS; i++)
        pthread_join(t[i], NULL);

    for (i = 0; i < ALTURA; i++) {
        total += contagem[i];
        if (contagem[i] == 0) ausentes++;
        if (contagem[i] > 1)  repetidas++;
    }

    printf("modo: %s | %d linhas, %d threads\n",
           usa_mutex ? "correto (com mutex)" : "bug (sem mutex)",
           ALTURA, NTHREADS);
    printf("  linhas nunca processadas : %d\n", ausentes);
    printf("  linhas processadas 2+ vez: %d\n", repetidas);
    printf("  total de processamentos  : %d (esperado %d)\n", total, ALTURA);

    return (ausentes || repetidas) ? 1 : 0;
}
