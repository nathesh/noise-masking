CC=gcc
CFLAGS=-lsndfile -lfftw3 -lm -pthread -lportaudio
SOURCES=src/summation.c src/fft.c src/hc.c
EXECUTABLE=exe/mask

all:
	$(CC) -g $(SOURCES) $(CFLAGS) -o $(EXECUTABLE)

clean:
	rm *o $(EXECUTABLE)
