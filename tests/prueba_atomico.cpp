#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cassert>

#include "control_atomico_rw.h"
#include "tipos.h"

int main() {
    const int L = 6;
    const int E = 2;
    const int K = 100;

    ControlAtomicoRW control;
    Registro reg{};

    std::atomic<int> lectoras_dentro{0};
    std::atomic<int> escritoras_dentro{0};
    std::atomic<int> max_lectoras{0};
    std::atomic<long> inconsistencias{0};
    std::atomic<long> violaciones_exclusion{0};

    // Acumuladores globales para sumar los fallos locales de cada hebra al terminar
    std::atomic<std::uint64_t> total_ocupado{0};
    std::atomic<std::uint64_t> total_esc_esp{0};
    std::atomic<std::uint64_t> total_cas_fallido{0};

    std::vector<std::thread> hebras;

    // Lanzamos las lectoras
    for (int i = 0; i < L; ++i) {
        hebras.emplace_back([&, i]() {
            id_hebra = i;
            for (int k = 0; k < K; ++k) {
                control.entrar_lectura();

                int lec = lectoras_dentro.fetch_add(1) + 1;
                if (escritoras_dentro.load() > 0) {
                    violaciones_exclusion++;
                }

                // Actualizamos el maximo de lectoras simultaneas
                int prev_max = max_lectoras.load();
                while (lec > prev_max && !max_lectoras.compare_exchange_weak(prev_max, lec)) {}

                // Verificamos invariante del registro compartido
                long v = reg.version;
                long dv = reg.doble_version;
                if (dv != 2 * v) {
                    inconsistencias++;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(200));

                lectoras_dentro.fetch_sub(1);
                control.salir_lectura();
            }

            // Sumamos las estadisticas locales antes de que muera la hebra
            total_ocupado += intentos_hebra.recurso_ocupado;
            total_esc_esp += intentos_hebra.escritoras_esperando;
            total_cas_fallido += intentos_hebra.cas_fallido;
        });
    }

    // Lanzamos las escritoras
    for (int i = 0; i < E; ++i) {
        hebras.emplace_back([&, i]() {
            id_hebra = L + i;
            for (int k = 0; k < K; ++k) {
                control.entrar_escritura();

                int esc = escritoras_dentro.fetch_add(1) + 1;
                if (esc > 1 || lectoras_dentro.load() > 0) {
                    violaciones_exclusion++;
                }

                // Modificamos el registro compartido
                reg.version++;
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                reg.doble_version = 2 * reg.version;

                escritoras_dentro.fetch_sub(1);
                control.salir_escritura();
            }

            total_ocupado += intentos_hebra.recurso_ocupado;
            total_esc_esp += intentos_hebra.escritoras_esperando;
            total_cas_fallido += intentos_hebra.cas_fallido;
        });
    }

    for (auto& h : hebras) {
        h.join();
    }

    std::cout << "Version final: " << reg.version << " (esperado: " << E * K << ")\n";
    std::cout << "Doble version: " << reg.doble_version << " (esperado: " << 2 * E * K << ")\n";
    std::cout << "Inconsistencias: " << inconsistencias.load() << "\n";
    std::cout << "Violaciones exclusion: " << violaciones_exclusion.load() << "\n";
    std::cout << "Max lectoras simultaneas: " << max_lectoras.load() << "\n";
    std::cout << "Fallos -> Ocupado: " << total_ocupado.load()
              << " | Esc esperando: " << total_esc_esp.load()
              << " | CAS fallido: " << total_cas_fallido.load() << "\n";

    assert(reg.version == E * K);
    assert(reg.doble_version == 2 * E * K);
    assert(inconsistencias == 0);
    assert(violaciones_exclusion == 0);
    assert(total_esc_esp > 0); // Evidencia de que la prioridad de escritores actuo

    std::cout << "[OK] Todas las pruebas de ControlAtomicoRW pasaron.\n";
    return 0;
}