all : mat_mult neutron

CFLAGS += -lm
mat_mult : mat_mult.c
	gcc -O3 -mavx2 -msse4.2 -march=native -lopenblas -fopenmp $< -o $@

