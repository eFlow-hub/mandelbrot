/*
 * Conjunto de Mandelbrot — Implementacao 2
 * Infraestrutura de Software
 *
 * Uso: ./mandelbrot [largura] [altura] [max_iteracoes] [num_threads]
 */

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
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
 * delas precisa de sincronizacao para escrever no buffer: cada thread so toca
 * posicoes que nenhuma outra toca.
 */
static void calcula_linha(unsigned char *linha, int y, const Config *cfg)
{
    for (int x = 0; x < cfg->largura; x++)
        linha[x] = intensidade(x, y, cfg);
}

/* ------------------------------------------------------------------ serial */

static int calcula_serial(unsigned char *img, const Config *cfg)
{
    for (int y = 0; y < cfg->altura; y++)
        calcula_linha(img + (size_t)y * cfg->largura, y, cfg);
    return 0;
}

/* -------------------------------------------------------------- pthreads 1 */
/*
 * Estrategia 1: divisao estatica por faixas de linhas contiguas.
 *
 * Cada thread recebe [y_ini, y_fim) antes de comecar e nunca mais consulta
 * estado compartilhado. Zero sincronizacao no laco, e as linhas de uma thread
 * ficam vizinhas na memoria, o que favorece a localidade de cache.
 *
 * O custo: o Mandelbrot e desbalanceado. Linhas que cruzam o interior do
 * conjunto custam max_iter iteracoes por pixel, as das bordas escapam em uma ou
 * duas. A thread que pegar a faixa central termina muito depois das outras, e o
 * tempo total e o da mais lenta.
 */
typedef struct {
    unsigned char *img;
    const Config  *cfg;
    int            y_ini;   /* inclusivo */
    int            y_fim;   /* exclusivo */
} FaixaArg;

static void *worker_faixa(void *bruto)
{
    FaixaArg *a = (FaixaArg *)bruto;

    for (int y = a->y_ini; y < a->y_fim; y++)
        calcula_linha(a->img + (size_t)y * a->cfg->largura, y, a->cfg);

    return NULL;
}

static int calcula_pthreads1(unsigned char *img, const Config *cfg)
{
    pthread_t *threads;
    FaixaArg  *args;
    int base, resto, proxima_linha, criadas, i;
    int falhou = 0;

    threads = malloc((size_t)cfg->num_threads * sizeof *threads);
    args    = malloc((size_t)cfg->num_threads * sizeof *args);
    if (threads == NULL || args == NULL) {
        fprintf(stderr, "erro: falha ao alocar as estruturas das threads\n");
        free(threads);
        free(args);
        return -1;
    }

    /*
     * A divisao inteira descarta o resto: com altura=6 e 4 threads, 6/4 da 1 e
     * quatro threads de uma linha cobririam so 4 das 6 linhas. As duas ultimas
     * ficariam sem calcular. Por isso as primeiras (altura % num_threads)
     * threads recebem uma linha a mais.
     */
    base  = cfg->altura / cfg->num_threads;
    resto = cfg->altura % cfg->num_threads;

    proxima_linha = 0;
    for (i = 0; i < cfg->num_threads; i++) {
        int tamanho = base + (i < resto ? 1 : 0);

        args[i].img   = img;
        args[i].cfg   = cfg;
        args[i].y_ini = proxima_linha;
        args[i].y_fim = proxima_linha + tamanho;
        proxima_linha += tamanho;
    }

    /*
     * Cada thread recebe o endereco do SEU proprio elemento de args[]. Passar
     * &i faria todas lerem a mesma variavel, que o laco continua alterando.
     */
    for (criadas = 0; criadas < cfg->num_threads; criadas++) {
        if (pthread_create(&threads[criadas], NULL, worker_faixa,
                           &args[criadas]) != 0) {
            fprintf(stderr, "erro: falha ao criar a thread %d\n", criadas);
            falhou = 1;
            break;
        }
    }

    /* Espera inclusive no caminho de erro: threads ja criadas usam args[]. */
    for (i = 0; i < criadas; i++)
        pthread_join(threads[i], NULL);

    free(threads);
    free(args);
    return falhou ? -1 : 0;
}

/* -------------------------------------------------------------- pthreads 2 */
/*
 * Estrategia 2: fila dinamica de linhas.
 *
 * Em vez de repartir as linhas antes de comecar, existe um unico contador
 * compartilhado. Cada thread pega a proxima linha livre, calcula, e volta para
 * pegar outra, ate a fila esvaziar. Quem pega uma linha barata volta logo; quem
 * pega uma cara demora mais e pega menos linhas no total. A carga se equilibra
 * sozinha, sem ninguem precisar saber o custo de cada linha de antemao.
 *
 * O contador e o unico estado compartilhado, e le-e-incrementa nao e atomico:
 * compila para leitura, soma e escrita, entao duas threads podem ler o mesmo
 * valor antes de qualquer escrita e processar a mesma linha duas vezes,
 * deixando outra sem calcular. Por isso o mutex.
 *
 * A regiao critica contem apenas o incremento. O calculo fica FORA dela — se
 * ficasse dentro, as threads se serializariam e a versao paralela seria mais
 * lenta que a serial, pagando o custo do lock sem nenhum ganho.
 */
typedef struct {
    unsigned char  *img;
    const Config   *cfg;
    int             proxima;
    pthread_mutex_t mutex;
} FilaArg;

static void *worker_fila(void *bruto)
{
    FilaArg *f = (FilaArg *)bruto;

    for (;;) {
        int y;

        pthread_mutex_lock(&f->mutex);
        y = f->proxima++;
        pthread_mutex_unlock(&f->mutex);

        if (y >= f->cfg->altura)
            break;

        calcula_linha(f->img + (size_t)y * f->cfg->largura, y, f->cfg);
    }

    return NULL;
}

static int calcula_pthreads2(unsigned char *img, const Config *cfg)
{
    pthread_t *threads;
    FilaArg    fila;
    int criadas, i;
    int falhou = 0;

    threads = malloc((size_t)cfg->num_threads * sizeof *threads);
    if (threads == NULL) {
        fprintf(stderr, "erro: falha ao alocar as estruturas das threads\n");
        return -1;
    }

    fila.img     = img;
    fila.cfg     = cfg;
    fila.proxima = 0;
    if (pthread_mutex_init(&fila.mutex, NULL) != 0) {
        fprintf(stderr, "erro: falha ao inicializar o mutex\n");
        free(threads);
        return -1;
    }

    for (criadas = 0; criadas < cfg->num_threads; criadas++) {
        if (pthread_create(&threads[criadas], NULL, worker_fila, &fila) != 0) {
            fprintf(stderr, "erro: falha ao criar a thread %d\n", criadas);
            falhou = 1;
            break;
        }
    }

    /* Espera antes de destruir o mutex: ele vive na pilha desta funcao. */
    for (i = 0; i < criadas; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&fila.mutex);
    free(threads);
    return falhou ? -1 : 0;
}

/* ------------------------------------------------------------------- saida */

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

/* ---------------------------------------------------------------- execucao */

typedef int (*Implementacao)(unsigned char *, const Config *);

static const struct {
    const char   *rotulo;   /* como aparece no times.txt */
    const char   *sufixo;   /* como aparece no nome do .pgm */
    Implementacao calcula;
} IMPLEMENTACOES[] = {
    { "Serial",    "serial",    calcula_serial    },
    { "Pthreads1", "pthreads1", calcula_pthreads1 },
    { "Pthreads2", "pthreads2", calcula_pthreads2 },
};

#define NUM_IMPLEMENTACOES \
    ((int)(sizeof IMPLEMENTACOES / sizeof IMPLEMENTACOES[0]))

static int escreve_tempos(const double tempos[])
{
    FILE *f = fopen("times.txt", "w");
    if (f == NULL) {
        fprintf(stderr, "erro: nao foi possivel criar 'times.txt'\n");
        return -1;
    }

    for (int i = 0; i < NUM_IMPLEMENTACOES; i++)
        fprintf(f, "%s: %.6fs\n", IMPLEMENTACOES[i].rotulo, tempos[i]);

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
    double tempos[NUM_IMPLEMENTACOES];

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

    for (int i = 0; i < NUM_IMPLEMENTACOES; i++) {
        char nome[64];
        clock_t ini;

        ini = clock();
        if (IMPLEMENTACOES[i].calcula(img, &cfg) != 0) {
            free(img);
            return EXIT_FAILURE;
        }
        tempos[i] = (double)(clock() - ini) / CLOCKS_PER_SEC;

        snprintf(nome, sizeof nome, "mandelbrot_" LOGIN "_%s.pgm",
                 IMPLEMENTACOES[i].sufixo);

        if (escreve_imagem(nome, img, &cfg) != 0) {
            free(img);
            return EXIT_FAILURE;
        }
    }

    free(img);

    if (escreve_tempos(tempos) != 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
