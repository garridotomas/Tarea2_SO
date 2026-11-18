#include "monitor_rw.h"
#include <stdexcept>

MonitorRW::MonitorRW(Politica p, bool habilitar_traza)
    : politica(p), registrar_traza(habilitar_traza) {
    // Evita ejecutar FIFO accidentalmente como otra politica.
    if (p == Politica::Fifo) {
        throw std::logic_error("Pendiente: politica FIFO de MonitorRW");
    }
    if (p != Politica::Lectores && p != Politica::Escritores) {
        throw std::invalid_argument("Politica de monitor invalida");
    }
}

void MonitorRW::anotar(Tipo tipo, Accion accion) {
    if (registrar_traza) {
        traza.push_back({id_hebra, tipo, accion, -1});
    }
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);
    ++lectoras_esperando;
    anotar(Tipo::Lectora, Accion::Pide);

    // wait libera m mientras espera y lo recupera antes de continuar.
    cambio_estado.wait(lock, [this] {
        return !escritora_activa
            && (politica == Politica::Lectores || escritoras_esperando == 0);
    });

    --lectoras_esperando;
    ++lectoras_activas;
    anotar(Tipo::Lectora, Accion::Entra);
    // Al retornar se libera m, pero la lectora conserva su permiso.
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);
    --lectoras_activas;
    anotar(Tipo::Lectora, Accion::Sale);

    // La ultima lectora puede habilitar a una escritora.
    if (lectoras_activas == 0) {
        cambio_estado.notify_all();
    }
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);
    // Se cuenta antes de esperar para bloquear nuevas lectoras en R4.
    ++escritoras_esperando;
    anotar(Tipo::Escritora, Accion::Pide);

    cambio_estado.wait(lock, [this] {
        return !escritora_activa && lectoras_activas == 0
            && (politica == Politica::Escritores || lectoras_esperando == 0);
    });

    --escritoras_esperando;
    escritora_activa = true;
    anotar(Tipo::Escritora, Accion::Entra);
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);
    escritora_activa = false;
    anotar(Tipo::Escritora, Accion::Sale);

    // Cada hebra despertada vuelve a comprobar su condicion bajo m.
    cambio_estado.notify_all();
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
