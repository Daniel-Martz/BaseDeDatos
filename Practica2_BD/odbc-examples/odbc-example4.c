#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"

/*
 * example 5: consulta dinámica con parámetros
 * (Versión corregida)
 */

int main(void) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;
    
    /* === BÚFERES PARA PARÁMETROS DE ENTRADA === */
    SQLCHAR from[512];
    SQLCHAR to[512];
    SQLCHAR date[512];
    
    /* === BÚFERES PARA LAS 7 COLUMNAS DE RESULTADO === */
    SQLCHAR dep_airport[4];     // e.g., 'SVO' + null
    SQLCHAR arr_airport[4];
    SQLCHAR ticket_no[14];
    SQLCHAR first_departure[30];  // Los Timestamps se leen mejor como strings
    SQLCHAR last_arrival[30];
    SQLINTEGER num_vuelos;
    SQLINTEGER asientos_libres;

    /* === VARIABLES INDICADORAS PARA COLUMNAS DE RESULTADO === */
    // Estas guardan la longitud del string o si el valor es SQL_NULL_DATA
    SQLLEN ind_dep, ind_arr, ind_ticket, ind_first, ind_last, ind_num, ind_asientos;

    /* === VARIABLES INDICADORAS PARA PARÁMETROS DE ENTRADA === */
    // SQL_NTS (Null Terminated String) le dice al driver que calcule
    // la longitud de la cadena usando strlen()
    SQLLEN ind_from = SQL_NTS;
    SQLLEN ind_to   = SQL_NTS;
    SQLLEN ind_date = SQL_NTS;


    /* CONNECT */
    ret = odbc_connect(&env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        return EXIT_FAILURE;
    }

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    /* Query parametrizada */
    const char *query = 
         "WITH tabla AS ("
    " SELECT f.departure_airport, f.arrival_airport, tf.ticket_no,"
    "        MIN(f.scheduled_departure) AS first_departure,"
    "        MAX(f.scheduled_arrival) AS last_arrival,"
    "        COUNT(tf.flight_id) AS num_vuelos,"
    "        MIN(a.free_seats) AS asientos_libres"
    " FROM ticket_flights AS tf"
    " JOIN flights AS f ON tf.flight_id = f.flight_id"
    " JOIN ("
    "       SELECT fl.flight_id, COUNT(s.seat_no) - COUNT(bp.seat_no) AS free_seats"
    "       FROM flights AS fl"
    "       JOIN seats AS s ON fl.aircraft_code = s.aircraft_code"
    "       LEFT JOIN boarding_passes AS bp"
    "         ON bp.flight_id = fl.flight_id AND bp.seat_no = s.seat_no"
    "       GROUP BY fl.flight_id"
    "      ) AS a ON a.flight_id = f.flight_id"
    " GROUP BY tf.ticket_no, f.departure_airport, f.arrival_airport"
    " HAVING COUNT(tf.flight_id) < 3"
    "    AND MIN(a.free_seats) > 0"
    "    AND (MAX(f.scheduled_arrival) - MIN(f.scheduled_departure)) <= INTERVAL '24 hour'"
    ")"
    " SELECT * FROM tabla"
    " WHERE departure_airport = ?"
    "   AND arrival_airport = ?"    
    "   AND DATE(first_departure) = ?";

    SQLPrepare(stmt, (SQLCHAR*)query, SQL_NTS);

    /* === ASOCIAR (BIND) LAS 7 COLUMNAS DE RESULTADO (UNA SOLA VEZ) === */
    // Esto se hace ANTES del bucle, ya que las variables de destino no cambian.
    SQLBindCol(stmt, 1, SQL_C_CHAR,   dep_airport,     sizeof(dep_airport),     &ind_dep);
    SQLBindCol(stmt, 2, SQL_C_CHAR,   arr_airport,     sizeof(arr_airport),     &ind_arr);
    SQLBindCol(stmt, 3, SQL_C_CHAR,   ticket_no,       sizeof(ticket_no),       &ind_ticket);
    SQLBindCol(stmt, 4, SQL_C_CHAR,   first_departure, sizeof(first_departure), &ind_first);
    SQLBindCol(stmt, 5, SQL_C_CHAR,   last_arrival,    sizeof(last_arrival),    &ind_last);
    SQLBindCol(stmt, 6, SQL_C_LONG,   &num_vuelos,     sizeof(num_vuelos),      &ind_num);
    SQLBindCol(stmt, 7, SQL_C_LONG,   &asientos_libres,sizeof(asientos_libres), &ind_asientos);


    while (1) {
        /* Leer entradas del usuario */
        printf("from: "); fflush(stdout);
        if (scanf("%511s", from) != 1) break;

        printf("to: "); fflush(stdout);
        if (scanf("%511s", to) != 1) break;

        printf("date (YYYY-MM-DD): "); fflush(stdout);
        if (scanf("%511s", date) != 1) break;

        /* === ASOCIAR (BIND) PARÁMETROS CORREGIDO === */
        // El último argumento ahora es un puntero válido a una variable
        // que contiene el valor SQL_NTS.
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, from, 0, &ind_from);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, to,   0, &ind_to);
        SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_DATE,    10, 0, date, 0, &ind_date);

        /* Ejecutar la consulta */
        ret = SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
             printf("Error ejecutando la consulta.\n");
             /* * Aquí deberías añadir una función que imprima el error ODBC
              * (usando SQLGetDiagRec) para saber qué ha fallado.
              */
             SQLCloseCursor(stmt); // Limpiar cursor igualmente
             continue; // Continuar al siguiente ciclo del loop
        }


        /* Leer resultados */
        printf("--- Resultados de la Consulta ---\n");
        int rows_found = 0;
        while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
            rows_found = 1;
            /* === IMPRIMIR LAS 7 COLUMNAS === */
            // Comprobamos si el indicador es NULL antes de imprimir (buena práctica)
            printf("Fila: dep=%s, arr=%s, ticket=%s, depart=%s, arrive=%s, vuelos=%d, asientos=%d\n",
                   (ind_dep == SQL_NULL_DATA) ? "NULL" : (char*)dep_airport,
                   (ind_arr == SQL_NULL_DATA) ? "NULL" : (char*)arr_airport,
                   (ind_ticket == SQL_NULL_DATA) ? "NULL" : (char*)ticket_no,
                   (ind_first == SQL_NULL_DATA) ? "NULL" : (char*)first_departure,
                   (ind_last == SQL_NULL_DATA) ? "NULL" : (char*)last_arrival,
                   (ind_num == SQL_NULL_DATA) ? 0 : (int)num_vuelos,
                   (ind_asientos == SQL_NULL_DATA) ? 0 : (int)asientos_libres);
        }
        
        if (!rows_found) {
            printf("No se encontraron resultados.\n");
        }
        printf("--- Fin de Resultados ---\n\n");

        /* Limpiar cursor para la siguiente ejecución */
        SQLCloseCursor(stmt);
    }

    /* Free statement handle */
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    /* Disconnect */
    ret = odbc_disconnect(env, dbc);
    if (!SQL_SUCCEEDED(ret)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
