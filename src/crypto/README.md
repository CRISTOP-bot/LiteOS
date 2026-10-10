# Criptografía en LiteOS

## Módulos presentes

La carpeta contiene implementaciones portables freestanding de SHA-256 y SHA-512, helpers de memoria/comparación en `secure.c`, HMAC-SHA-256/SHA-512 en `mac/hmac.c` y HKDF-SHA-256/SHA-512 y PBKDF2-HMAC-SHA-256 en `kdf/`. No hay dependencias de libc del host ni asignación dinámica en estas rutas.

La API pública permanece en `src/crypto.h`, conforme a la convención ya usada por el kernel y sus includes; no se duplicó el header bajo `src/crypto/`. `init` precede cada ciclo de contexto; `update` procesa bloques parciales; `final` termina y limpia el estado. Las funciones son síncronas y no retienen punteros del llamador. No copies ni compartas contextos activos.

Para SHA-256, la longitud acumulada máxima es \(2^{61}-1\) bytes, límite de la longitud de mensaje de SHA-256. SHA-512 conserva un contador de bytes de 128 bits y rechaza longitudes mayores que el máximo representable por su campo de longitud en bits. No se reserva memoria dinámica.

## Buffers y errores

Los punteros deben referirse a buffers válidos y escribibles/legibles según corresponda. `NULL` solo es válido para una entrada de longitud cero y para operaciones de borrado/comparación de longitud cero. Los buffers de entrada de `update` no deben solaparse con el contexto; la salida de `final` no debe solaparse con el contexto que se limpia. Las funciones one-shot HMAC consumen la clave y el mensaje antes de escribir la etiqueta, por lo que la salida puede solaparse con ellos. HKDF-Extract consume sus entradas antes de escribir el PRK. HKDF-Expand copia el PRK de entrada antes de escribir la salida, pero esta no debe solaparse con `info`; la API combinada tampoco permite solapar `info` con la salida. PBKDF2 no permite que su salida se solape con la contraseña o la sal. Las salidas de HKDF no superan 255 bloques de digest (8160 bytes para SHA-256; 16320 para SHA-512). PBKDF2 requiere al menos una iteración y limita los bloques de salida a 2^32-1. Las operaciones no asignan memoria dinámica.

`CRYPTO_OK` es éxito y `CRYPTO_ERR_INVALID_ARGUMENT` es fallo de argumento o longitud. `crypto_ct_equal` retorna cero si los bytes son iguales, uno si difieren y negativo ante punteros inválidos. Su bucle recorre todos los bytes para una longitud fija; la longitud es pública. El código C y el hardware no permiten prometer una garantía absoluta de tiempo constante.

`crypto_secure_zero` usa escrituras `volatile` para evitar que el compilador elimine el borrado. No puede borrar copias que hayan quedado en registros, cachés u otros lugares fuera del buffer indicado.

## MAC y KDF (fase 2)

HMAC-SHA-256 y HMAC-SHA-512 siguen RFC 2104 y aceptan API incremental y one-shot. HMAC autentica mensajes con una clave; no se debe sustituir por un hash simple. Los contextos guardan el pad externo y se limpian al finalizar. La implementación no promete resistencia absoluta a canales laterales.

HKDF-SHA-256/SHA-512 siguen RFC 5869. `salt == NULL && salt_len == 0` representa salt omitido y usa HashLen bytes cero según el RFC; un puntero no nulo de longitud cero representa una clave HMAC vacía. Expand admite como máximo 255 bloques. HKDF deriva material de claves, no es una función de almacenamiento de contraseñas.

PBKDF2-HMAC-SHA-256 sigue RFC 8018 y limita el coste a un contador de iteraciones no nulo; el llamador configura las iteraciones según el uso y los recursos disponibles. Está disponible para compatibilidad, pero no convierte PBKDF2 en la opción preferible para almacenar contraseñas nuevas; no se fija un coste universal.

## Aleatoriedad y estado de seguridad

Esta fase **no implementa un generador aleatorio criptográfico**. LiteOS devuelve `ENOSYS` al leer `/dev/random` y `/dev/urandom`; esa conducta de fallo explícito se conserva. No usar contadores, TSC, PID, direcciones, xorshift ni hashes simples como entropía o como CSPRNG. No se han añadido cifrados, AEAD, firmas ni intercambio de claves.

## Pruebas

`make test-crypto` conserva la prueba SHA-2 existente y ejecuta también `tests/crypto_phase2_test.c`. La nueva suite comprueba HMAC con vectores RFC 4231 (incluida una clave mayor que el bloque), HKDF-SHA-256 con RFC 5869 casos 1–3, PBKDF2-HMAC-SHA-256 con RFC 7914 y errores de parámetros/contextos. Para HKDF-SHA-512, RFC 5869 no publica un vector SHA-512; el vector adicional se contrastó con Python `hmac`/`hashlib` independiente. Esto no reemplaza la compilación freestanding o pruebas en QEMU.
