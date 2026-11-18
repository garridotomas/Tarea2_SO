#include "control_atomico_rw.h"
#include "monitor_rw.h"
#include "tipos.h"

#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
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

int main(int argc, char *argv[]){
    Config config;

    if(!leer_argumentos(argc, argv, config)){
        return 1;
    }

    Politica politica=
        convertir_politica(config.politica_texto);

    std::unique_ptr<ControlRW> control;

    if(config.modo== "cond"){
        control= std::make_unique<MonitorRW>(
            politica,
            config.traza
        );
    }
    else{
        control= std::make_unique<ControlAtomicoRW>();
    }

    // Un unico registro compartido.
    Registro registro;

    // Aqui se guardan todas las hebras creadas.
    std::vector<std::thread> hebras;
    hebras.reserve(config.L + config.E);

    bool error_creacion= false;

    // -------------------------
    // HEBRAS LECTORAS
    // -------------------------

    for(int i= 0; i< config.L; i++){
        try{
            hebras.emplace_back([&, i](){

                id_hebra= i;

                for(long k= 0; k< config.K; k++){

                    control->entrar_lectura();

                    // El acceso al registro ocurre
                    // dentro del intervalo admitido.
                    long version= registro.version;
                    long doble_version=
                        registro.doble_version;

                    // Por ahora solo realizamos la lectura.
                    (void)version;
                    (void)doble_version;

                    if(config.dur_lec_us> 0){
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(
                                config.dur_lec_us
                            )
                        );
                    }

                    control->salir_lectura();
                }
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

            int id= config.L + i;

            try{
                hebras.emplace_back([&, id](){

                    id_hebra= id;

                    for(long k= 0; k< config.K; k++){

                        control->entrar_escritura();

                        // La escritora modifica el
                        // registro compartido.
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

                        control->salir_escritura();
                    }
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

    // Esperar todas las hebras que alcanzaron a crearse.
    for(std::thread &hebra : hebras){
        if(hebra.joinable()){
            hebra.join();
        }
    }

    if(error_creacion){
        std::cerr
            << "La ejecucion termino por error "
            << "al crear hebras\n";

        return 1;
    }

    std::cout << "\nEjecucion terminada\n";

    std::cout
        << "Hebras lectoras: "
        << config.L
        << "\n";

    std::cout
        << "Hebras escritoras: "
        << config.E
        << "\n";

    std::cout
        << "Operaciones por hebra: "
        << config.K
        << "\n";

    std::cout
        << "Version final: "
        << registro.version
        << "\n";

    std::cout
        << "Doble version final: "
        << registro.doble_version
        << "\n";

    return 0;
}