#include "control_atomico_rw.h"
#include <thread>

void ControlAtomicoRW::entrar_lectura() {
    std::uint64_t esperado = estado.load();

    while (true) {
        bool esc_activa = (esperado & MASK_ESC_ACTIVA) != 0;
        std::uint64_t esc_esperando = (esperado & MASK_ESC_ESP) >> SHIFT_ESC_ESP;

        // Prioridad de escritores: si hay una escribiendo o esperando, no entramos
        if (esc_activa || esc_esperando > 0) {
            if (esc_activa) {
                intentos_hebra.recurso_ocupado++;
            } else {
                intentos_hebra.escritoras_esperando++;
            }
            std::this_thread::yield();
            esperado = estado.load();
            continue;
        }

        // Sumamos 1 lectora activa de forma atomica
        std::uint64_t nuevo = esperado + UNO_LEC_ACT;
        if (estado.compare_exchange_weak(esperado, nuevo)) {
            return;
        }

        // Si el CAS falla, 'esperado' ya trae el valor nuevo para la siguiente vuelta
        intentos_hebra.cas_fallido++;
    }
}

void ControlAtomicoRW::salir_lectura() {
    estado.fetch_sub(UNO_LEC_ACT);
}

void ControlAtomicoRW::entrar_escritura() {
    // Nos anotamos en la fila primero para bloquear a las lectoras nuevas
    estado.fetch_add(UNO_ESC_ESP);

    std::uint64_t esperado = estado.load();

    while (true) {
        bool esc_activa = (esperado & MASK_ESC_ACTIVA) != 0;
        std::uint64_t lec_activas = (esperado & MASK_LEC_ACT) >> SHIFT_LEC_ACT;

        // Solo pasamos si no hay nadie escribiendo ni leyendo
        if (esc_activa || lec_activas > 0) {
            intentos_hebra.recurso_ocupado++;
            std::this_thread::yield();
            esperado = estado.load();
            continue;
        }

        // En un mismo CAS bajamos las esperando y marcamos escritora activa
        std::uint64_t nuevo = (esperado - UNO_ESC_ESP) | MASK_ESC_ACTIVA;
        if (estado.compare_exchange_weak(esperado, nuevo)) {
            return;
        }

        intentos_hebra.cas_fallido++;
    }
}

void ControlAtomicoRW::salir_escritura() {
    estado.fetch_sub(MASK_ESC_ACTIVA);
}