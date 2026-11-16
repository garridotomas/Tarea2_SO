#pragma once
#include "tipos.h"
#include <vector>

// RESPONSABLE: integrante 4.
// Se llama despues de los join(), con traza habilitada.
ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica);
