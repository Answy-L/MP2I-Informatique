# Makefile — projet détecteur de formes (MP2I)
# make        : compile tout et produit l'exécutable "formes"
# make clean  : supprime les .o et l'exécutable

CC = gcc
CFLAGS = -Wall -Wextra -std=c99

formes: main.o bmp.o
	$(CC) $(CFLAGS) -o formes main.o bmp.o

main.o: main.c bmp.h
	$(CC) $(CFLAGS) -c main.c

bmp.o: bmp.c bmp.h
	$(CC) $(CFLAGS) -c bmp.c

clean:
	rm -f *.o formes
