/*
 * Experimento 5 — variaveis compartilhadas numa regiao paralela do OpenMP.
 *
 * A regra do OpenMP: variaveis declaradas FORA da regiao paralela sao
 * compartilhadas por padrao; as declaradas DENTRO sao privadas de cada thread.
 *
 *   bug      zr, zi e iter declaradas antes do #pragma. As threads sobrescrevem
 *            o estado umas das outras no meio do laco de escape.
 *   correto  declaradas dentro do laco, entao cada thread tem as suas.
 *
 * O modo bug nao trava nem gera erro: produz uma imagem diferente a cada
 * execucao, porque o resultado passa a depender do entrelacamento das threads.
 *
 * Uso: ./exp5_openmp_sem_private [bug|correto]
 * Imprime a imagem. Rodar duas vezes e comparar as saidas entre si.
 */

#include <omp.h>
#include <stdio.h>
#include <string.h>

#define LARGURA   60
#define ALTURA    60
#define MAX_ITER 200

static unsigned char img[ALTURA][LARGURA];

int main(int argc, char *argv[])
{
    int correto = (argc > 1 && strcmp(argv[1], "correto") == 0);
    int y;

    /* Compartilhadas por estarem declaradas fora da regiao paralela. */
    double zr, zi;
    int iter;

    if (correto) {
#pragma omp parallel for schedule(dynamic)
        for (y = 0; y < ALTURA; y++) {
            for (int x = 0; x < LARGURA; x++) {
                /* Privadas: declaradas dentro do laco. */
                double p_zr = 0.0, p_zi = 0.0;
                int p_iter = 0;
                double c_re = -2.0 + x * (3.0 / LARGURA);
                double c_im = -1.5 + y * (3.0 / ALTURA);

                while (p_zr * p_zr + p_zi * p_zi <= 4.0 && p_iter < MAX_ITER) {
                    double t = p_zr * p_zr - p_zi * p_zi + c_re;
                    p_zi = 2.0 * p_zr * p_zi + c_im;
                    p_zr = t;
                    p_iter++;
                }
                img[y][x] = (unsigned char)((p_iter * 255) / MAX_ITER);
            }
        }
    } else {
#pragma omp parallel for schedule(dynamic)
        for (y = 0; y < ALTURA; y++) {
            for (int x = 0; x < LARGURA; x++) {
                double c_re = -2.0 + x * (3.0 / LARGURA);
                double c_im = -1.5 + y * (3.0 / ALTURA);

                zr = 0.0;
                zi = 0.0;
                iter = 0;
                while (zr * zr + zi * zi <= 4.0 && iter < MAX_ITER) {
                    double t = zr * zr - zi * zi + c_re;
                    zi = 2.0 * zr * zi + c_im;
                    zr = t;
                    iter++;
                }
                img[y][x] = (unsigned char)((iter * 255) / MAX_ITER);
            }
        }
    }

    fprintf(stderr, "modo: %s\n", correto ? "correto" : "bug (compartilhadas)");

    for (y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++)
            printf(x == 0 ? "%u" : " %u", img[y][x]);
        putchar('\n');
    }

    return 0;
}
