CC=gcc
CFLAGS=-lsndfile -lfftw3 -lm -pthread -lportaudio -Wall -D_gnu_source
SOURCES=noise.c fft.c hc.c
EXECUTABLE=mask

all:
	$(CC) -g $(SOURCES) $(CFLAGS) -o $(EXECUTABLE)

clean:
	rm *o $(EXECUTABLE)
