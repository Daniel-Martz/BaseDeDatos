#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"

/*
 * example 3 with a queries build on-the-fly, the bad way
 */

int main(void) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret; /* ODBC API return status */
    char from[512];
    char to[512];
    char date[512];
    char query[2200];
    SQLCHAR y[8000];

    /* CONNECT */
    ret = odbc_connect(&env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        return EXIT_FAILURE;
    }

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    printf("from = ");
    fflush(stdout);
    while (fgets(from, sizeof(from), stdin) != NULL) {
        from[strcspn(from, "\n")] = 0;


        printf("to = ");
        fflush(stdout);
        if(fgets(to, sizeof(to), stdin) == NULL)    {
            break;
        }
        to[strcspn(to, "\n")] = 0;

        printf("date = ");
        fflush(stdout);
        if(fgets(date, sizeof(date), stdin) == NULL)    {
            break;
        }
        date[strcspn(date, "\n")] = 0;

        sprintf(query, "WITH tabla AS ( \
SELECT \
    f.departure_airport, \
    f.arrival_airport, \
    tf.ticket_no, \
    MIN(f.scheduled_departure) AS first_departure, \
    MAX(f.scheduled_arrival) AS last_arrival, \
    COUNT(tf.flight_id) AS num_vuelos, \
    MIN(a.free_seats) AS asientos_libres \
FROM ticket_flights AS tf \
JOIN flights AS f ON tf.flight_id = f.flight_id \
JOIN ( \
    SELECT s.flight_id, COUNT(s.seat_no) - COUNT(bp.seat_no) AS free_seats \
    FROM seats AS s \
    LEFT JOIN boarding_passes AS bp \
        ON bp.flight_id = s.flight_id AND bp.seat_no = s.seat_no \
    GROUP BY s.flight_id \
) AS a ON a.flight_id = f.flight_id \
GROUP BY tf.ticket_no, f.departure_airport, f.arrival_airport \
HAVING COUNT(tf.flight_id) < 3 \
   AND MIN(a.free_seats) > 0 \
   AND (MAX(f.scheduled_arrival) - MIN(f.scheduled_departure)) <= INTERVAL '24 hour' \
) \
SELECT * \
FROM tabla \
WHERE departure_airport = '%s' \
  AND arrival_airport = '%s' \
  AND DATE(first_departure) = '%s'", from, to, date);


        printf ("%s\n", query); /*ojo error de syntax*/

        SQLExecDirect(stmt, (SQLCHAR*) query, SQL_NTS);

        SQLBindCol(stmt, 1, SQL_C_CHAR, y, sizeof(y), NULL);

        /* Loop through the rows in the result-set */
        while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
            printf("y = %s\n", y);
        }

        SQLCloseCursor(stmt); /*OJO:limpio la variable stmt*/

        printf("from = ");
        fflush(stdout);
    }
    printf("\n");
    
    /* free up statement handle */
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    /* DISCONNECT */
    ret = odbc_disconnect(env, dbc);
    if (!SQL_SUCCEEDED(ret)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


