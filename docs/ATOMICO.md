# Versión Atómica: Prioridad de Escritores (R8 - R10)

Implementación sin mutex ni variables de condición en `include/control_atomico_rw.h` y `src/control_atomico_rw.cpp`.

## Formato de la palabra de 64 bits

El estado se guarda en un único `std::atomic<std::uint64_t>` (verificado como lock-free en hardware con `static_assert`):

* **Bit 0 (1 bit):** `escritora_activa` (`0` libre, `1` escribiendo).
* **Bits 1..16 (16 bits):** `escritoras_esperando` (hasta 65.535).
* **Bits 17..32 (16 bits):** `lectoras_activas` (hasta 65.535).
* **Bits 33..63 (31 bits):** Sin uso (siempre en `0`).

Con la restricción del enunciado ($L + E \le 1000$), los campos de 16 bits nunca sufren desbordamiento.

## Protocolo de admisión y salida

* **Lectora (`entrar_lectura`):** Lee `estado` con `load()`. Si `escritora_activa == 1` o `escritoras_esperando > 0`, registra el fallo en `intentos_hebra` (priorizando `recurso_ocupado` si ocurren ambas), llama a `std::this_thread::yield()` y reintenta. Si ambos son `0`, intenta sumar `1` en los bits de `lectoras_activas` usando `compare_exchange_weak`.
* **Escritora (`entrar_escritura`):** Primero suma `1` a `escritoras_esperando` con `fetch_add` para bloquear de inmediato a lectoras nuevas. Luego espera activamente hasta que `escritora_activa == 0` y `lectoras_activas == 0`. Al cumplirse, ejecuta un único `compare_exchange_weak` que resta `1` de `escritoras_esperando` y enciende el bit `escritora_activa` al mismo tiempo.
* **Salidas (`salir_lectura` / `salir_escritura`):** Usan `fetch_sub` directo sobre su campo correspondiente, ya que cada hebra que sale es dueña única de su permiso activo.

## Por qué la solución no es lock-free

Aunque no usa mutex y emplea CAS, el protocolo no garantiza progreso global (no es *lock-free*): si una escritora gana el CAS (dejando `escritora_activa = 1`) y justo después el planificador del sistema operativo la suspende antes de llamar a `salir_escritura()`, el resto de las hebras (lectoras y escritoras) quedarán reintentando indefinidamente en espera activa sin poder completar sus operaciones hasta que esa hebra vuelva a recibir CPU.