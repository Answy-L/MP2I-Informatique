# Makefile du compresseur Huffman — projet MP2I (Romain CHEN, Mohammed MOUSSAOUI)
#
# Cibles :
#   make / make all   : compile le binaire « huffman »
#   make debug        : compile « huffman-debug » avec UBSan
#   make test         : compile puis exécute toute la suite de tests
#   make clean        : supprime les fichiers générés
#
# Les variables CC, CFLAGS et LDFLAGS peuvent être redéfinies :
#   make CC=gcc
#   make CFLAGS="-std=c17 -O0 -g"

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2
LDFLAGS ?=

# Avertissements et sanitizers pour la cible debug (clang et gcc).
CFLAGS_DEBUG ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O1 -g -fsanitize=undefined
LDFLAGS_DEBUG ?= -fsanitize=undefined

REP_INC := include

SRC := src/main.c src/huffman.c src/bit_io.c
OBJ := $(SRC:.c=.o)

.PHONY: all debug test clean

all: huffman

huffman: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(REP_INC) -Isrc -c -o $@ $<

debug: huffman-debug

huffman-debug: $(SRC) include/huffman.h src/bit_io.h
	$(CC) $(CFLAGS_DEBUG) -I$(REP_INC) -Isrc $(SRC) -o $@ $(LDFLAGS_DEBUG)

test: huffman
	bash ./tests/test_huffman.sh

clean:
	rm -f huffman huffman-debug $(OBJ)
