CC = gcc
CFLAGS = -lX11 -lm -Wall -O3
clean:
	rm -f hw2_NB_pthread hw2_NB_openmp hw2_NB_single
all:
	$(CC) hw2_NB_single.c -o hw2_NB_single $(CFLAGS)
	$(CC) hw2_NB_BHalgo.c -o hw2_NB_BHalgo $(CFLAGS) -pthread
	$(CC) hw2_NB_openmp.c -o hw2_NB_openmp $(CFLAGS) -fopenmp
	$(CC) hw2_NB_pthread.c -o hw2_NB_pthread $(CFLAGS) -pthread
single:
	$(CC) hw2_NB_single.c -o hw2_NB_single $(CFLAGS)
pthread:
	$(CC) hw2_NB_pthread.c -o hw2_NB_pthread $(CFLAGS) -pthread
openmp:
	$(CC) hw2_NB_openmp.c -o hw2_NB_openmp $(CFLAGS) -fopenmp
BH:
	$(CC) hw2_NB_BHalgo.c -o hw2_NB_BHalgo $(CFLAGS) -pthread
