CC=gcc
CFLAGS=-lsndfile -lfftw3 -lm -pthread -lportaudio  -D_GNU_SOURCE
SOURCES=summation.c fft.c hc.c
EXECUTABLE=mask

all:
	$(CC) -g $(SOURCES) $(CFLAGS) -o $(EXECUTABLE)

clean:
	rm *o $(EXECUTABLE)
