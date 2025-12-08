#!/usr/bin/expect -f
# test_use_deleted_books.sh
# Objetivo: Comprobar que tras borrar un libro, el siguiente add reutiliza el hueco.

set timeout -1
set programName "library"
set filename "test_reuse"

# 1. Limpieza previa: Borramos archivos antiguos para empezar de cero
spawn rm -f $filename.db $filename.ind $filename.lst

spawn ./$programName first_fit $filename
expect "Type command and argument/s."

# Añadimos Libro 1 (Ocupará Offset 0)
send  "add 12346|978-2-12345680-3|El Quijote|Catedra\r"
expect "Record with BookID=12346 has been added"

# Añadimos Libro 2 (Ocupará Offset 46 aprox) -> ESTE LO BORRAREMOS
# Tamaño calculado en tus tests anteriores: 36 bytes payload
send  "add 12345|978-2-12345086-3|La busca|Catedra\r"
expect "Record with BookID=12345 has been added"

# Añadimos Libro 3 (Ocupará Offset 90 aprox) -> TOPE
send  "add 12347|978-2-12345680-4|Amar|catedra\r"
expect "Record with BookID=12347 has been added"

send  "add 12348|978-2-12345680-1|La insoportable levedad del ser|Catedra\r"
expect "Record with BookID=12346 has been added"

send  "add 12348|978-2-12345680-1|El Cuentacuentos maldito de los cien demonios alienados en la sociedad postmoderna|Catedra\r"
expect "Record with BookID=12346 has been added"

# PASO 2: BORRAR EL LIBRO DE EN MEDIO (12345)


send "del 12345\r"
expect "Record with BookID=12345 has been deleted"



# Verificamos que el hueco está en la lista de borrados
send "printLst\n"
# Esperamos ver el offset 46 (que era donde estaba el libro 12345)
expect "offset: #46"
expect "size: #36"

# PASO 3: AÑADIR UN NUEVO LIBRO QUE QUEPA EN EL HUECO


send  "add 99999|978-2-00000000-0|Edito|Si\r"
expect "Record with BookID=99999 has been added"

send "printInd\n"
expect "Entry #0"
expect "    key: #12346"
expect "    offset: #0"
expect "    size: #38"
expect "Entry #1"
expect "    key: #12347"
expect "    offset: #90"
expect "    size: #38"
expect "Entry #2"
expect "    key: #99999"
expect "    offset: #46"
expect "    size: #28"
expect "exit"

send "printLst\n"
expect "exit"

send "exit\n"
expect "all done"

# 1. Comprobación lógica del script (Si llegamos aquí, Expect no falló)
puts "1) Logic Check: OK ;-)"

