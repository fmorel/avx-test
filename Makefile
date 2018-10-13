all : mat_mult

mat_mult : mat_mult.c
	gcc -O2 -mavx2 $< -o $@
