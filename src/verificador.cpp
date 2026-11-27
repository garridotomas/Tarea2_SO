#include "verificador.h"
#include <stdexcept>

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica) {

    (void)traza;
    (void)politica;

    // TODO(4): reconstruir pendientes y activas.
    // Comprobar reglas R6 y calcular evidencia.
    // Ante error: devolver indice y explicacion.
    throw std::logic_error(
        "TODO(4): implementar verificar_traza");
}
