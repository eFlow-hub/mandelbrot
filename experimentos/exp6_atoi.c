/*
 * Experimento 6 — atoi nao distingue "converteu zero" de "nao converteu".
 *
 * atoi devolve int e nada mais. Diante de "abc" devolve 0; diante de um numero
 * maior que INT_MAX o comportamento e indefinido. Nos dois casos o programa
 * segue adiante com um valor invalido.
 *
 * No mandelbrot isso vira num_threads = 0 (nenhuma thread criada, imagem vazia)
 * ou max_iteracoes = 0 (divisao por zero na normalizacao, SIGFPE).
 *
 * strtol devolve as duas informacoes que faltam: onde a leitura parou (endptr)
 * e se houve estouro de faixa (errno == ERANGE).
 *
 * Uso: ./exp6_atoi
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static void compara(const char *texto)
{
    char *fim;
    long valor;
    int valido;

    errno = 0;
    valor = strtol(texto, &fim, 10);
    valido = !(fim == texto || *fim != '\0' || errno == ERANGE ||
               valor < 1 || valor > INT_MAX);

    printf("  entrada %-14s atoi=%-12d strtol=%-12ld  %s\n",
           texto, atoi(texto), valor,
           valido ? "aceito" : "REJEITADO pelo strtol");
}

int main(void)
{
    printf("comparacao entre atoi e strtol na leitura de um argumento:\n");

    compara("\"4\"");
    compara("4");
    compara("abc");
    compara("3x");
    compara("");
    compara("0");
    compara("-2");
    compara("9999999999");
    compara("2147483647");

    printf("\nconsequencia no programa real:\n");
    printf("  num_threads=0     nenhuma thread criada, imagem fica vazia\n");
    printf("  max_iteracoes=0   divisao por zero na normalizacao (SIGFPE)\n");

    return 0;
}
