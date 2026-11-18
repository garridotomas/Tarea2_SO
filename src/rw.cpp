#include "tipos.h"

#include <iostream>
#include <limits>
#include <string>

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

    (void)politica;

    std::cout << "Configuracion valida\n";
    std::cout << "Modo: " << config.modo << "\n";
    std::cout << "Politica: "
              << config.politica_texto << "\n";
    std::cout << "Lectoras: "
              << config.L << "\n";
    std::cout << "Escritoras: "
              << config.E << "\n";
    std::cout << "K: "
              << config.K << "\n";
    std::cout << "Traza: "
              << (config.traza ? "SI" : "NO")
              << "\n";

    return 0;
}