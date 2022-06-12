SOURCES=$(wildcard *.c)
# HEADERS=$(SOURCES:.c=.h)
FLAGS=-DDEBUG -g

all: run

# main: $(SOURCES) $(HEADERS)
main: $(SOURCES)
	mpicc $(SOURCES) $(FLAGS) -o main

clear: clean

clean:
	rm main a.out

run: main
	mpirun -np 4 --oversubscribe --map-by node ./main
