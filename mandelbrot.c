/*
 * Conjunto de Mandelbrot — Implementacao 2
 * Infraestrutura de Software
 *
 * Uso: ./mandelbrot [largura] [altura] [max_iteracoes] [num_threads]
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOGIN "mrbm"

/* Regiao do plano complexo fixada pelo enunciado. */
#define RE_MIN (-2.0)
#define RE_MAX  (1.0)
#define IM_MIN (-1.5)
#define IM_MAX  (1.5)

typedef struct {
    int largura;
    int altura;
    int max_iter;
    int num_threads;
} Config;

/*
 * Intensidade de um pixel, normalizada em [0, 255].
 *
 * O passo divide pela largura (nao por largura-1): o pixel x=largura-1 cai em
 * RE_MAX - passo, e nao em RE_MAX. Os gabaritos da disciplina so batem assim.
 *
 * O teste de escape usa o modulo ao quadrado (<= 4.0) para evitar a raiz
 * quadrada, e e avaliado antes de cada iteracao. A contagem resultante e
 * exatamente a que os gabaritos esperam.
 */
static unsigned char intensidade(int x, int y, const Config *cfg)
{
    double c_re = RE_MIN + x * ((RE_MAX - RE_MIN) / cfg->largura);
    double c_im = IM_MIN + y * ((IM_MAX - IM_MIN) / cfg->altura);
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (zr * zr + zi * zi <= 4.0 && iter < cfg->max_iter) {
        /* zr novo depende do zr antigo, entao guarda em t antes de sobrescrever. */
        double t = zr * zr - zi * zi + c_re;
        zi = 2.0 * zr * zi + c_im;
        zr = t;
        iter++;
    }

    /* Divisao inteira: trunca, nao arredonda. Pontos do conjunto dao 255. */
    return (unsigned char)(((long)iter * 255) / cfg->max_iter);
}

/*
 * Calcula uma linha inteira da imagem. E a unidade de trabalho que as quatro
 * implementacoes distribuem entre si — linhas sao independentes, entao nenhuma
 * delas precisa de sincronizacao para escrever no buffer.
 */
static void calcula_linha(unsigned char *linha, int y, const Config *cfg)
{
    for (int x = 0; x < cfg->largura; x++)
        linha[x] = intensidade(x, y, cfg);
}

static void calcula_serial(unsigned char *img, const Config *cfg)
{
    for (int y = 0; y < cfg->altura; y++)
        calcula_linha(img + (size_t)y * cfg->largura, y, cfg);
}

/*
 * Grava a imagem sem cabecalho de formato: um valor por pixel, separados por um
 * espaco, uma linha por linha da imagem, sem espaco a direita.
 */
static int escreve_imagem(const char *nome, const unsigned char *img,
                          const Config *cfg)
{
    FILE *f = fopen(nome, "w");
    if (f == NULL) {
        fprintf(stderr, "erro: nao foi possivel criar '%s'\n", nome);
        return -1;
    }

    for (int y = 0; y < cfg->altura; y++) {
        const unsigned char *linha = img + (size_t)y * cfg->largura;
        for (int x = 0; x < cfg->largura; x++)
            fprintf(f, x == 0 ? "%u" : " %u", linha[x]);
        fputc('\n', f);
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "erro: falha ao fechar '%s'\n", nome);
        return -1;
    }
    return 0;
}

static int escreve_tempos(const char *rotulos[], const double tempos[], int n)
{
    FILE *f = fopen("times.txt", "w");
    if (f == NULL) {
        fprintf(stderr, "erro: nao foi possivel criar 'times.txt'\n");
        return -1;
    }

    for (int i = 0; i < n; i++)
        fprintf(f, "%s: %.6fs\n", rotulos[i], tempos[i]);

    if (fclose(f) != 0) {
        fprintf(stderr, "erro: falha ao fechar 'times.txt'\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    Config cfg;
    unsigned char *img;
    clock_t ini;
    const char *rotulos[1] = { "Serial" };
    double tempos[1];

    if (argc != 5) {
        fprintf(stderr,
                "uso: %s [largura] [altura] [max_iteracoes] [num_threads]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    cfg.largura     = atoi(argv[1]);
    cfg.altura      = atoi(argv[2]);
    cfg.max_iter    = atoi(argv[3]);
    cfg.num_threads = atoi(argv[4]);

    img = malloc((size_t)cfg.largura * cfg.altura);
    if (img == NULL) {
        fprintf(stderr, "erro: falha ao alocar a imagem\n");
        return EXIT_FAILURE;
    }

    ini = clock();
    calcula_serial(img, &cfg);
    tempos[0] = (double)(clock() - ini) / CLOCKS_PER_SEC;

    if (escreve_imagem("mandelbrot_" LOGIN "_serial.pgm", img, &cfg) != 0) {
        free(img);
        return EXIT_FAILURE;
    }

    free(img);

    if (escreve_tempos(rotulos, tempos, 1) != 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
