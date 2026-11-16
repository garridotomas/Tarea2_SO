#include "monitor_rw.h"
#include <stdexcept>

MonitorRW::MonitorRW(Politica p, bool habilitar_traza)
    : politica(p), registrar_traza(habilitar_traza) {}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);
    // TODO(1): implementar entrar_lectura.
    throw std::logic_error("Pendiente: MonitorRW::entrar_lectura");
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);
    // TODO(1): implementar salir_lectura.
    throw std::logic_error("Pendiente: MonitorRW::salir_lectura");
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);
    // TODO(1): implementar entrar_escritura.
    throw std::logic_error("Pendiente: MonitorRW::entrar_escritura");
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);
    // TODO(1): implementar salir_escritura.
    throw std::logic_error("Pendiente: MonitorRW::salir_escritura");
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
