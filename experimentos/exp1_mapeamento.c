/*
 * Experimento 1 — o mapeamento pixel -> plano complexo e a normalizacao.
 *
 * Tres variantes do mesmo calculo, para isolar duas decisoes que parecem
 * arbitrarias mas mudam o resultado:
 *
 *   correto    passo = 3.0 / largura        normalizacao truncada
 *   borda      passo = 3.0 / (largura - 1)  normalizacao truncada
 *   arredonda  passo = 3.0 / largura        normalizacao arredondada
 *
 * A variante "borda" e a tentativa natural de quem quer que o ultimo pixel caia
 * exatamente no limite superior da regiao. A variante "arredonda" e a tentativa
 * natural de quem trata a intensidade como um valor continuo sendo discretizado.
 *
 * Uso: ./exp1_mapeamento <correto|borda|arredonda>
 * Imprime a imagem 4x4 do teste 1 oficial (./mandelbrot 4 4 50 1) em stdout.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#define LARGURA   4
#define ALTURA    4
#define MAX_ITER 50

int main(int argc, char *argv[])
{
    int borda, arredonda;
    double passo_re, passo_im;

    if (argc != 2) {
        fprintf(stderr, "uso: %s <correto|borda|arredonda>\n", argv[0]);
        return 1;
    }

    borda     = (strcmp(argv[1], "borda") == 0);
    arredonda = (strcmp(argv[1], "arredonda") == 0);

    if (!borda && !arredonda && strcmp(argv[1], "correto") != 0) {
        fprintf(stderr, "variante desconhecida: %s\n", argv[1]);
        return 1;
    }

    passo_re = borda ? 3.0 / (LARGURA - 1) : 3.0 / LARGURA;
    passo_im = borda ? 3.0 / (ALTURA  - 1) : 3.0 / ALTURA;

    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            double c_re = -2.0 + x * passo_re;
            double c_im = -1.5 + y * passo_im;
            double zr = 0.0;
            double zi = 0.0;
            int iter = 0;
            int valor;

            while (zr * zr + zi * zi <= 4.0 && iter < MAX_ITER) {
                double t = zr * zr - zi * zi + c_re;
                zi = 2.0 * zr * zi + c_im;
                zr = t;
                iter++;
            }

            valor = arredonda
                  ? (int)round(iter * 255.0 / MAX_ITER)
                  : (iter * 255) / MAX_ITER;

            printf(x == 0 ? "%d" : " %d", valor);
        }
        putchar('\n');
    }

    return 0;
}
