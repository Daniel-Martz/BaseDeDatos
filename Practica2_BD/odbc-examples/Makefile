CC = gcc -g
CFLAGS = -Wall -Wextra -pedantic #-ansi
LDLIBS = -lodbc

export PGDATABASE:=test01
export PGUSER :=alumnodb
export PGPASSWORD :=alumnodb
export PGCLIENTENCODING:=LATIN9
export PGHOST:=localhost

DBNAME =$(PGDATABASE)
PSQL = psql
CREATEDB = createdb
DROPDB = dropdb --if-exists
PG_DUMP = pg_dump
PG_RESTORE = pg_restore

test01: dropdb createdb restore shell
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


EXE = odbc-connection-test odbc-connection-test-2 odbc-example1 odbc-example2 odbc-example3 odbc-example4 

all : $(EXE)

test01: 

clean :
	rm -f *.o core $(EXE)

$(EXE) : % : %.o odbc.o
