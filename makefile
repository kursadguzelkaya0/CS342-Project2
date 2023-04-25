
all: mps 

mps: mps.c
	gcc -Wall -g -o mps mps.c

clean:
	rm -fr *~ mps 