CC = gcc
CFLAGS = -Wall -Wextra -O2 -fopenmp -pthread

mandelbrot: mandelbrot.c
	$(CC) $(CFLAGS) -o $@ $<

check: mandelbrot
	@bash testes/check.sh

clean:
	rm -f mandelbrot mandelbrot_mrbm_*.pgm times.txt
	rm -f experimentos/exp1_mapeamento \
	      experimentos/exp2_argumento_compartilhado \
	      experimentos/exp3_resto_perdido \
	      experimentos/exp4_contador_sem_mutex \
	      experimentos/exp5_openmp_sem_private \
	      experimentos/exp6_atoi \
	      experimentos/exp7_clock

.PHONY: check clean
