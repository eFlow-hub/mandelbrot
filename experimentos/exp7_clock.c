/*
 * Experimento 7 — clock() mede a coisa errada em codigo paralelo.
 *
 * clock() devolve tempo de CPU consumido pelo processo, somado sobre todas as
 * threads. clock_gettime(CLOCK_MONOTONIC) devolve tempo de parede.
 *
 * Numa versao serial os dois coincidem, e por isso o erro passa despercebido
 * enquanto so existe a implementacao serial. Com N threads ocupadas, clock()
 * cresce por um fator proximo de N justamente quando o tempo real cai — a
 * medicao inverte a conclusao sobre o ganho da paralelizacao.
 *
 * Uso: ./exp7_clock [num_threads]
 */

#define _POSIX_C_SOURCE 200809L

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LARGURA   1200
#define ALTURA    1200
#define MAX_ITER  2000

/*
 * SEM static, e com a soma impressa no fim.
 *
 * Na primeira versao img era static e nunca lido. O gcc -O2 concluiu que as
 * escritas eram mortas, removeu-as, e com elas todo o laco de calculo: o
 * assembly gerado tinha 2 instrucoes de ponto flutuante no programa inteiro.
 * O benchmark media o tempo de nao fazer nada — 1200x1200 pixels em 48
 * microssegundos.
 */
unsigned char img[ALTURA][LARGURA];

static double agora(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void calcula(int num_threads)
{
    int y;

#pragma omp parallel for schedule(dynamic) num_threads(num_threads)
    for (y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
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
            img[y][x] = (unsigned char)(((long)iter * 255) / MAX_ITER);
        }
    }
}

static void mede(int num_threads)
{
    clock_t c_ini;
    double p_ini, cpu, parede;
    unsigned long soma = 0;

    c_ini = clock();
    p_ini = agora();
    calcula(num_threads);
    parede = agora() - p_ini;
    cpu = (double)(clock() - c_ini) / CLOCKS_PER_SEC;

    /* Le a imagem: sem isso o compilador tem licenca para apagar o calculo. */
    for (int y = 0; y < ALTURA; y++)
        for (int x = 0; x < LARGURA; x++)
            soma += img[y][x];

    printf("  %2d thread(s): clock()=%8.4fs   clock_gettime()=%8.4fs   razao=%5.2fx   [soma=%lu]\n",
           num_threads, cpu, parede, parede > 0 ? cpu / parede : 0.0, soma);
}

int main(int argc, char *argv[])
{
    int max = (argc > 1) ? atoi(argv[1]) : 8;

    printf("imagem %dx%d, max_iter=%d\n", LARGURA, ALTURA, MAX_ITER);
    printf("clock() mede CPU somada; clock_gettime() mede tempo de parede\n\n");

    for (int n = 1; n <= max; n *= 2)
        mede(n);

    printf("\nA razao acompanha o numero de threads: e o fator pelo qual\n");
    printf("clock() superestima o tempo da versao paralela.\n");

    return 0;
}
