#pragma once
#include "control_rw.h"
#include "tipos.h"
#include <condition_variable>
#include <mutex>
#include <vector>

// RESPONSABLE: integrante 1.
class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(
        Politica p, bool habilitar_traza = true);

    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

    std::vector<AnotacionTraza> obtener_traza();

private:
    Politica politica;
    bool registrar_traza;
    std::mutex m;
    std::vector<AnotacionTraza> traza;

    // TODO(1): agregar contadores, condiciones y turnos.
    // El estado de admision se protege siempre con m.
};
