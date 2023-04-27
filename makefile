
all: mps 

mps: mps.c
	gcc -Wall -g -o mps mps.c -lm

clean:
	rm -fr *~ mps 