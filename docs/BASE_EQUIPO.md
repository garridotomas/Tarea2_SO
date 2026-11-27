# Guia para comenzar

## Estado de la base

Estan definidos los tipos comunes, las clases y sus metodos.
Faltan los algoritmos, argumentos, hebras, mediciones y verificador.
Los metodos pendientes lanzan un error explicito.
Esta base no demuestra correccion.

## Acuerdos ya definidos

- No cambiar las cuatro firmas de ControlRW.
- No duplicar los tipos de tipos.h.
- No volver a definir las variables de comunes.cpp.
- MonitorRW recibe politica y un bool para activar la traza.
- obtener_traza() devuelve una copia bajo el mutex.
- verificar_traza() devuelve ResultadoVerificacion.
- indice_error comienza en cero y solo se interpreta si hay error.
- Cada hebra asigna un id_hebra unico al comenzar.
- Integrante 2 incrementa sus contadores intentos_hebra.
- Integrante 3 los copia antes de que termine cada hebra.
- Cada copia va a una casilla exclusiva; se suman tras los join().
- Los diagnosticos atomicos son distintos del estado de admision.

## Primera comprobacion

En Linux o WSL:
    make
    ./rw

El programa inicial solo imprime un mensaje y retorna 2.
Los algoritmos pendientes no deben usarse como controles reales.

## Recordatorios

Monitor: no mantener su mutex durante el trabajo sobre el registro.
Atomicos: una palabra y CAS para admitir; prioridad de escritores.
FIFO: turnos explicitos y lectoras consecutivas concurrentes.
Principal: validar overflow antes de multiplicar y gestionar join.
Verificador: comprobar reglas y contar evidencia de R6.
Experimentos: E1 y E2 segun el PDF; no inventar resultados.
