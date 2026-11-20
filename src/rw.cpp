#include "control_atomico_rw.h"
#include "monitor_rw.h"
#include "tipos.h"
#include "verificador.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <time.h>
#include <vector>

struct Config{
    std::string modo;
    std::string politica_texto;

    int L= 0;
    int E= 0;
    long K= 0;

    long dur_lec_us= 0;
    long dur_esc_us= 0;

    bool traza= true;
};

struct MedicionHebra{
    long long espera_total_ns= 0;
    long long espera_max_ns= 0;

    IntentosFallidos intentos{};
};

bool convertir_long(const char *texto, long &valor){
    try{
        std::string s(texto);
        size_t pos= 0;

        long resultado= std::stol(s, &pos);

        if(pos!= s.size()){
            return false;
        }

        valor= resultado;
        return true;
    }
    catch(...){
        return false;
    }
}

bool leer_argumentos(int argc, char *argv[], Config &config){
    if(argc!= 8 && argc!= 9){
        std::cerr
            << "Uso: ./rw <modo> <politica> <L> <E> <K> "
            << "<dur_lec_us> <dur_esc_us> [notraza]\n";

        return false;
    }

    config.modo= argv[1];
    config.politica_texto= argv[2];

    if(config.modo!= "cond" &&
       config.modo!= "atomic"){
        std::cerr
            << "Error: modo debe ser cond o atomic\n";

        return false;
    }

    if(config.politica_texto!= "lectores" &&
       config.politica_texto!= "escritores" &&
       config.politica_texto!= "fifo"){
        std::cerr
            << "Error: politica invalida\n";

        return false;
    }

    if(config.modo== "atomic" &&
       config.politica_texto!= "escritores"){
        std::cerr
            << "Error: modo atomic solo funciona "
            << "con politica escritores\n";

        return false;
    }

    long L;
    long E;

    if(!convertir_long(argv[3], L) ||
       !convertir_long(argv[4], E) ||
       !convertir_long(argv[5], config.K) ||
       !convertir_long(argv[6], config.dur_lec_us) ||
       !convertir_long(argv[7], config.dur_esc_us)){

        std::cerr
            << "Error: parametros numericos invalidos\n";

        return false;
    }

    if(L< 1 || E< 1 || config.K< 1){
        std::cerr
            << "Error: L, E y K deben ser >= 1\n";

        return false;
    }

    if(L + E> 1000){
        std::cerr
            << "Error: L + E no puede superar 1000\n";

        return false;
    }

    if(config.dur_lec_us< 0 ||
       config.dur_esc_us< 0){
        std::cerr
            << "Error: las duraciones deben ser >= 0\n";

        return false;
    }

    if(L> std::numeric_limits<int>::max() ||
       E> std::numeric_limits<int>::max()){
        std::cerr
            << "Error: cantidad de hebras fuera de rango\n";

        return false;
    }

    config.L= static_cast<int>(L);
    config.E= static_cast<int>(E);

    if(argc== 9){
        if(std::string(argv[8])!= "notraza"){
            std::cerr
                << "Error: parametro opcional debe ser notraza\n";

            return false;
        }

        config.traza= false;
    }

    return true;
}

Politica convertir_politica(const std::string &texto){
    if(texto== "lectores"){
        return Politica::Lectores;
    }

    if(texto== "escritores"){
        return Politica::Escritores;
    }

    return Politica::Fifo;
}

double diferencia_cpu(
    const timespec &inicio,
    const timespec &fin
){
    double segundos=
        static_cast<double>(fin.tv_sec - inicio.tv_sec);

    double nanosegundos=
        static_cast<double>(fin.tv_nsec - inicio.tv_nsec)
        / 1000000000.0;

    return segundos + nanosegundos;
}

int main(int argc, char *argv[]){
    Config config;

    if(!leer_argumentos(argc, argv, config)){
        return 1;
    }

    Politica politica=
        convertir_politica(config.politica_texto);

    std::unique_ptr<ControlRW> control;

    // Se guarda este puntero para obtener la traza
    // cuando se utiliza la implementacion cond.
    MonitorRW *monitor= nullptr;

    if(config.modo== "cond"){
        auto nuevo_monitor=
            std::make_unique<MonitorRW>(
                politica,
                config.traza
            );

        monitor= nuevo_monitor.get();
        control= std::move(nuevo_monitor);
    }
    else{
        control=
            std::make_unique<ControlAtomicoRW>();
    }

    Registro registro;

    std::atomic<int> lectores_dentro{0};
    std::atomic<int> escritores_dentro{0};
    std::atomic<int> max_lectores{0};

    std::atomic<long> inconsistencias{0};
    std::atomic<long> violaciones_exclusion{0};

    int total_hebras=
        config.L + config.E;

    std::vector<MedicionHebra> mediciones(
        total_hebras
    );

    std::vector<std::thread> hebras;
    hebras.reserve(total_hebras);

    bool error_creacion= false;

    auto pared_inicio=
        std::chrono::steady_clock::now();

    timespec cpu_inicio{};
    timespec cpu_fin{};

    if(clock_gettime(
        CLOCK_PROCESS_CPUTIME_ID,
        &cpu_inicio
    )== -1){
        std::cerr
            << "Error al medir tiempo de CPU\n";

        return 1;
    }

    // -------------------------
    // HEBRAS LECTORAS
    // -------------------------

    for(int i= 0; i< config.L; i++){
        try{
            hebras.emplace_back([&, i](){

                id_hebra= i;
                intentos_hebra= {};

                for(long k= 0; k< config.K; k++){

                    auto inicio_espera=
                        std::chrono::steady_clock::now();

                    control->entrar_lectura();

                    auto fin_espera=
                        std::chrono::steady_clock::now();

                    long long espera_ns=
                        std::chrono::duration_cast<
                            std::chrono::nanoseconds
                        >(
                            fin_espera - inicio_espera
                        ).count();

                    mediciones[i].espera_total_ns+=
                        espera_ns;

                    if(espera_ns>
                       mediciones[i].espera_max_ns){

                        mediciones[i].espera_max_ns=
                            espera_ns;
                    }

                    int lectores_actuales=
                        lectores_dentro.fetch_add(1) + 1;

                    int max_actual=
                        max_lectores.load();

                    while(
                        lectores_actuales> max_actual &&
                        !max_lectores.compare_exchange_weak(
                            max_actual,
                            lectores_actuales
                        )
                    ){
                    }

                    if(escritores_dentro.load()!= 0){
                        violaciones_exclusion.fetch_add(1);
                    }

                    long version=
                        registro.version;

                    long doble_version=
                        registro.doble_version;

                    if(doble_version!= 2 * version){
                        inconsistencias.fetch_add(1);
                    }

                    if(config.dur_lec_us> 0){
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(
                                config.dur_lec_us
                            )
                        );
                    }

                    lectores_dentro.fetch_sub(1);

                    control->salir_lectura();
                }

                mediciones[i].intentos=
                    intentos_hebra;
            });
        }
        catch(const std::system_error &e){
            std::cerr
                << "Error creando hebra lectora: "
                << e.what()
                << "\n";

            error_creacion= true;
            break;
        }
    }

    // -------------------------
    // HEBRAS ESCRITORAS
    // -------------------------

    if(!error_creacion){
        for(int i= 0; i< config.E; i++){

            int id=
                config.L + i;

            try{
                hebras.emplace_back([&, id](){

                    id_hebra= id;
                    intentos_hebra= {};

                    for(long k= 0; k< config.K; k++){

                        auto inicio_espera=
                            std::chrono::steady_clock::now();

                        control->entrar_escritura();

                        auto fin_espera=
                            std::chrono::steady_clock::now();

                        long long espera_ns=
                            std::chrono::duration_cast<
                                std::chrono::nanoseconds
                            >(
                                fin_espera - inicio_espera
                            ).count();

                        mediciones[id].espera_total_ns+=
                            espera_ns;

                        if(espera_ns>
                           mediciones[id].espera_max_ns){

                            mediciones[id].espera_max_ns=
                                espera_ns;
                        }

                        int escritoras_previas=
                            escritores_dentro.fetch_add(1);

                        if(
                            escritoras_previas!= 0 ||
                            lectores_dentro.load()!= 0
                        ){
                            violaciones_exclusion.fetch_add(1);
                        }

                        registro.version++;

                        registro.doble_version=
                            2 * registro.version;

                        if(config.dur_esc_us> 0){
                            std::this_thread::sleep_for(
                                std::chrono::microseconds(
                                    config.dur_esc_us
                                )
                            );
                        }

                        escritores_dentro.fetch_sub(1);

                        control->salir_escritura();
                    }

                    mediciones[id].intentos=
                        intentos_hebra;
                });
            }
            catch(const std::system_error &e){
                std::cerr
                    << "Error creando hebra escritora: "
                    << e.what()
                    << "\n";

                error_creacion= true;
                break;
            }
        }
    }

    for(std::thread &hebra : hebras){
        if(hebra.joinable()){
            hebra.join();
        }
    }

    auto pared_fin=
        std::chrono::steady_clock::now();

    if(clock_gettime(
        CLOCK_PROCESS_CPUTIME_ID,
        &cpu_fin
    )== -1){
        std::cerr
            << "Error al medir tiempo de CPU\n";

        return 1;
    }

    if(error_creacion){
        std::cerr
            << "La ejecucion termino por error "
            << "al crear hebras\n";

        return 1;
    }

    // -------------------------
    // INVARIANTES
    // -------------------------

    long version_esperada=
        static_cast<long>(config.E) * config.K;

    long doble_esperada=
        2 * version_esperada;

    bool registro_correcto=
        registro.version== version_esperada &&
        registro.doble_version== doble_esperada;

    bool exclusion_correcta=
        violaciones_exclusion.load()== 0;

    bool lecturas_correctas=
        inconsistencias.load()== 0;

    // -------------------------
    // MEDICIONES
    // -------------------------

    double tiempo_pared=
        std::chrono::duration<double>(
            pared_fin - pared_inicio
        ).count();

    double tiempo_cpu=
        diferencia_cpu(
            cpu_inicio,
            cpu_fin
        );

    long long total_espera_lectoras= 0;
    long long total_espera_escritoras= 0;

    long long max_espera_lectoras= 0;
    long long max_espera_escritoras= 0;

    for(int i= 0; i< config.L; i++){

        total_espera_lectoras+=
            mediciones[i].espera_total_ns;

        if(mediciones[i].espera_max_ns>
           max_espera_lectoras){

            max_espera_lectoras=
                mediciones[i].espera_max_ns;
        }
    }

    for(int i= config.L;
        i< total_hebras;
        i++){

        total_espera_escritoras+=
            mediciones[i].espera_total_ns;

        if(mediciones[i].espera_max_ns>
           max_espera_escritoras){

            max_espera_escritoras=
                mediciones[i].espera_max_ns;
        }
    }

    long operaciones_lectura=
        static_cast<long>(config.L) *
        config.K;

    long operaciones_escritura=
        static_cast<long>(config.E) *
        config.K;

    double promedio_espera_lectoras_us=
        static_cast<double>(
            total_espera_lectoras
        )
        / operaciones_lectura
        / 1000.0;

    double promedio_espera_escritoras_us=
        static_cast<double>(
            total_espera_escritoras
        )
        / operaciones_escritura
        / 1000.0;

    double max_espera_lectoras_us=
        static_cast<double>(
            max_espera_lectoras
        )
        / 1000.0;

    double max_espera_escritoras_us=
        static_cast<double>(
            max_espera_escritoras
        )
        / 1000.0;

    // -------------------------
    // INTENTOS FALLIDOS ATOMIC
    // -------------------------

    std::uint64_t fallos_recurso_ocupado= 0;
    std::uint64_t fallos_escritoras_esperando= 0;
    std::uint64_t fallos_cas= 0;

    for(const MedicionHebra &medicion : mediciones){

        fallos_recurso_ocupado+=
            medicion.intentos.recurso_ocupado;

        fallos_escritoras_esperando+=
            medicion.intentos.escritoras_esperando;

        fallos_cas+=
            medicion.intentos.cas_fallido;
    }

    // -------------------------
    // TRAZA Y VERIFICADOR
    // -------------------------

    bool politica_ok= true;
    long evidencia= 0;

    if(config.modo== "cond" &&
       config.traza &&
       monitor!= nullptr){

        try{
            std::vector<AnotacionTraza> traza=
                monitor->obtener_traza();

            ResultadoVerificacion resultado=
                verificar_traza(
                    traza,
                    politica
                );

            politica_ok=
                resultado.politica_ok;

            evidencia=
                resultado.evidencia;

            if(!politica_ok){
                std::cerr
                    << "Error de politica en anotacion "
                    << resultado.indice_error
                    << ": "
                    << resultado.detalle
                    << "\n";
            }
        }
        catch(const std::exception &e){
            std::cerr
                << "Error en verificacion de traza: "
                << e.what()
                << "\n";

            politica_ok= false;
        }
    }

    // -------------------------
    // RESULTADO GLOBAL
    // -------------------------

    bool correcto=
        registro_correcto &&
        exclusion_correcta &&
        lecturas_correctas &&
        politica_ok;

    // -------------------------
    // RESUMEN
    // -------------------------

    std::cout
        << "\n========== RESUMEN ==========\n";

    std::cout
        << "Modo: "
        << config.modo
        << "\n";

    std::cout
        << "Politica: "
        << config.politica_texto
        << "\n";

    std::cout
        << "Version final: "
        << registro.version
        << " / esperada: "
        << version_esperada
        << "\n";

    std::cout
        << "Doble version final: "
        << registro.doble_version
        << " / esperada: "
        << doble_esperada
        << "\n";

    std::cout
        << "Inconsistencias: "
        << inconsistencias.load()
        << "\n";

    std::cout
        << "Violaciones de exclusion: "
        << violaciones_exclusion.load()
        << "\n";

    std::cout
        << "Maximo lectoras simultaneas: "
        << max_lectores.load()
        << "\n";

    std::cout
        << std::fixed
        << std::setprecision(6);

    std::cout
        << "Tiempo pared (s): "
        << tiempo_pared
        << "\n";

    std::cout
        << "Tiempo CPU (s): "
        << tiempo_cpu
        << "\n";

    std::cout
        << "Espera promedio lectoras (us): "
        << promedio_espera_lectoras_us
        << "\n";

    std::cout
        << "Espera maxima lectoras (us): "
        << max_espera_lectoras_us
        << "\n";

    std::cout
        << "Espera promedio escritoras (us): "
        << promedio_espera_escritoras_us
        << "\n";

    std::cout
        << "Espera maxima escritoras (us): "
        << max_espera_escritoras_us
        << "\n";

    if(config.modo== "atomic"){

        std::cout
            << "Fallos recurso ocupado: "
            << fallos_recurso_ocupado
            << "\n";

        std::cout
            << "Fallos escritoras esperando: "
            << fallos_escritoras_esperando
            << "\n";

        std::cout
            << "Fallos CAS: "
            << fallos_cas
            << "\n";
    }

    if(config.modo== "cond" &&
       config.traza){

        std::cout
            << "Politica OK: "
            << (politica_ok ? "SI" : "NO")
            << "\n";

        std::cout
            << "Evidencia: "
            << evidencia
            << "\n";
    }

    std::cout
        << "Correcto: "
        << (correcto ? "SI" : "NO")
        << "\n";

    return correcto ? 0 : 1;
}