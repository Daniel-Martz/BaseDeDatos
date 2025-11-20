#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ISBNSIZE 16
#define LIB_SIZE 80
#define BESTFIT 0
#define WORSTFIT 1
#define FIRSTFIT 2

typedef struct
{
    int bookID;
    char isbn[ISBNSIZE];
    char *title;
    char *printedBy;
} book;

typedef struct
{
    int key;         /* book id*/
    long int offset; /* book is stored in disk at this position */
    size_t size;     /* book record size. This is a redundant field that helps in the implementation */
} indexbook;

typedef struct
{
    size_t register_size;
    size_t offset;
} indexdeletedbook;

/*Estructura y funciones modificadas para guardar en el array indices y no solo enteros*/
typedef struct
{
    indexbook *array;
    size_t used;
    size_t size;
} Array_add;

void initArray(Array_add *a, size_t initialSize)
{
    /* create initial empty array of size initialSize */
    a->array = malloc(initialSize * sizeof(indexbook));
    a->used = 0;
    a->size = initialSize;
}

void insertArray(Array_add *a, indexbook element)
{
    /* insert item "element" in array
       a->used is the number of used entries,
       a->size is the number of entries */
    size_t pos = 0;

    if (a->used == a->size)
    {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(indexbook));
    }
    while (pos < a->used && a->array[pos].key < element.key)
        pos++;

    if (pos < a->used && a->array[pos].key == element.key)
    {
        printf("Record with BookID=%i exists\n", element.key);
        return;
    }
    /* Hago hueco con memmove*/
    memmove(&a->array[pos + 1], &a->array[pos], (a->used - pos) * sizeof(indexbook));

    /*inserto */
    a->array[pos] = element;
    a->used++;
}

void freeArray(Array_add *a)
{
    /* free memory allocated for array */
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

void delete(Array_add *a, int pos)
{
    if (pos < 0 || pos >= a->used)
        return;

    /* desplazamos todos los elementos hacia la izquierda */
    memmove(&a->array[pos],
            &a->array[pos + 1],
            (a->used - pos - 1) * sizeof(indexbook));

    a->used--;
}

int bin_search(Array_add *a, int key)
{
    int low = 0;
    int high = (int)a->used - 1;

    while (low <= high)
    {
        int mid = low + (high - low) / 2;

        if (a->array[mid].key == key)
            return mid;

        if (a->array[mid].key < key)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

void load_ind_to_array(Array_add *a, FILE *binario)
{
    int i, size, size_elemento, num_inds;
    indexbook *ind_aux;
    if (a == NULL || binario == NULL)
    {
        return;
    }

    ind_aux = (indexbook *)malloc(sizeof(indexbook));
    if (ind_aux == NULL)
    {
        return;
    }

    fseek(binario, 0, SEEK_END);
    size = ftell(binario);
    size_elemento = sizeof(int) + sizeof(long int) + sizeof(size_t);
    num_inds = size / size_elemento;

    for (i = 0; i < num_inds; i++)
    {
        fseek(binario, 0, i * size_elemento);
        fread(&ind_aux, size_elemento, 1, binario);
        insertArray(a, *ind_aux);
    }
}

int main(int argc, char *argv[])
{
    char *raiz = NULL;
    char aux[LIB_SIZE];
    char datos[256], indice[256], lista[256];
    char *args = NULL;
    char *token = NULL;
    long int offset;
    FILE *db = NULL;
    FILE *ind = NULL;
    FILE *lst = NULL;
    size_t size = 0;
    size_t l1, l2;
    book b;
    indexbook idx;
    Array_add indices;
    int i = 0;
    int aux_id;
    int posicion;

    if (argc != 3)
    {
        printf("Missing argument");
        printf("Uso: %s <estrategia> <nombre_raiz>\n", argv[0]);
        printf("Estrategias posibles: best fit, first fit, worst fit\n");
        return 0;
    }

    char *estrategia = argv[1];
    if (strcmp(estrategia, "best_fit") != 0 && strcmp(estrategia, "first_fit") != 0 && strcmp(estrategia, "worst_fit") != 0)
    {
        printf("Unknown search strategy unknown_search_strategy");
        printf("Estrategia no valida. Debe ser: best fit, first fit o worst fit\n");
        return 0;
    }

    raiz = argv[2];

    /*Creamos nombres de ficheros*/
    snprintf(datos, sizeof(datos), "%s.db", raiz);
    snprintf(indice, sizeof(indice), "%s.ind", raiz);
    snprintf(lista, sizeof(lista), "%s.lst", raiz);

    printf("Estrategia seleccionada: %s\n", estrategia);
    printf("Archivos a crear:\n");
    printf("Datos: %s\n", datos);
    printf("Indice: %s\n", indice);
    printf("Listado: %s\n", lista);

    db = fopen(datos, "wb");
    if (!db)
    {
        perror("Error al crear el fichero de datos");
        return 0;
    }
    ind = fopen(indice, "wb");
    if (!ind)
    {
        perror("Error al crear el fichero de datos");
        return 0;
    }
    lst = fopen(lista, "wb");
    if (!lst)
    {
        perror("Error al crear el fichero de datos");
        return 0;
    }

    /*inicializo array para indices*/
    initArray(&indices, 2);
    if (indices.array == NULL)
    {
        perror("Error al crear el array de índices");
        return 0;
    }

    load_ind_to_array(&indices, ind);

 while (1)
    {
        printf("Type command and argument/s. Type exit to stop\n");
        printf("> ");
        fflush(stdout);

        if (!fgets(aux, LIB_SIZE, stdin))
            break;

        /* eliminar salto de línea */
        aux[strcspn(aux, "\n")] = 0;

        /* comando exit */
        if (strcmp(aux, "exit") == 0)
        {
            printf("exit\n");
            break;
        }

        /* comando add */
        if (strncmp(aux, "add ", 4) == 0)
        {
            args = aux + 4;

            /* dividir por | y empiezo por bookID*/
            token = strtok(args, "|");
            if (!token)
                continue;
            b.bookID = atoi(token);

            /*Ahora el isbn*/
            token = strtok(NULL, "|");
            strncpy(b.isbn, token, ISBNSIZE);

            /*Ahora el titulo con el separador |*/
            token = strtok(NULL, "|");
            if (!(b.title = malloc(strlen(token) + 2)))
            {
                fprintf(stderr, "Error reservando memoria para title");
                return -1;
            }
            sprintf(b.title, "%s|", token);

            /*Por ultimo el autor*/
            token = strtok(NULL, "|");
            if (!(b.printedBy = malloc(strlen(token) + 1)))
            {
                fprintf(stderr, "Error reservando memoria para title");
                return -1;
            }
            sprintf(b.printedBy, "%s", token);

            /*Calculamos lo que va a ocupar*/
            l1 = strlen(b.title);
            l2 = strlen(b.printedBy);
            size = sizeof(int) + sizeof(char) * ISBNSIZE + sizeof(char) * l1 + sizeof(char) * l2;

            /* calcular offset actual del fichero */
            fseek(db, 0, SEEK_END); /* Muevo puntero al final del archivo*/
            offset = ftell(db);     /*Saco la posición donde estoy*/

            /* escribir registro en .db */
            fwrite(&size, sizeof(size_t), 1, db);       /*Primero lo que ocupa*/
            fwrite(&b.bookID, sizeof(int), 1, db);      /*Ahora escribimos el ID*/
            fwrite(b.isbn, sizeof(char), ISBNSIZE, db); /*El isbn*/
            fwrite(b.title, sizeof(char), l1, db);      /*El titulo*/
            fwrite(b.printedBy, sizeof(char), l2, db);  /*El autor*/

            /* escribir entrada la lista de indices */
            idx.key = b.bookID;
            idx.offset = offset;
            idx.size = size;

            insertArray(&indices, idx);

            printf("Record with BookID=%d has been added to the database\n", b.bookID);

            free(b.title);
            free(b.printedBy);

            continue;
        }

        /*Delete*/
        if (strncmp(aux, "del ", 4) == 0)
        {
            args = aux + 4;

            token = strtok(args, "|");
            if (!token)
                continue;
            aux_id = atoi(token);

            posicion = bin_search(&indices, aux_id);
            if (posicion == -1)
            {
                printf("Item with key %i does not exist", aux_id);
            }
        }

        /* comando printInd */
        if (strcmp(aux, "printInd") == 0)
        {

            for (i = 0; i < indices.used; i++)
            {
                printf("Entry #%d\n", i);
                printf("    key: #%d\n", indices.array[i].key);
                printf("    offset: #%ld\n", indices.array[i].offset);
                printf("    size: #%ld\n", indices.array[i].size);
            }
            continue;
        }

        printf("Unknown command.\n");
    }

    for (i = 0; i < indices.used; i++)
    {
        fwrite(&indices.array[i].key, sizeof(int), 1, ind);
        fwrite(&indices.array[i].offset, sizeof(long int), 1, ind);
        fwrite(&indices.array[i].size, sizeof(size_t), 1, ind);
    }

    fclose(db);
    fclose(ind);
    fclose(lst);
    freeArray(&indices);

    return 0;
}
