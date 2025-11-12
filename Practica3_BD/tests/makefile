
# Nombre del compilador
CC = gcc

# Flags de compilación
CFLAGS = -Wall -Wextra -g

# Nombre del ejecutable
TARGET = library

# Archivos fuente
SRCS = Library.c

# Archivos objeto
OBJS = $(SRCS:.c=.o)

# Regla por defecto
all: $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# Regla de ejecución con parámetros
run: $(TARGET)
	./$(TARGET) best\ fit test
