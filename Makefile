# Version 1.0
# Por lo general no se requiere modificar el fichero

export PGDATABASE:=flight
export PGUSER :=alumnodb
export PGPASSWORD :=alumnodb
export PGCLIENTENCODING:=UTF8
export PGHOST:=localhost

DBNAME =$(PGDATABASE)
PSQL = psql
CREATEDB = createdb
DROPDB = dropdb --if-exists
PG_DUMP = pg_dump
PG_RESTORE = pg_restore

all: dropdb createdb restore shell

mycomando:
	@echo aqui va una descripcion
	@cat myfichero.sql | psql mibasededatos
createdb:
	@echo Creando BBDD
	@$(CREATEDB)
dropdb:
	@echo Eliminando BBDD
	@$(DROPDB) $(DBNAME)
	rm -f *.log
dump:
	@echo creando dumpfile
	@$(PG_DUMP) > $(DBNAME).sql
restore:
	@echo restore data base
	@cat $(DBNAME).sql | $(PSQL)  
psql: shell
shell:
	@echo create psql shell
	@$(PSQL)  

allqueries: query1 query2 query3 query4 query5 query6
	@cat query?.log > all_queries.log

query1:
	@echo query-1: "Esta consulta muestra el numero de reservas que contienen un billete de ida y vuelta por aeropuerto." | tee query1.log
	@cat query1.sql | $(PSQL) | tee -a query1.log
query2:
	@echo query-2: "Esta consulta muestra el precio de la reserva a partir del precio de cada vuelo." | tee query2.log
	@cat query2.sql | $(PSQL) | tee -a query2.log
query3:
	@echo query3: "Esta consulta muestra el numero de pasajeros recibidos por aeropuerto." | tee query3.log
	@cat query3.sql | $(PSQL) | tee -a query3.log
query4:
	@echo query4: "please insert here a short description" | tee query4.log
	@cat query4.sql | $(PSQL) | tee -a query4.log
query5:
	@echo query5: "Esta consulta muestra las reservas para las que no se emitio una tarjeta de embarque." | tee query5.log
	@cat query5.sql | $(PSQL) | tee -a query5.log
query6:
	@echo query6: "Esta consulta muestra el trayecto donde la media de los retrasos acumulados sea mayor." | tee query6.log
	@cat query6.sql | $(PSQL) | tee -a query6.log
