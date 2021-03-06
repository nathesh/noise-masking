CC=gcc
CFLAGS=-lsndfile -lfftw3 -lm -pthread -lportaudio
SOURCES=src/summation.c src/fft.c src/hc.c
EXECUTABLE=exe/mask
OBJECTS=summation.o fft.o hc.o


all : mask

mask : $(OBJECTS)
	$(CC) -o mask $(OBJECTS) $(CFLAGS)
summation.o : summation.c
	$(CC) -c summation.c
fft.o : fft.c fft.h
	$(CC) -c fft.c
hc.o : hc.c hc.h
	$(CC) -c hc.c

clean:
	rm *,o mask
