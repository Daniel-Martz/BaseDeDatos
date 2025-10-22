#ifndef AUX_H
#define AUX_H

/* CONNECTION PARAMETERS, ADAPT TO YOUR SCENARIO */
<<<<<<< HEAD
=======
#define CONNECTION_PARS "DRIVER=PostgreSQL ANSI;DATABASE=flight;SERVER=localhost;PORT=5432;UID=alumnodb;PWD=alumnodb;"
>>>>>>> 3750b002218b6c7c0d4c0d5208be3f5937eb3b40

/* REPORT OF THE MOST RECENT ERROR USING HANDLE handle */
void odbc_extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type);

/* STANDARD CONNECTION PROCEDURE */
int odbc_connect(SQLHENV* env, SQLHDBC* dbc);

/* STANDARD DISCONNECTION PROCEDURE */
int odbc_disconnect(SQLHENV env, SQLHDBC dbc);

#endif
