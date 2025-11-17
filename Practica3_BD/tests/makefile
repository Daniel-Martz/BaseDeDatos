
# Nombre del compilador
CC = gcc

# Flags de compilación
CFLAGS = -Wall -Wextra -g -ansi

# Nombre del ejecutable
TARGET = library 

# Archivos fuente
SRCS = library.c

# Archivos objeto
OBJS = library.o

# Regla por defecto
all: $(TARGET)

# Compilar el ejecutable
library: library.o
	$(CC) $(CFLAGS) -o library $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

clean_bin:
	rm -f test.db test.ind test.lst

# Regla de ejecución con parámetros
run: $(TARGET)
	./$(TARGET) best fit test

run_test1: library
	expect ./add_data_test.sh

run_test2: library
	expect ./add_index_test.sh
