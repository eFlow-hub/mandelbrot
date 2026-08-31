/*
 * Experimento 3 — o resto da divisao inteira na divisao estatica de linhas.
 *
 * Reproduz a estrategia 1 (faixas contiguas) nas dimensoes do teste 3 oficial:
 * ./mandelbrot 10 6 40 4 — seis linhas para quatro threads.
 *
 *   bug      tamanho = altura / num_threads              6/4 = 1, quatro faixas
 *            de uma linha cobrem 4 das 6 linhas; as duas ultimas ficam sem
 *            calcular e saem com o conteudo inicial do buffer (zero).
 *
 *   correto  as primeiras (altura % num_threads) threads recebem uma linha a
 *            mais: 2 threads com 2 linhas e 2 threads com 1 linha = 6.
 *
 * Uso: ./exp3_resto_perdido [bug|correto]
 * Imprime a imagem, comparavel com testes/esperado/teste3.txt.
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define LARGURA   10
#define ALTURA     6
#define MAX_ITER  40
#define NTHREADS   4

/* static: zerado na inicializacao, entao linha nao calculada aparece como 0. */
static unsigned char img[ALTURA][LARGURA];

typedef struct {
    int y_ini;
    int y_fim;
} Faixa;

static unsigned char intensidade(int x, int y)
{
    double c_re = -2.0 + x * (3.0 / LARGURA);
    double c_im = -1.5 + y * (3.0 / ALTURA);
    double zr = 0.0, zi = 0.0;
    int iter = 0;

    while (zr * zr + zi * zi <= 4.0 && iter < MAX_ITER) {
        double t = zr * zr - zi * zi + c_re;
        zi = 2.0 * zr * zi + c_im;
        zr = t;
        iter++;
    }
    return (unsigned char)((iter * 255) / MAX_ITER);
}

static void *worker(void *bruto)
{
    Faixa *f = (Faixa *)bruto;

    for (int y = f->y_ini; y < f->y_fim; y++)
        for (int x = 0; x < LARGURA; x++)
            img[y][x] = intensidade(x, y);

    return NULL;
}

int main(int argc, char *argv[])
{
    int correto = (argc > 1 && strcmp(argv[1], "correto") == 0);
    pthread_t t[NTHREADS];
    Faixa faixas[NTHREADS];
    int base = ALTURA / NTHREADS;
    int resto = ALTURA % NTHREADS;
    int proxima = 0;
    int i;

    for (i = 0; i < NTHREADS; i++) {
        int tamanho = base + ((correto && i < resto) ? 1 : 0);

        faixas[i].y_ini = proxima;
        faixas[i].y_fim = proxima + tamanho;
        proxima += tamanho;
    }

    fprintf(stderr, "modo: %s | base=%d resto=%d | linhas cobertas: %d de %d\n",
            correto ? "correto" : "bug", base, resto, proxima, ALTURA);
    for (i = 0; i < NTHREADS; i++)
        fprintf(stderr, "  thread %d: linhas [%d, %d)\n",
                i, faixas[i].y_ini, faixas[i].y_fim);

    for (i = 0; i < NTHREADS; i++) {
        if (pthread_create(&t[i], NULL, worker, &faixas[i]) != 0) {
            fprintf(stderr, "erro: falha ao criar a thread %d\n", i);
            return 1;
        }
    }
    for (i = 0; i < NTHREADS; i++)
        pthread_join(t[i], NULL);

    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++)
            printf(x == 0 ? "%u" : " %u", img[y][x]);
        putchar('\n');
    }

    return 0;
}
