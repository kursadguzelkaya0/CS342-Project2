
all: mps mps_cv

mps: mps.c
	gcc -Wall -g -o mps -lpthread mps.c -lm

mps_cv: mps_cv.c
	gcc -Wall -g -o mps_cv -lpthread mps_cv.c -lm

clean:
	rm -fr *~ mps mps_cv