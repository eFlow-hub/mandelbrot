CC = gcc
CFLAGS = -Wall -Wextra -O2 -fopenmp -pthread

mandelbrot: mandelbrot.c
	$(CC) $(CFLAGS) -o $@ $<

check: mandelbrot
	@bash testes/check.sh

clean:
	rm -f mandelbrot mandelbrot_mrbm_*.pgm times.txt
	rm -f experimentos/exp1_mapeamento

.PHONY: check clean
