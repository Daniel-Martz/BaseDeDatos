#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ISBNSIZE 16
#define SIZE_MIN sizeof(size_t) + sizeof(int) + ((ISBNSIZE + 2) * sizeof(char)) /* Preguntar a la profesora en la otra clase les han dicho que solo es sizeof(size_t)*/
#define LIB_SIZE 80                                                             /* Tamano maximo que va a ocupar un libro en la memoria*/
#define BESTFIT 0
#define WORSTFIT 1
#define FIRSTFIT 2
#define NO_STRAT -1

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

/*Estructura y funciones modificadas para guardar en el array indices y no solo enteros*/
typedef struct
{
    indexdeletedbook *array;
    size_t used;
    size_t size;
} Array_del;

void initArray_add(Array_add *a, size_t initialSize)
{
    /* create initial empty array of size initialSize */
    a->array = malloc(initialSize * sizeof(indexbook));
    a->used = 0;
    a->size = initialSize;
}

void insertArray_add(Array_add *a, indexbook element)
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

void freeArray_add(Array_add *a)
{
    /* free memory allocated for array */
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

void deleteArray_add(Array_add *a, int pos)
{
    if (pos < 0 || pos >= (int)a->used)
        return;

    /* desplazamos todos los elementos hacia la izquierda */
    memmove(&a->array[pos],
            &a->array[pos + 1],
            (a->used - pos - 1) * sizeof(indexbook));

    a->used--;
}

void deleteArray_del(Array_del *a, int pos)
{
    if (pos < 0 || pos >= (int)a->used)
        return;

    /* desplazamos todos los elementos hacia la izquierda */
    memmove(&a->array[pos],
            &a->array[pos + 1],
            (a->used - pos - 1) * sizeof(indexdeletedbook));

    a->used--;
}

/*FUNCIONES PARA MANEJAR EL ARRAY DE BORRADOS*/
void initArray_del(Array_del *a, size_t initialSize)
{
    /* create initial empty array of size initialSize */
    a->array = malloc(initialSize * sizeof(indexdeletedbook));
    a->used = 0;
    a->size = initialSize;
}

void freeArray_del(Array_del *a)
{
    /* free memory allocated for array */
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

void insertArray_del_ff(Array_del *a, indexdeletedbook element)
{
    /* insert item "element" in array
       a->used is the number of used entries,
       a->size is the number of entries */
    size_t pos = 0;

    if (a->used == a->size)
    {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(indexdeletedbook));
    }
    while (pos < a->used && a->array[pos].register_size >= element.register_size)
        pos++;

    /* Hago hueco con memmove*/
    memmove(&a->array[pos + 1], &a->array[pos], (a->used - pos) * sizeof(indexdeletedbook));

    /*inserto */
    a->array[pos] = element;
    a->used++;
}

void insertArray_del_bf(Array_del *a, indexdeletedbook element)
{
    /* insert item "element" in array
       a->used is the number of used entries,
       a->size is the number of entries */
    size_t pos = 0;

    if (a->used == a->size)
    {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(indexdeletedbook));
    }
    while (pos < a->used && a->array[pos].register_size <= element.register_size)
        pos++;

    /* Hago hueco con memmove*/
    memmove(&a->array[pos + 1], &a->array[pos], (a->used - pos) * sizeof(indexdeletedbook));

    /*inserto */
    a->array[pos] = element;
    a->used++;
}

void insertArray_del_wf(Array_del *a, indexdeletedbook element)
{
    /* insert item "element" in array
       a->used is the number of used entries,
       a->size is the number of entries */
    size_t pos = 0;

    if (a->used == a->size)
    {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(indexdeletedbook));
    }
    while (pos < a->used && a->array[pos].register_size >= element.register_size)
        pos++;

    /* Hago hueco con memmove*/
    memmove(&a->array[pos + 1], &a->array[pos], (a->used - pos) * sizeof(indexdeletedbook));

    /*inserto */
    a->array[pos] = element;
    a->used++;
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

size_t add_best_fit(int size, Array_del *array_del, FILE *db)
{
    size_t offset = 0, size_aux = 0;
    int i = 0;
    indexdeletedbook index_del_aux;

    if (size <= 0 || !array_del || !array_del->array || !db)
    {
        return -1;
    }

    if (array_del->used <= 0)
    {
        fseek(db, 0, SEEK_END);     /* Muevo puntero al final del archivo*/
        offset = (size_t)ftell(db); /*Saco la posición donde estoy*/
        return offset;
    }

    while (i < (int)array_del->used && (array_del->array[i].register_size < (size_t)size))
    {
        i++;
    }
    if (i >= (int)array_del->used)
    {
        fseek(db, 0, SEEK_END);     /* Muevo puntero al final del archivo*/
        offset = (size_t)ftell(db); /*Saco la posición donde estoy*/
    }
    else
    {
        offset = array_del->array[i].offset;
        size_aux = array_del->array[i].register_size - size;

        if ((size_aux) > SIZE_MIN)
        {
            index_del_aux.offset = offset + size;
            index_del_aux.register_size = size_aux;
            deleteArray_del(array_del, i);
            insertArray_del_bf(array_del, index_del_aux);
        }
        else
        {
            deleteArray_del(array_del, i);
        }
    }
    return offset;
}

size_t add_worst_fit(int size, Array_del *array_del, FILE *db)
{
    size_t aux_offset = 0, size_aux;
    indexdeletedbook nuevo_index_del;
    if (size <= 0 || !array_del || !array_del->array)
    {
        return -1;
    }

    if (array_del->used <= 0)
    {
        fseek(db, 0, SEEK_END);         /* Muevo puntero al final del archivo*/
        aux_offset = (size_t)ftell(db); /*Saco la posición donde estoy*/
        return aux_offset;
    }

    /*cogemos el offset del primero en la lista de borrados*/
    if (array_del->array[0].register_size >= (size_t)size)
    {
        aux_offset = array_del->array[0].offset;
        size_aux = array_del->array[0].register_size - size;

        if (size_aux > SIZE_MIN)
        {
            nuevo_index_del.offset = aux_offset + size;
            nuevo_index_del.register_size = size_aux;
            deleteArray_del(array_del, 0);
            insertArray_del_wf(array_del, nuevo_index_del);
        }
        else
        {
            deleteArray_del(array_del, 0);
        }
    }
    /*nos vamos al final de la lista de borrados*/
    else
    {
        fseek(db, 0, SEEK_END);         /* Muevo puntero al final del archivo*/
        aux_offset = (size_t)ftell(db); /*Saco la posición donde estoy*/
    }
    return aux_offset;
}

size_t add_first_fit(int size, Array_del *array_del, FILE *db)
{
    size_t aux_offset = 0, size_aux;
    int i = 0;
    indexdeletedbook nuevo_index_del;
    if (size <= 0 || !array_del || !array_del->array)
    {
        return -1;
    }

    if (array_del->used <= 0)
    {
        fseek(db, 0, SEEK_END);         /* Muevo puntero al final del archivo*/
        aux_offset = (size_t)ftell(db); /*Saco la posición donde estoy*/
        return aux_offset;
    }

    while (i < (int)array_del->used && (array_del->array[i].register_size < (size_t)size))
    {
        i++;
    }
    if (i >= (int)array_del->used)
    {
        fseek(db, 0, SEEK_END);         /* Muevo puntero al final del archivo*/
        aux_offset = (size_t)ftell(db); /*Saco la posición donde estoy*/
    }
    else
    {
        aux_offset = array_del->array[i].offset;
        size_aux = array_del->array[i].register_size - size;

        if (size_aux > SIZE_MIN)
        {
            nuevo_index_del.offset = aux_offset + size;
            nuevo_index_del.register_size = size_aux;
            deleteArray_del(array_del, i);
            insertArray_del_ff(array_del, nuevo_index_del);
        }
        else
        {
            deleteArray_del(array_del, i);
        }
    }

    return aux_offset;
}

void load_ind_to_array_add(Array_add *a, FILE *binario)
{
    int i, size, size_elemento, num_inds;
    indexbook ind_aux;

    if (a == NULL || binario == NULL)
    {
        return;
    }

    fseek(binario, 0, SEEK_END);
    size = ftell(binario);
    size_elemento = sizeof(int) + sizeof(long int) + sizeof(size_t);
    num_inds = size / size_elemento;

    fseek(binario, 0, SEEK_SET);

    for (i = 0; i < num_inds; i++)
    {
        fread(&ind_aux.key, sizeof(int), 1, binario);
        fread(&ind_aux.offset, sizeof(long int), 1, binario);
        fread(&ind_aux.size, sizeof(size_t), 1, binario);

        insertArray_add(a, ind_aux);
    }
}

void load_ind_to_array_del(Array_del *a, FILE *binario, int strat)
{
    int i, size, size_elemento, num_inds;
    indexdeletedbook ind_aux;

    if (a == NULL || binario == NULL || strat == NO_STRAT)
    {
        return;
    }

    fseek(binario, 0, SEEK_END);
    size = ftell(binario);

    if (size < sizeof(int))
        return;

    size_elemento = sizeof(size_t) + sizeof(size_t);
    num_inds = (size - sizeof(int)) / size_elemento;

    /* Nos saltamos el primer entero que es la estrategia */
    fseek(binario, sizeof(int), SEEK_SET);

    for (i = 0; i < num_inds; i++)
    {
        fread(&ind_aux.offset, sizeof(size_t), 1, binario);
        fread(&ind_aux.register_size, sizeof(size_t), 1, binario);

        if (strat == FIRSTFIT)
        {
            insertArray_del_ff(a, ind_aux);
        }
        else if (strat == BESTFIT)
        {
            insertArray_del_bf(a, ind_aux);
        }
        else if (strat == WORSTFIT)
        {
            insertArray_del_wf(a, ind_aux);
        }
    }
}

int main(int argc, char *argv[])
{
    int strat = NO_STRAT;
    char *raiz = NULL;
    char aux[LIB_SIZE];
    char datos[256], indice[256], lista[256];
    char *args = NULL;
    char *token = NULL;
    char buffer[256];
    long int offset;
    FILE *db = NULL;
    FILE *ind = NULL;
    FILE *lst = NULL;
    size_t size = 0;
    size_t l1, l2;
    book book_aux;
    indexbook idx;
    Array_add indices;
    int i = 0;
    int aux_id;
    int posicion;
    indexbook aux_ind;
    indexdeletedbook aux_deleted;
    Array_del array_del;

    if (argc != 3)
    {
        printf("Missing argument\n");
        printf("Uso: %s <estrategia> <nombre_raiz>\n", argv[0]);
        printf("Estrategias posibles: best fit, first fit, worst fit\n");
        return 0;
    }

    char *estrategia = argv[1];
    /*Vemos qque tipo de estrategia se utiliza*/
    if (!strcmp(estrategia, "best_fit"))
    {
        strat = BESTFIT;
    }
    else if (!strcmp(estrategia, "first_fit"))
    {
        strat = FIRSTFIT;
    }
    else if (!strcmp(estrategia, "worst_fit"))
    {
        strat = WORSTFIT;
    }

    else if (strat == NO_STRAT)
    {
        printf("Unknown search strategy unknown_search_strategy\n");
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
        perror("Error al crear el fichero de datos\n");
        return 0;
    }

    ind = fopen(indice, "r+b");
    if (ind == NULL)
    {
        ind = fopen(indice, "w+b");
        if (!ind)
        {
            perror("Error al crear el fichero de índices\n");
            return 0;
        }
    }
    lst = fopen(lista, "wb");
    if (!lst)
    {
        perror("Error al crear el fichero de datos\n");
        return 0;
    }

    /*inicializo array para indices*/
    initArray_add(&indices, 2);
    if (indices.array == NULL)
    {
        perror("Error al crear el array de índices\n");
        return 0;
    }

    load_ind_to_array_add(&indices, ind);

    /* iniciallizo array para borrados*/
    initArray_del(&array_del, 2);
    if (array_del.array == NULL)
    {
        perror("Error al crear el array de borrados\n");
        return 0;
    }

    load_ind_to_array_del(&array_del, lst, strat);

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
            /*printf("all done\n");*/
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
            book_aux.bookID = atoi(token);

            /*falta ver si ya esta insertado*/

            /*Ahora el isbn*/
            token = strtok(NULL, "|");
            strncpy(book_aux.isbn, token, ISBNSIZE);

            /*Ahora el titulo con el separador |*/
            token = strtok(NULL, "|");
            if (!(book_aux.title = malloc(strlen(token) + 2)))
            {
                fprintf(stderr, "Error reservando memoria para title\n");
                return -1;
            }
            sprintf(book_aux.title, "%s|", token);

            /*Por ultimo el autor*/
            token = strtok(NULL, "|");
            if (!(book_aux.printedBy = malloc(strlen(token) + 1)))
            {
                fprintf(stderr, "Error reservando memoria para title\n");
                return -1;
            }
            sprintf(book_aux.printedBy, "%s", token);

            /*Calculamos lo que va a ocupar*/
            l1 = strlen(book_aux.title);
            l2 = strlen(book_aux.printedBy);
            size = sizeof(int) + sizeof(char) * ISBNSIZE + sizeof(char) * l1 + sizeof(char) * l2;

            /* escribir entrada la lista de indices */

            if (strat == FIRSTFIT)
            {
                offset = add_first_fit(size, &array_del, db);
            }
            else if (strat == BESTFIT)
            {
                offset = add_best_fit(size, &array_del, db);
            }
            else
            {
                offset = add_worst_fit(size, &array_del, db);
            }

            if (offset == -1)
            {
                fprintf(stderr, "Error añadiendo un nuevo dato\n");
                return -1;
            }
            fseek(db, offset, SEEK_SET);

            /* escribir registro en .db */
            fwrite(&size, sizeof(size_t), 1, db);              /*Primero lo que ocupa*/
            fwrite(&book_aux.bookID, sizeof(int), 1, db);      /*Ahora escribimos el ID*/
            fwrite(book_aux.isbn, sizeof(char), ISBNSIZE, db); /*El isbn*/
            fwrite(book_aux.title, sizeof(char), l1, db);      /*El titulo*/
            fwrite(book_aux.printedBy, sizeof(char), l2, db);  /*El autor*/

            idx.key = book_aux.bookID;
            idx.offset = offset;
            idx.size = size;

            insertArray_add(&indices, idx);

            printf("Record with BookID=%d has been added to the database\n", book_aux.bookID);

            free(book_aux.title);
            free(book_aux.printedBy);

            continue;
        }

        /*Delete*/
        if (strncmp(aux, "del ", 4) == 0)
        {
            args = aux + 4;

            token = strtok(args, "\n");
            if (!token)
                continue;
            aux_id = atoi(token);

            /*comprobamos que existe el dato en array de indices para asi porder borrarlo*/
            posicion = bin_search(&indices, aux_id);
            if (posicion == -1)
            {
                printf("Item with key %i does not exist\n", aux_id);
            }

            /*eleminamos el elemento de array de indices*/
            aux_ind = indices.array[posicion];
            deleteArray_add(&indices, posicion);

            /*Conseguimos los datos del elemento indexdeletedbook que queremos anadir al array de borrados */
            aux_deleted.offset = aux_ind.offset;
            aux_deleted.register_size = aux_ind.size;
            if (strat == FIRSTFIT)
            {
                insertArray_del_ff(&array_del, aux_deleted);
            }
            else if (strat == BESTFIT)
            {
                insertArray_del_bf(&array_del, aux_deleted);
            }
            else
            {
                insertArray_del_wf(&array_del, aux_deleted);
            }
            printf("Record with BookID=%i has been deleted\n", aux_ind.key);
            continue;
        }

        /*comando find buscar elemento en el archivo de datos binario y sacar todos sus elementos*/
        if (strncmp(aux, "find ", 5) == 0)
        {
            args = aux + 5;

            token = strtok(args, "\n");
            if (!token)
                continue;
            aux_id = atoi(token);

            posicion = bin_search(&indices, aux_id);
            if (posicion == -1)
            {
                printf("Record with bookId=%i does not exist\n", aux_id);
            }
            aux_ind = indices.array[posicion];
            /*castameos a lo que queremos que se convierta el tipo de dato : ej 3A 30 00 00 lo casteamos en int y sacamos el entero 12345*/
            fseek(db, aux_ind.offset + (int)sizeof(size_t), SEEK_SET);
            fread(&book_aux.bookID, (int)sizeof(int), 1, db);
            fread(book_aux.isbn, ISBNSIZE, 1, db);
            /*como es tamano variable cogemos el resto del registro y luego separamos gracias a '|' */
            fread(buffer, aux_ind.size - (int)sizeof(int) - ISBNSIZE, 1, db);
            buffer[aux_ind.size - (int)sizeof(int) - ISBNSIZE] = '\0';
            token = strtok(buffer, "|");
            strcpy(book_aux.title, token);
            token = strtok(NULL, "\0");
            strcpy(book_aux.printedBy, token);

            printf("%i|%s|%s|%s\n", book_aux.bookID, book_aux.isbn, book_aux.printedBy, book_aux.title);
            continue;
        }

        /* comando printInd */
        if (strcmp(aux, "printInd") == 0)
        {

            for (i = 0; i < (int)indices.used; i++)
            {
                printf("Entry #%d\n", i);
                printf("    key: #%d\n", indices.array[i].key);
                printf("    offset: #%ld\n", indices.array[i].offset);
                printf("    size: #%ld\n", indices.array[i].size);
            }
            continue;
        }

        /* comando printLst */
        if (strcmp(aux, "printLst") == 0)
        {
            for (i = 0; i < (int)array_del.used; i++)
            {
                printf("Entry #%d\n", i);
                printf("    offset: #%ld\n", array_del.array[i].offset);
                printf("    size: #%ld\n", array_del.array[i].register_size);
            }
            continue;
        }

        /* comando printRec*/
        if (strcmp(aux, "printRec") == 0)
        {
            for (i = 0; i < (int)indices.used; i++)
            {
                aux_ind = indices.array[i];
                /*castameos a lo que queremos que se convierta el tipo de dato : ej 3A 30 00 00 lo casteamos en int y sacamos el entero 12345*/
                fseek(db, aux_ind.offset + (int)sizeof(size_t), SEEK_SET);
                fread(&book_aux.bookID, (int)sizeof(int), 1, db);
                fread(book_aux.isbn, ISBNSIZE, 1, db);
                /*como es tamano variable cogemos el resto del registro y luego separamos gracias a '|' */
                fread(buffer, aux_ind.size - (int)sizeof(int) - ISBNSIZE, 1, db);
                buffer[aux_ind.size - (int)sizeof(int) - ISBNSIZE] = '\0';
                token = strtok(buffer, "|");
                strcpy(book_aux.title, token);
                token = strtok(NULL, "\0");
                strcpy(book_aux.printedBy, token);

                printf("%i|%s|%s|%s\n", book_aux.bookID, book_aux.isbn, book_aux.printedBy, book_aux.title);
            }
            continue;
        }
        printf("Unknown command.\n");
    }

    for (i = 0; i < (int)indices.used; i++)
    {
        fwrite(&indices.array[i].key, sizeof(int), 1, ind);
        fwrite(&indices.array[i].offset, sizeof(long int), 1, ind);
        fwrite(&indices.array[i].size, sizeof(size_t), 1, ind);
    }

    fwrite(&strat, sizeof(int), 1, lst);
    for (i = 0; i < (int)array_del.used; i++)
    {
        fwrite(&array_del.array[i].offset, sizeof(size_t), 1, lst);
        fwrite(&array_del.array[i].register_size, sizeof(size_t), 1, lst);
    }

    fclose(db);
    fclose(ind);
    fclose(lst);
    freeArray_add(&indices);
    freeArray_del(&array_del);

    return 0;
}
