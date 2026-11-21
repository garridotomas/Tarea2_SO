#pragma once
#include "control_rw.h"
#include "tipos.h"
#include <condition_variable>
#include <mutex>
#include <vector>

// Monitor comun para las politicas de lectores y escritores.
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

    // Todo este estado se consulta y modifica con m adquirido.
    std::condition_variable cambio_estado;
    int lectoras_activas = 0;
    int lectoras_esperando = 0;
    int escritoras_esperando = 0;
    bool escritora_activa = false;

    // Solo se llama con m adquirido; FIFO agregara sus propios turnos.
    void anotar(Tipo tipo, Accion accion);
};
