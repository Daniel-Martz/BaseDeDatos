#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"
#include "lbpass.h"

void results_bpass(char *bookID,
                   int *n_choices, char ***choices,
                   int max_length,
                   int max_rows)
{
    int i;
    *n_choices = 0;

    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt_tickets, stmt_seat, stmt_bno, stmt_insert;
    SQLRETURN ret;

    /* Flag para controlar la transacción (1 = OK, 0 = FALLO) */
    int flag_consulta1 = 1;
    

    /*Buffers para los datos*/
    SQLINTEGER flight_id;
    SQLCHAR ticket_no[14];
    SQLCHAR passenger_name[100];
    SQLCHAR aircraft_code[4];
    SQLCHAR scheduled_departure[30];
    SQLCHAR seat_no[5];
    SQLINTEGER new_boarding_no;

    /*Indicadores*/
    SQLLEN ind_bookID = SQL_NTS;
    SQLLEN ind_tk = 0, ind_fid = 0, ind_ac = 0, ind_seat = 0, ind_bno = 0;

    /* Conexión a la base de datos */
    ret = odbc_connect(&env, &dbc);
    if (!SQL_SUCCEEDED(ret))
    {
        fprintf(stderr, "Error conectando a la base de datos.\n");
        return;
    }

    /* Desactivamos autocommit para controlar la transacción manualmente */
    ret = SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    if (!SQL_SUCCEEDED(ret))
    {
        fprintf(stderr, "Error desactivando autocommit.\n");
        odbc_disconnect(env, dbc);
        return;
    }

    /* --- Preparar TODAS las consultas --- */
    const char *sql_tickets =
        "SELECT tf.flight_id, tf.ticket_no, t.passenger_name, f.aircraft_code, f.scheduled_departure "
        "FROM ticket_flights AS tf "
        "JOIN tickets AS t ON tf.ticket_no = t.ticket_no "
        "JOIN flights AS f ON tf.flight_id = f.flight_id "
        "LEFT JOIN boarding_passes AS bp ON tf.ticket_no = bp.ticket_no AND tf.flight_id = bp.flight_id "
        "WHERE t.book_ref = ? AND bp.flight_id IS NULL "
        "ORDER BY tf.ticket_no ASC";
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_tickets);
    SQLPrepare(stmt_tickets, (SQLCHAR *)sql_tickets, SQL_NTS);

    const char *sql_seat =
        "SELECT s.seat_no FROM seats AS s "
        "WHERE s.aircraft_code = ? AND s.seat_no NOT IN ( "
        "  SELECT bp.seat_no FROM boarding_passes AS bp WHERE bp.flight_id = ? "
        ") ORDER BY s.seat_no ASC LIMIT 1 "
        "FOR UPDATE SKIP LOCKED";
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_seat);
    SQLPrepare(stmt_seat, (SQLCHAR *)sql_seat, SQL_NTS);

    const char *sql_bno = "SELECT COALESCE(MAX(boarding_no), 0) + 1 FROM boarding_passes WHERE flight_id = ?";
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_bno);
    SQLPrepare(stmt_bno, (SQLCHAR *)sql_bno, SQL_NTS);

    const char *sql_insert = "INSERT INTO boarding_passes (ticket_no, flight_id, boarding_no, seat_no) VALUES (?, ?, ?, ?)";
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_insert);
    SQLPrepare(stmt_insert, (SQLCHAR *)sql_insert, SQL_NTS);

    /* --- Ejecutar Consulta 1 (Billetes) --- */
    SQLBindParameter(stmt_tickets, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 6, 0, bookID, 0, &ind_bookID);

    ret = SQLExecute(stmt_tickets);
    if (!SQL_SUCCEEDED(ret))
    {
        snprintf((*choices)[0], max_length, "ERROR: Fallo al buscar billetes para la reserva %s.", bookID);
        flag_consulta1 = 0;
    }
    
    SQLBindCol(stmt_tickets, 1, SQL_C_LONG, &flight_id, sizeof(flight_id), &ind_fid);
    SQLBindCol(stmt_tickets, 2, SQL_C_CHAR, ticket_no, sizeof(ticket_no), &ind_tk);
    SQLBindCol(stmt_tickets, 3, SQL_C_CHAR, passenger_name, sizeof(passenger_name), NULL);
    SQLBindCol(stmt_tickets, 4, SQL_C_CHAR, aircraft_code, sizeof(aircraft_code), &ind_ac);
    SQLBindCol(stmt_tickets, 5, SQL_C_CHAR, scheduled_departure, sizeof(scheduled_departure), NULL);

    i = 0;
    while (flag_consulta1 && SQL_SUCCEEDED(ret = SQLFetch(stmt_tickets)) && i < max_rows)
    {
        /* 1. Ejecución Consulta 2 (Encontrar asiento) */
        SQLBindParameter(stmt_seat, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, aircraft_code, 0, &ind_ac);
        SQLBindParameter(stmt_seat, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &flight_id, 0, &ind_fid);
        SQLBindCol(stmt_seat, 1, SQL_C_CHAR, seat_no, sizeof(seat_no), &ind_seat);
        
        ret = SQLExecute(stmt_seat);
        if (!SQL_SUCCEEDED(ret)) {
            snprintf((*choices)[0], max_length, "ERROR BD al ejecutar búsqueda de asiento");
            flag_consulta1 = 0;
            SQLCloseCursor(stmt_seat);
            break;
        }
        ret = SQLFetch(stmt_seat);
        if (ret == SQL_NO_DATA)
        {
            snprintf((*choices)[0], max_length, "ERROR: Vuelo lleno (ID: %d)", (int)flight_id);
            flag_consulta1 = 0;
            SQLCloseCursor(stmt_seat);
            break; 
        }
        if (!SQL_SUCCEEDED(ret))
        {
            snprintf((*choices)[0], max_length, "ERROR BD al buscar asiento (fetch)");
            flag_consulta1 = 0;
            SQLCloseCursor(stmt_seat);
            break;
        }
        SQLCloseCursor(stmt_seat);

        /* 2. Ejecución Consulta 3 (Generar boarding_no) */
        SQLBindParameter(stmt_bno, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &flight_id, 0, &ind_fid);
        SQLBindCol(stmt_bno, 1, SQL_C_LONG, &new_boarding_no, sizeof(new_boarding_no), &ind_bno);
        
        ret = SQLExecute(stmt_bno);
        if (!SQL_SUCCEEDED(ret)) {
            snprintf((*choices)[0], max_length, "ERROR BD al generar boarding_no");
            flag_consulta1 = 0;
            SQLCloseCursor(stmt_bno);
            break;
        }
        ret = SQLFetch(stmt_bno);
        if (!SQL_SUCCEEDED(ret))
        {
            snprintf((*choices)[0], max_length, "ERROR BD al generar boarding_no");
            flag_consulta1 = 0;
            SQLCloseCursor(stmt_bno);
            break;
        }
        SQLCloseCursor(stmt_bno);

        /* 3. Ejecución Consulta 4 (Insertar) */
        SQLBindParameter(stmt_insert, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 13, 0, ticket_no, 0, &ind_tk);
        SQLBindParameter(stmt_insert, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &flight_id, 0, &ind_fid);
        SQLBindParameter(stmt_insert, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &new_boarding_no, 0, &ind_bno);
        SQLBindParameter(stmt_insert, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 4, 0, seat_no, 0, &ind_seat);
        
        ret = SQLExecute(stmt_insert);
        if (!SQL_SUCCEEDED(ret))
        {
            snprintf((*choices)[0], max_length, "ERROR BD al insertar la tarjeta embarque");
            flag_consulta1 = 0;
            break;
        }

        /* 4. Éxito para este billete: Formatear salida */
        snprintf((*choices)[i], max_length, "Nombre: %-20.20s | Vuelo: %-6d | Salida: %-19s | Asiento: %-4s",
                 (char *)passenger_name,
                 (int)flight_id,
                 (char *)scheduled_departure,
                 (char *)seat_no);
        i++;
    }

    /* --- Finalizar Transacción --- */
    if (flag_consulta1 == 1)
    {
        /* Éxito: Confirmar transacción */
        SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);
        if (i == 0)
        {
            snprintf((*choices)[0], max_length, "No hay nuevas tarjetas de embarque para esta reserva.");
            *n_choices = 1;
        }
        else
        {
            *n_choices = i;
        }
    }
    else
    {
        /* Error: Revertir transacción */
        SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_ROLLBACK);
        *n_choices = 1;
    }

    /* LIMPIEZA */
    SQLCloseCursor(stmt_tickets);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_tickets);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_seat);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_bno);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_insert);

    /* Reactivar Autocommit */
    SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    odbc_disconnect(env, dbc);
}