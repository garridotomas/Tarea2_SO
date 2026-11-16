#include "control_atomico_rw.h"
#include "monitor_rw.h"
#include "verificador.h"
#include <iostream>

// RESPONSABLE: integrante 3.
int main() {
    std::cout
        << "Base creada. Implementacion pendiente.\n"
        << "Leer docs/BASE_EQUIPO.md.\n";

    // TODO(3), en este orden:
    // 1. Leer y validar los argumentos del enunciado.
    // 2. Crear MonitorRW o ControlAtomicoRW.
    // 3. Crear un unico Registro compartido.
    // 4. Crear L lectoras y E escritoras.
    // 5. Cada hebra ejecuta K operaciones.
    // 6. Esperar todas las hebras con join().
    // 7. Comprobar invariantes y verificar traza.
    // 8. Mostrar mediciones y resultados.
    //
    // El registro se utiliza ENTRE entrar_* y salir_*.
    // El trabajo simulado tambien ocurre en ese intervalo.
    //
    // Cada hebra asigna su id_hebra al comenzar.
    // Antes de terminar copia intentos_hebra a su propia
    // casilla de resultados; main suma despues de join().
    //
    // Capturar errores dentro de cada hebra.
    // Si falla crear una hebra, esperar las ya creadas.
    //
    // No declarar correcto=SI mientras falten componentes.

    return 2; // Esqueleto: no ejecuta un experimento.
}
