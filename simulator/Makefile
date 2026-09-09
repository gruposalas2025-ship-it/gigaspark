# Gigaspark OS - Simulador Nativo (Makefile simple)
# Uso: make        (compilar)
#      make run    (compilar y ejecutar)
#      make clean  (limpiar)

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
CFLAGS += -I../sdk -I../common/include
CFLAGS += $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2) -lm

SRC = main.c sdk_stubs.c
OBJ = $(SRC:.c=.o)
BIN = gigaspark_sim

.PHONY: all clean run

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) $(OBJ)
