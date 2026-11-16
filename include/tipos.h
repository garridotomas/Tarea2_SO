#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

enum class Politica { Lectores, Escritores, Fifo };
enum class Tipo { Lectora, Escritora };
enum class Accion { Pide, Entra, Sale };

struct AnotacionTraza {
    int hebra;
    Tipo tipo;
    Accion accion;
    long turno; // -1 cuando no se usa FIFO.
};

struct Registro {
    long version = 0;
    long doble_version = 0;
};

// Cada hebra tiene su propia copia.
extern thread_local int id_hebra;

struct IntentosFallidos {
    std::uint64_t recurso_ocupado = 0;
    std::uint64_t escritoras_esperando = 0;
    std::uint64_t cas_fallido = 0;
};

extern thread_local IntentosFallidos intentos_hebra;

struct ResultadoVerificacion {
    bool politica_ok = false;
    long evidencia = 0;
    std::size_t indice_error = 0; // Contado desde cero.
    std::string detalle;
};
