#include "monitor_rw.h"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

void exigir(bool condicion, const char* mensaje) {
    if (!condicion) {
        throw std::runtime_error(mensaje);
    }
}

// Una barrera sencilla para iniciar varias hebras sin usar pausas.
class Barrera {
public:
    explicit Barrera(int cantidad) : faltan(cantidad) {}

    void esperar() {
        std::unique_lock<std::mutex> lock(m);
        if (--faltan == 0) {
            cambio.notify_all();
        } else {
            cambio.wait(lock, [this] { return faltan == 0; });
        }
    }

private:
    std::mutex m;
    std::condition_variable cambio;
    int faltan;
};

// Evita que un bloqueo del monitor deje las pruebas esperando para siempre.
class LimitePrueba {
public:
    LimitePrueba() : vigilancia([this] {
        std::unique_lock<std::mutex> lock(m);
        if (!cambio.wait_for(lock, std::chrono::seconds(30),
                             [this] { return terminado; })) {
            std::cerr << "ERROR: las pruebas no terminaron en 30 segundos\n";
            std::abort();
        }
    }) {}

    ~LimitePrueba() {
        {
            std::lock_guard<std::mutex> lock(m);
            terminado = true;
        }
        cambio.notify_all();
        vigilancia.join();
    }

private:
    std::mutex m;
    std::condition_variable cambio;
    bool terminado = false;
    std::thread vigilancia;
};

void probar_lectoras_concurrentes(Politica politica, bool traza) {
    MonitorRW monitor(politica, traza);
    Barrera dentro(2);
    auto leer = [&](int id) {
        id_hebra = id;
        monitor.entrar_lectura();
        // Ambas deben entrar antes de que cualquiera salga.
        dentro.esperar();
        monitor.salir_lectura();
    };
    std::thread primera(leer, 0);
    std::thread segunda(leer, 1);
    primera.join();
    segunda.join();
    exigir(monitor.obtener_traza().size() == (traza ? 6u : 0u),
           "Traza incorrecta en la prueba de lectoras concurrentes");
}

// Esta comprobacion es local a las pruebas; no reemplaza el verificador R6.
void revisar_traza(const std::vector<AnotacionTraza>& traza,
                   Politica politica, int hebras, int operaciones) {
    std::vector<int> estado(hebras, 0); // 0: fuera, 1: pendiente, 2: dentro.
    std::vector<int> terminadas(hebras, 0);
    int lectoras = 0;
    int lectoras_pendientes = 0;
    int escritoras_pendientes = 0;
    bool escritora = false;
    long evidencia = 0;

    exigir(traza.size() == static_cast<std::size_t>(hebras * operaciones * 3),
           "Faltan o sobran eventos en la traza");
    for (std::size_t i = 0; i < traza.size(); ++i) {
        const auto& evento = traza[i];
        exigir(evento.hebra >= 0 && evento.hebra < hebras, "ID fuera de rango");
        exigir(evento.turno == -1, "Turno inesperado fuera de FIFO");
        const bool lectura = evento.tipo == Tipo::Lectora;
        exigir(lectura == (evento.hebra < 4), "Tipo de hebra incorrecto");
        int& actual = estado[evento.hebra];
        int& pendientes = lectura ? lectoras_pendientes : escritoras_pendientes;

        if (evento.accion == Accion::Pide) {
            exigir(actual == 0, "Solicitud duplicada");
            actual = 1;
            ++pendientes;
            if (lectura && politica == Politica::Lectores && !escritora) {
                exigir(i + 1 < traza.size()
                           && traza[i + 1].hebra == evento.hebra
                           && traza[i + 1].accion == Accion::Entra,
                       "Una lectora habilitada no entro inmediatamente");
                if (escritoras_pendientes > 0) ++evidencia;
            }
        } else if (evento.accion == Accion::Entra) {
            exigir(actual == 1, "Entrada sin solicitud");
            exigir(!escritora, "Entrada durante una escritura activa");
            if (lectura) {
                if (politica == Politica::Escritores) {
                    exigir(escritoras_pendientes == 0,
                           "Una lectora adelanto a una escritora pendiente");
                }
                ++lectoras;
            } else {
                exigir(lectoras == 0, "Escritora junto a lectoras activas");
                if (politica == Politica::Lectores) {
                    exigir(lectoras_pendientes == 0,
                           "Una escritora adelanto a una lectora pendiente");
                } else if (lectoras_pendientes > 0) {
                    ++evidencia;
                }
                escritora = true;
            }
            actual = 2;
            --pendientes;
        } else {
            exigir(evento.accion == Accion::Sale && actual == 2,
                   "Salida sin entrada");
            if (lectura) --lectoras;
            else escritora = false;
            actual = 0;
            ++terminadas[evento.hebra];
        }
    }
    exigir(lectoras == 0 && !escritora && lectoras_pendientes == 0
               && escritoras_pendientes == 0,
           "Quedaron solicitudes sin terminar");
    for (int cantidad : terminadas) {
        exigir(cantidad == operaciones, "Una hebra no termino sus operaciones");
    }
    std::cout << "  Eventos: " << traza.size()
              << "; casos de prioridad observados: " << evidencia << '\n';
}

void probar_carga(Politica politica, bool traza) {
    constexpr int lectoras = 4;
    constexpr int escritoras = 3;
    constexpr int operaciones = 200;
    constexpr int cantidad = lectoras + escritoras;
    MonitorRW monitor(politica, traza);
    Registro registro;
    Barrera inicio(cantidad);
    std::vector<std::thread> hebras;
    // Cada hebra escribe solamente en su propia casilla.
    std::vector<int> errores(cantidad, 0);
    for (int id = 0; id < cantidad; ++id) {
        hebras.emplace_back([&, id] {
            id_hebra = id;
            inicio.esperar();
            for (int k = 0; k < operaciones; ++k) {
                if (id < lectoras) {
                    monitor.entrar_lectura();
                    if (registro.doble_version != 2 * registro.version) {
                        ++errores[id];
                    }
                    monitor.salir_lectura();
                } else {
                    monitor.entrar_escritura();
                    ++registro.version;
                    registro.doble_version = 2 * registro.version;
                    monitor.salir_escritura();
                }
            }
        });
    }
    for (auto& hebra : hebras) hebra.join();
    exigir(registro.version == escritoras * operaciones,
           "Se perdieron escrituras");
    exigir(registro.doble_version == 2 * registro.version,
           "Registro final inconsistente");
    for (int cantidad_errores : errores) {
        exigir(cantidad_errores == 0, "Una lectora vio un registro inconsistente");
    }
    auto copia = monitor.obtener_traza();
    if (traza) {
        revisar_traza(copia, politica, cantidad, operaciones);
        copia.clear();
        exigir(!monitor.obtener_traza().empty(), "La traza no es una copia");
    } else {
        exigir(copia.empty(), "Se registraron eventos con la traza desactivada");
    }
}

int main() {
    LimitePrueba limite;
    try {
        for (auto politica : {Politica::Lectores, Politica::Escritores}) {
            std::cout << (politica == Politica::Lectores ? "Lectores" : "Escritores")
                      << '\n';
            for (bool traza : {true, false}) {
                probar_lectoras_concurrentes(politica, traza);
                probar_carga(politica, traza);
            }
        }
        bool fifo_pendiente = false;
        try {
            MonitorRW fifo(Politica::Fifo);
        } catch (const std::logic_error&) {
            fifo_pendiente = true;
        }
        exigir(fifo_pendiente, "FIFO debe informar que esta pendiente");
        std::cout << "OK: pruebas del monitor completadas\n";
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
