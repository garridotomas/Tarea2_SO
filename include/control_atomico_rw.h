#pragma once
#include "control_rw.h"
#include "tipos.h"
#include <atomic>
#include <cstdint>

// RESPONSABLE: integrante 2.
// Solo implementa prioridad de escritores.
class ControlAtomicoRW : public ControlRW {
public:
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

private:
    static_assert(
        std::atomic<std::uint64_t>::is_always_lock_free,
        "Se requiere uint64_t atomic sin candado interno");

    std::atomic<std::uint64_t> estado{0};

    // TODO(2): definir campos de bits y sus mascaras.
    // Admision mediante CAS exitoso sobre estado.
};
