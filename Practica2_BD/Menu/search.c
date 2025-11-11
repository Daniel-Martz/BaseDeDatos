#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"
#include "search.h"

/**
 * @brief Busca vuelos (directos o con una escala) según origen, destino y fecha.
 * @author Rodrigo Diaz-Reganon
 * 
 * @param from          Codigo del aeropuerto de origen
 * @param to            Código del aeropuerto de destino
 * @param date          Fecha de salida en formato YYYY-MM-DD.
 * @param n_choices     Puntero para almacenar el número de resultados.
 * @param choices       Array de cadenas para almacenar los resultados.
 * @param max_length    Longitud máxima de cada cadena en 'choices'.
 * @param max_rows      Número máximo de filas a almacenar en 'choices'.
 */
void results_search(char *from, char *to, char *date,
                    int *n_choices, char ***choices,
                    int max_length,
                    int max_rows)
{
    int i;
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
    SQLCHAR aircraft_code_1[10];
    SQLCHAR aircraft_code_2[10];

    SQLLEN ind_first, ind_last, ind_fl1, ind_fl2, ind_num, ind_asientos;
    SQLLEN ind_ac1, ind_ac2;
    SQLLEN ind_from = SQL_NTS, ind_to = SQL_NTS;
    SQLLEN date_len;

    /* Query SQL principal */
    const char *query =
        /* Seleccionamos todos los campos necesarios, incluidos los de la escala */
        "SELECT flight_id_1, flight_id_2, aircraft_code_1, aircraft_code_2, first_departure, last_arrival, num_vuelos, asientos_libres "
        "FROM ("
        /* Bloque 1: Vuelos directos */
        "  SELECT "
        "    f.flight_id AS flight_id_1,"
        "    NULL AS flight_id_2,"
        "    f.aircraft_code AS aircraft_code_1, "
        "    NULL AS aircraft_code_2, "
        "    f.departure_airport,"
        "    f.arrival_airport,"
        "    f.scheduled_departure AS first_departure,"
        "    f.scheduled_arrival AS last_arrival,"
        "    1 AS num_vuelos,"
        "    ("
        "      SELECT COUNT(s.seat_no) - COUNT(bp.seat_no) "
        "      FROM seats s "
        "      LEFT JOIN boarding_passes bp "
        "        ON bp.seat_no = s.seat_no AND bp.flight_id = f.flight_id "
        "      WHERE s.aircraft_code = f.aircraft_code"
        "    ) AS asientos_libres "
        "  FROM flights f "
        "  UNION ALL "
        /* Bloque 2: Vuelos con escala*/
        "  SELECT "
        "    f1.flight_id AS flight_id_1,"
        "    f2.flight_id AS flight_id_2,"
        "    f1.aircraft_code AS aircraft_code_1, "
        "    f2.aircraft_code AS aircraft_code_2, "
        "    f1.departure_airport,"
        "    f2.arrival_airport,"
        "    f1.scheduled_departure AS first_departure,"
        "    f2.scheduled_arrival AS last_arrival,"
        "    2 AS num_vuelos,"
        "    LEAST("
        "      ("
        "        SELECT COUNT(s1.seat_no) - COUNT(bp1.seat_no) "
        "        FROM seats s1 "
        "        LEFT JOIN boarding_passes bp1 "
        "          ON bp1.seat_no = s1.seat_no AND bp1.flight_id = f1.flight_id "
        "        WHERE s1.aircraft_code = f1.aircraft_code"
        "      ),"
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

    /* Inicialización del contador de resultados */
    *n_choices = 0;

    if (from == NULL || to == NULL || date == NULL ||
        strlen(from) == 0 || strlen(to) == 0 || strlen(date) == 0)
    {
        snprintf((*choices)[0], max_length, "Error no hay datos con estos valores.");
        *n_choices = 1;
        return;
    }

    /* Conexino a la base de datos */
    ret = odbc_connect(&env, &dbc);
    if (!SQL_SUCCEEDED(ret))
    {
        fprintf(stderr, "Error conectando a la base de datos.\n");
        snprintf((*choices)[0], max_length, "Error: no se pudo conectar a la base de datos.");
        *n_choices = 1;
        return;
    }

    /* Reservar un handle para el statement */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    /*Preparamos la consulta*/
    ret = SQLPrepare(stmt, (SQLCHAR *)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret))
    {
        snprintf((*choices)[0], max_length, "Error preparando la consulta SQL.");
        *n_choices = 1;
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        odbc_disconnect(env, dbc);
        return;
    }

    date_len = (SQLLEN)strlen(date);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, from, 0, &ind_from);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, to, 0, &ind_to);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 10, 0, date, 0, &date_len);

    /*Vinculamos la columna*/
    SQLBindCol(stmt, 1, SQL_C_LONG, &flight_id1, sizeof(flight_id1), &ind_fl1);
    SQLBindCol(stmt, 2, SQL_C_LONG, &flight_id2, sizeof(flight_id2), &ind_fl2);
    SQLBindCol(stmt, 3, SQL_C_CHAR, aircraft_code_1, sizeof(aircraft_code_1), &ind_ac1);
    SQLBindCol(stmt, 4, SQL_C_CHAR, aircraft_code_2, sizeof(aircraft_code_2), &ind_ac2);
    SQLBindCol(stmt, 5, SQL_C_CHAR, first_departure, sizeof(first_departure), &ind_first);
    SQLBindCol(stmt, 6, SQL_C_CHAR, last_arrival, sizeof(last_arrival), &ind_last);
    SQLBindCol(stmt, 7, SQL_C_LONG, &num_vuelos, sizeof(num_vuelos), &ind_num);
    SQLBindCol(stmt, 8, SQL_C_LONG, &asientos_libres, sizeof(asientos_libres), &ind_asientos);

    ret = SQLExecute(stmt);
    if (!SQL_SUCCEEDED(ret))
    {
        snprintf((*choices)[0], max_length, "Error ejecutando la consulta SQL.");
        *n_choices = 1;
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        odbc_disconnect(env, dbc);
        return;
    }

    /* Recorrer todas las filas devueltas */
    i = 0;
    while (SQL_SUCCEEDED(ret = SQLFetch(stmt)) && i < max_rows)
    {
        snprintf((*choices)[i], max_length,
                 "Salida: %-19.19s | Llegada: %-19.19s | Vuelos: %d | Plazas: %4d"
                 " | INFO: %ld %s %ld %s",
                 (ind_first == SQL_NULL_DATA) ? "N/A" : (char *)first_departure,
                 (ind_last == SQL_NULL_DATA) ? "N/A" : (char *)last_arrival,
                 (ind_num == SQL_NULL_DATA) ? 0 : (int)num_vuelos,
                 (ind_asientos == SQL_NULL_DATA) ? 0 : (int)asientos_libres,
                 (ind_fl1 == SQL_NULL_DATA) ? -1 : (long)flight_id1,
                 (ind_ac1 == SQL_NULL_DATA) ? "N/A" : (char *)aircraft_code_1,
                 (ind_fl2 == SQL_NULL_DATA) ? -1 : (long)flight_id2,
                 (ind_ac2 == SQL_NULL_DATA) ? "N/A" : (char *)aircraft_code_2);
        i++;
    }

    /* Comprobar si se encontraron resultados */
    if (i == 0)
    {
        snprintf((*choices)[0], max_length, "Error no hay datos con estos valores (sin resultados).");
        *n_choices = 1;
    }
    else
    {
        *n_choices = i;
    }

    /* Liberar handles y desconectar */
    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    odbc_disconnect(env, dbc);
}