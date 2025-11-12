#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ISBNSIZE 16

typedef struct{
    int bookID;
    char isbn[ISBNSIZE];
    char *title;
    char *printedBy;
} book;

typedef struct{
    int key;         /* book ISBN */
    long int offset; /* book is stored in disk at this position */
    size_t size;     /* book record size. This is a redundant field that helps in the implementation */
} indexbook;

typedef struct {
    int *array;
    size_t used;
    size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
    /* create initial empty array of size initialSize */
    a->array = malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

void insertArray(Array *a, int element) {
    /* insert item "element" in array
       a->used is the number of used entries,
       a->size is the number of entries */
    if (a->used == a->size) {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

void freeArray(Array *a) {
    /* free memory allocated for array */
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

int main(int argc, char *argv[]) {
    char *raiz = NULL;
    char datos[256], indice[256], lista[256];
    FILE *db = NULL;
    FILE *ind = NULL;
    FILE *lst = NULL;

    if (argc != 3) {
        printf("Uso: %s <estrategia> <nombre_raiz>\n", argv[0]);
        printf("Estrategias posibles: best fit, first fit, worst fit\n");
        return 1;
    }

    char *estrategia = argv[1];
    if (strcmp(estrategia, "best") != 0 && strcmp(estrategia, "first") != 0 && strcmp(estrategia, "worst") != 0 &&
        strcmp(estrategia, "best fit") != 0 && strcmp(estrategia, "first fit") != 0 && strcmp(estrategia, "worst fit") != 0) {
        printf("Estrategia no valida. Debe ser: best fit, first fit o worst fit\n");
        return 1;
    }

    *raiz = argv[2];

    /*Creamos nombres de ficheros*/
    snprintf(datos, sizeof(datos), "%s.db", raiz);
    snprintf(indice, sizeof(indice), "%s.ind", raiz);
    snprintf(lista, sizeof(lista), "%s.lst", raiz);

    printf("Estrategia seleccionada: %s\n", estrategia);
    printf("Archivos a crear:\n");
    printf("Datos: %s\n", datos);
    printf("Indice: %s\n", indice);
    printf("Listado: %s\n", lista);

    db = fopen(datos, "w");
    if (!db) {
        perror("Error al crear el fichero de datos");
        return 1;
    }
        ind = fopen(datos, "w");
    if (!ind) {
        perror("Error al crear el fichero de datos");
        return 1;
    }
        lst = fopen(datos, "w");
    if (!lst) {
        perror("Error al crear el fichero de datos");
        return 1;
    }

    while()
    return 0;
}











    