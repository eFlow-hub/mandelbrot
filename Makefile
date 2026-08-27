CC = gcc
CFLAGS = -Wall -Wextra -O2 -fopenmp -pthread

mandelbrot: mandelbrot.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f mandelbrot mandelbrot_mrbm_*.pgm times.txt

.PHONY: clean
