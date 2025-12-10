#!/usr/bin/expect -f
# test_use_deleted_books.sh
# Objetivo: Comprobar que tras borrar un libro, el siguiente add reutiliza el hueco.

set timeout -1
set programName "library"
set filename "test_reuse"

# 1. Limpieza previa: Borramos archivos antiguos para empezar de cero
spawn rm -f $filename.db $filename.ind $filename.lst

spawn ./$programName worst_fit $filename
expect "Type command and argument/s."

# Añadimos Libro (Ocupará Offset 0)
send  "add 12346|978-2-12345680-3|El Quijote|Catedra\r"
expect "Record with BookID=12346 has been added"
expect "exit"

# Añadimos Libro (Ocupará Offset 46)
send  "add 12345|978-2-12345086-3|La busca|Catedra\r"
expect "Record with BookID=12345 has been added"
expect "exit"

# Añadimos Libro (Ocupará Offset 90)
send  "add 12347|978-2-12345680-4|Amar|catedra\r"
expect "Record with BookID=12347 has been added"
expect "exit"

# Añadimos Libro (Ocupará Offset 130)
send  "add 12348|978-2-12345086-3|la busca|catedra\r"
expect "Record with BookID=12348 has been added to the database"
expect "exit"


#Veamos que se han guardado correctamente y ordenadas por key de menor a mayor
puts "------------------------"
send "printInd\n"
expect "Entry #0"
expect "    key: #12345"
expect "    offset: #46"
expect "    size: #36"
expect "Entry #1"
expect "    key: #12346"
expect "    offset: #0"
expect "    size: #38"
expect "Entry #2"
expect "    key: #12347"
expect "    offset: #90"
expect "    size: #32"
expect "Entry #3"
expect "    key: #12348"
expect "    offset: #130"
expect "    size: #36"

#delete book 12346
send "del 12346\r"
expect "Record with BookID=12346 has been deleted"
expect "exit"
send "printInd\n"
expect "Entry #0"
expect "    key: #12345"
expect "    offset: #46"
expect "    size: #36"
expect "Entry #1"
expect "    key: #12347"
expect "    offset: #90"
expect "    size: #32"
expect "Entry #2"
expect "    key: #12348"
expect "    offset: #130"
expect "    size: #36"

#delete book 12347
send "del 12347\r"
expect "Record with BookID=12347 has been deleted"
expect "exit"
send "printInd\n"
expect "Entry #0"
expect "    key: #12345"
expect "    offset: #46"
expect "    size: #36"
expect "Entry #1"
expect "    key: #12348"
expect "    offset: #130"
expect "    size: #36"

#Veamos los huecos (deben estar ordenados de mayor a menor size)
send "printLst\n"
expect "Entry #0"
expect "    offset: #0"
expect "    size: #38"
expect "Entry #1"
expect "    offset: #90"
expect "    size: #32"

# Estos dos libros deben de guardarse a continuación del resto (offset: 174 para adelante)
send  "add 12347|978-2-12345680-1|La insoportable levedad del ser|Catedra\r"
expect "Record with BookID=12347 has been added"

send  "add 12346|978-2-12345680-1|El Cuentacuentos maldito de los cien demonios alienados en la sociedad postmoderna|Catedra\r"
expect "Record with BookID=12346 has been added"

# Aquí vemos los dos libros insertados al final
send  "printInd\n"

expect "Entry #0"
expect "    key: #12345"
expect "    offset: #46"
expect "    size: #36"
expect "Entry #1"
expect "    key: #12346"
expect "    offset: #241"
expect "    size: #110"
expect "Entry #2"
expect "    key: #12347"
expect "    offset: #174"
expect "    size: #59"
expect "Entry #3"
expect "    key: #12348"
expect "    offset: #130"
expect "    size: #36"

# Observamos que no se han modificado los huecos
send  "printLst\n"
expect "Entry #0"
expect "    offset: #0"
expect "    size: #38"
expect "Entry #1"
expect "    offset: #90"
expect "    size: #32"

# BORRAR EL LIBRO DE EN MEDIO (12345)
send "del 12345\r"
expect "Record with BookID=12345 has been deleted"

# Verificamos que el hueco está en la lista de borrados. Esperamos ver el offset 46 (que era donde estaba el libro 12345)
send "printLst\n"
expect "Entry #0"
expect "    offset: #0"
expect "    size: #38"
expect "Entry #1"
expect "    offset: #46"
expect "    size: #36"
expect "Entry #2"
expect "    offset: #90"
expect "    size: #32"

# AÑADIR UN NUEVO LIBRO QUE QUEPA EN EL HUECO
send  "add 99999|978-2-00000000-0|Edito|Si\r"
expect "Record with BookID=99999 has been added"

# Vemos que se añade la primer hueco donde entra que si vemos lo impreso en printLst es el hueco de offset 0
send "printInd\n"
expect "Entry #0"
expect "    key: #12346"
expect "    offset: #241"
expect "    size: #110"
expect "Entry #1"
expect "    key: #12347"
expect "    offset: #174"
expect "    size: #59"
expect "Entry #2"
expect "    key: #12348"
expect "    offset: #130"
expect "    size: #36"
expect "Entry #3"
expect "    key: #99999"
expect "    offset: #0"
expect "    size: #28"
expect "exit"

# Se debe haber eliminado el hueco de offset 0 y se debe haber añadido uno nuevo puesto que el tamaño del dato es 28 y el del hueco era de 38
send "printLst\n"
expect "Entry #0"
expect "    offset: #46"
expect "    size: #36"
expect "Entry #1"
expect "    offset: #90"
expect "    size: #32"
expect "Entry #2"
expect "    offset: #28"
expect "    size: #10"
expect "exit"

send "exit\n"
expect "all done"

# 1. Comprobación lógica del script (Si llegamos aquí, Expect no falló)
puts "1) Logic Check: OK ;-)"

if {[file exists [file join $filename.ind]]} {
    puts "2) file $filename.ind Exists, ;-)"
} else {
    puts "2) file $filename.ind NOT found, :-("
}

## call diff program for index
set output "differ"
try {
set output [exec diff -s $filename.ind ${filename}_control_wf.ind]
} trap CHILDSTATUS {} {}
if {[regexp -nocase "identical" $output] || [regexp -nocase "idénticos" $output]} {
    puts "3) index files are identical, ;-)"
} else {
    puts "3) files differ, :-("
}

## call diff program for list
set output "differ"
try {
set output [exec diff -s $filename.lst ${filename}_control_wf.lst]
} trap CHILDSTATUS {} {}
if {[regexp -nocase "identical" $output] || [regexp -nocase "idénticos" $output]} {
    puts "3) delete books files are identical, ;-)"
} else {
    puts "3) files differ, :-("
}
puts "4) Script end"