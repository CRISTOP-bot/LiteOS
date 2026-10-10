# Criptografía en LiteOS

## Módulos presentes

La carpeta contiene implementaciones portables freestanding de SHA-256 y SHA-512, además de helpers de memoria/comparación en `secure.c`. Los hashes están escritos desde FIPS 180-4; el repositorio no tiene dependencias de libc del host ni asignación dinámica en estas rutas.

`crypto.h` define las API one-shot e incremental. `init` debe preceder a cada ciclo de uso; `update` acepta bloques parciales y mensajes vacíos; `final` calcula el digest y borra el contexto con `crypto_secure_zero`. Un error de argumentos o de longitud en `update` no modifica el contexto activo. Después de un `final` exitoso, hay que reinicializar antes de reutilizarlo. No copies ni compartas concurrentemente contextos.

Para SHA-256, la longitud acumulada máxima es \(2^{61}-1\) bytes, límite de la longitud de mensaje de SHA-256. SHA-512 conserva un contador de bytes de 128 bits y rechaza longitudes mayores que el máximo representable por su campo de longitud en bits. No se reserva memoria dinámica.

## Buffers y errores

Los punteros deben referirse a buffers válidos y escribibles/legibles según corresponda. `NULL` solo es válido para una entrada de longitud cero y para operaciones de borrado/comparación de longitud cero. Los buffers de entrada de `update` no deben solaparse con el contexto; la salida de `final` no debe solaparse con el contexto. Las funciones one-shot leen toda la entrada antes de escribir el digest, por lo que permiten que la salida se solape con la entrada. Las operaciones son síncronas y no retienen punteros del llamador.

`CRYPTO_OK` es éxito y `CRYPTO_ERR_INVALID_ARGUMENT` es fallo de argumento o longitud. `crypto_ct_equal` retorna cero si los bytes son iguales, uno si difieren y negativo ante punteros inválidos. Su bucle recorre todos los bytes para una longitud fija; la longitud es pública. El código C y el hardware no permiten prometer una garantía absoluta de tiempo constante.

`crypto_secure_zero` usa escrituras `volatile` para evitar que el compilador elimine el borrado. No puede borrar copias que hayan quedado en registros, cachés u otros lugares fuera del buffer indicado.

## Aleatoriedad y estado de seguridad

Esta fase **no implementa un generador aleatorio criptográfico**. LiteOS devuelve `ENOSYS` al leer `/dev/random` y `/dev/urandom`; esa conducta de fallo explícito se conserva. No usar contadores, TSC, PID, direcciones, xorshift ni hashes simples como entropía o como CSPRNG. No se han añadido cifrados, MAC, KDF, firmas ni intercambio de claves.

## Pruebas

`make test-crypto` construye la prueba host existente con los dos hashes y los helpers nuevos. Comprueba los vectores de SHA-2 existentes, longitudes en los bordes de bloques y padding, procesamiento incremental, errores de argumentos, borrado de contexto y comparaciones. Los vectores `empty` y `abc` son los casos conocidos de referencia; los patrones repetidos se contrastaron con `hashlib` al preparar esta fase. La prueba host no forma parte del kernel arrancable.
