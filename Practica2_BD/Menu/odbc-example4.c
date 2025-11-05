#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"

/*
 * example 5: dynamic query with parameters using scanf
 */

int main(void) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;
    SQLCHAR from[512];
    SQLCHAR to[512];
    SQLCHAR date[512];
    SQLCHAR y[8000];

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
        " JOIN (SELECT s.flight_id, COUNT(s.seat_no) - COUNT(bp.seat_no) AS free_seats"
        "       FROM seats AS s"
        "       LEFT JOIN boarding_passes AS bp"
        "       ON bp.flight_id = s.flight_id AND bp.seat_no = s.seat_no"
        "       GROUP BY s.flight_id) AS a"
        " ON a.flight_id = f.flight_id"
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

    while (1) {
        /* Leer entradas del usuario */
        printf("from: "); fflush(stdout);
        if (scanf("%511s", from) != 1) break;

        printf("to: "); fflush(stdout);
        if (scanf("%511s", to) != 1) break;

        printf("date (YYYY-MM-DD): "); fflush(stdout);
        if (scanf("%511s", date) != 1) break;

        /* Asociar parámetros correctamente (cadenas) */
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, from, 0, NULL);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, to, 0, NULL);
        SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, date, 0, NULL);

        /* Ejecutar la consulta */
        SQLExecute(stmt);

        /* Asociar columna de resultados */
        SQLBindCol(stmt, 1, SQL_C_CHAR, y, sizeof(y), NULL);

        /* Leer resultados */
        while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
            printf("y = %s\n", y);
        }

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
