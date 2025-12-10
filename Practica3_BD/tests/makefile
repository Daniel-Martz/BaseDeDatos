
# Nombre del compilador
CC = gcc

# Flags de compilación
CFLAGS = -Wall -Wextra -g -ansi

# Nombre del ejecutable
TARGET = permissions library 

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

firstfit:
	@echo Running first_fit
	@valgrind --leak-check=full -s ./library first_fit test

bestfit:
	@echo Running best_fit
	@valgrind --leak-check=full -s ./library best_fit test_reuse

worstfit:
	@echo Running worst_fit_fit
	@valgrind --leak-check=full -s ./library worst_fit test_reuse

#Para que no haya problemas cunado un test necesite ejecutar otro
permissions:
	chmod +x *.sh

clean:
	rm -f $(OBJS) $(TARGET)

clean_bin:
	rm -f test.db test.ind test.lst

# Regla de ejecución con parámetros
run: $(TARGET)
	./$(TARGET) best fit test

run_tests: run_test0 run_test1 run_test2 run_test3 run_test4 run_test5 run_test6 run_test7 run_test8

run_test0: library
	expect ./cli_tests.sh

run_test1: library
	expect ./add_data_test.sh

run_test2: library
	expect ./add_index_test.sh

run_test3: library
	expect ./add_delete_test_01.sh

run_test4: library
	expect ./add_delete_test_02.sh

run_test5: library
	expect ./add_delete_test_03.sh

run_test6: library
	expect ./add_delete_test_04.sh 

run_test7: library
	expect ./many_entries.sh

run_test8: library
	expect ./reload_index.sh

run_test9: library
	expect ./test_del_add_ff.sh

run_test10: library
	expect ./test_del_add_wf.sh

run_test11: library
	expect ./test_del_add_bf.sh
