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

    // Palabra de 64 bits:
    // [63..33: libre] | [32..17: lectoras_activas] | [16..1: escritoras_esperando] | [0: escritora_activa]
    std::atomic<std::uint64_t> estado{0};

    // Desplazamientos y mascaras para cada campo de bits
    static constexpr std::uint64_t SHIFT_ESC_ACTIVA = 0;
    static constexpr std::uint64_t MASK_ESC_ACTIVA  = 0x1ULL;

    static constexpr std::uint64_t SHIFT_ESC_ESP    = 1;
    static constexpr std::uint64_t MASK_ESC_ESP     = 0xFFFFULL << SHIFT_ESC_ESP;
    static constexpr std::uint64_t UNO_ESC_ESP      = 1ULL << SHIFT_ESC_ESP;

    static constexpr std::uint64_t SHIFT_LEC_ACT    = 17;
    static constexpr std::uint64_t MASK_LEC_ACT     = 0xFFFFULL << SHIFT_LEC_ACT;
    static constexpr std::uint64_t UNO_LEC_ACT      = 1ULL << SHIFT_LEC_ACT;
};