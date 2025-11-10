#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"
#include "search.h"

void results_search(char *from, char *to, char *date,
                    int *n_choices, char ***choices,
                    int max_length,
                    int max_rows)
/* 
 * @param from form field from
 * @param to form field to
 * @param n_choices fill this with the number of results
 * @param choices fill this with the actual results
 * @param max_length output win maximum width
 * @param max_rows output win maximum number of rows
 */
{
    int i;
    *n_choices = 0;

    /* Variables para conexión y manejo ODBC */
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;

    /* Buffers para columnas del resultado */
    SQLINTEGER flight_id1;
    SQLINTEGER flight_id2;
    SQLCHAR first_departure[30];
    SQLCHAR last_arrival[30];
    SQLINTEGER num_vuelos;
    SQLINTEGER asientos_libres;

    /* Indicadores de NULL para cada columna */
    SQLLEN ind_first, ind_last, ind_fl1, ind_fl2, ind_num, ind_asientos;
    SQLLEN ind_from = SQL_NTS, ind_to = SQL_NTS, ind_date = SQL_NTS;

    /* Conexión a la base de datos */
    ret = odbc_connect(&env, &dbc);
    if (!SQL_SUCCEEDED(ret))
    {
        fprintf(stderr, "Error conectando a la base de datos.\n");
        return;
    }
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    /* Consulta SQL: vuelos directos y con transbordo */
    const char *query =
        "SELECT flight_id_1, flight_id_2, first_departure, last_arrival, num_vuelos, asientos_libres "
        "FROM ("
        "  /* Subconsulta vuelos directos */"
        "  SELECT "
        "    f.flight_id AS flight_id_1,"
        "    NULL AS flight_id_2,"
        "    f.departure_airport,"
        "    f.arrival_airport,"
        "    f.scheduled_departure AS first_departure,"
        "    f.scheduled_arrival AS last_arrival,"
        "    1 AS num_vuelos,"
        "    ("
        "      /* Contar asientos libres */"
        "      SELECT COUNT(s.seat_no) - COUNT(bp.seat_no) "
        "      FROM seats s "
        "      LEFT JOIN boarding_passes bp "
        "        ON bp.seat_no = s.seat_no AND bp.flight_id = f.flight_id "
        "      WHERE s.aircraft_code = f.aircraft_code"
        "    ) AS asientos_libres "
        "  FROM flights f"
        ""
        "  UNION ALL "
        ""
        "  /* Subconsulta vuelos con transbordo */"
        "  SELECT "
        "    f1.flight_id AS flight_id_1,"
        "    f2.flight_id AS flight_id_2,"
        "    f1.departure_airport,"
        "    f2.arrival_airport,"
        "    f1.scheduled_departure AS first_departure,"
        "    f2.scheduled_arrival AS last_arrival,"
        "    2 AS num_vuelos,"
        "    LEAST("
        "      /* Asientos libres del primer vuelo */"
        "      ("
        "        SELECT COUNT(s1.seat_no) - COUNT(bp1.seat_no) "
        "        FROM seats s1 "
        "        LEFT JOIN boarding_passes bp1 "
        "          ON bp1.seat_no = s1.seat_no AND bp1.flight_id = f1.flight_id "
        "        WHERE s1.aircraft_code = f1.aircraft_code"
        "      ),"
        "      /* Asientos libres del segundo vuelo */"
        "      ("
        "        SELECT COUNT(s2.seat_no) - COUNT(bp2.seat_no) "
        "        FROM seats s2 "
        "        LEFT JOIN boarding_passes bp2 "
        "          ON bp2.seat_no = s2.seat_no AND bp2.flight_id = f2.flight_id "
        "        WHERE s2.aircraft_code = f2.aircraft_code"
        "      )"
        "    ) AS asientos_libres "
        "  FROM flights f1 "
        "  JOIN flights f2 "
        "    ON f1.arrival_airport = f2.departure_airport "
        "  WHERE f2.scheduled_departure > f1.scheduled_arrival "
        "    AND (f2.scheduled_arrival - f1.scheduled_departure) <= INTERVAL '24 hours'"
        ") AS vuelos "
        "WHERE departure_airport = ? "
        "  AND arrival_airport = ? "
        "  AND DATE(first_departure) = ? "
        "  AND asientos_libres > 0 "
        "ORDER BY last_arrival - first_departure ASC;";

    /* Preparar la consulta */
    ret = SQLPrepare(stmt, (SQLCHAR *)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret))
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt); 
        odbc_disconnect(env, dbc);            
        return;
    }

    /* Vincular parámetros de entrada */
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, from, 0, &ind_from);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, to, 0, &ind_to);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_DATE, 10, 0, date, 0, &ind_date);

    /* Vincular columnas de salida a variables C */
    SQLBindCol(stmt, 1, SQL_C_LONG, &flight_id1, sizeof(flight_id1), &ind_fl1);
    SQLBindCol(stmt, 2, SQL_C_LONG, &flight_id2, sizeof(flight_id2), &ind_fl2);
    SQLBindCol(stmt, 3, SQL_C_CHAR, first_departure, sizeof(first_departure), &ind_first);
    SQLBindCol(stmt, 4, SQL_C_CHAR, last_arrival, sizeof(last_arrival), &ind_last);
    SQLBindCol(stmt, 5, SQL_C_LONG, &num_vuelos, sizeof(num_vuelos), &ind_num);
    SQLBindCol(stmt, 6, SQL_C_LONG, &asientos_libres, sizeof(asientos_libres), &ind_asientos);

    /* Ejecutar la consulta */
    ret = SQLExecute(stmt);
    if (!SQL_SUCCEEDED(ret))
    {
        fprintf(stderr, "Error ejecutando la consulta.\n");
        SQLCloseCursor(stmt);              
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        odbc_disconnect(env, dbc);           
        return;
    }

    /* Formatear cada línea con salida: salida | llegada | num vuelos | plazas libres */
    i = 0;
    while (SQL_SUCCEEDED(ret = SQLFetch(stmt)) && i < max_rows)
    {
        snprintf((*choices)[i], max_length,
                 "Salida: %s | Llegada: %s | Vuelos: %d | Plazas: %d",
                 (ind_first == SQL_NULL_DATA) ? "N/A" : (char *)first_departure,
                 (ind_last == SQL_NULL_DATA) ? "N/A" : (char *)last_arrival,
                 (ind_num == SQL_NULL_DATA) ? 0 : (int)num_vuelos,
                 (ind_asientos == SQL_NULL_DATA) ? 0 : (int)asientos_libres);
        i++;
    }
    if (i == 0)
    {
        if (max_rows > 0) 
        {
            snprintf((*choices)[0], max_length, "No se encontraron vuelos para los criterios seleccionados.");
            *n_choices = 1;
        }
        else
        {
            *n_choices = 0;
        }
    }
    else
    {
        *n_choices = i;
    }
    
    /* Liberar recursos y desconectar */
    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    odbc_disconnect(env, dbc);
}