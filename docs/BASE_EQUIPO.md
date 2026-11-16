# Guia para comenzar

## Estado de la base

Estan definidos los tipos comunes, las clases y sus metodos.
Faltan los algoritmos, argumentos, hebras, mediciones y verificador.
Los metodos pendientes lanzan un error explicito.
Esta base no demuestra correccion.

## Reparto

| Integrante | Archivos principales | Trabajo |
| --- | --- | --- |
| 1 | monitor_rw.h y monitor_rw.cpp | Monitor, tres politicas y traza. |
| 2 | control_atomico_rw.h y control_atomico_rw.cpp | Palabra atomica, CAS y reintentos. |
| 3 | rw.cpp, Makefile y readme.txt | Argumentos, hebras, registro, diagnostico y mediciones. |
| 4 | verificador.h, verificador.cpp, tests/ y scripts/ | Verificador, pruebas y experimentos. |

Las cabeceras estan en include/ y las implementaciones en src/.

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

## Como empezar sin esperar a los otros

1. Integrante 1 implementa el monitor y genera la traza.
2. Integrante 2 implementa el protocolo atomico.
3. Integrante 3 prepara argumentos, hebras y mediciones usando
   ControlRW. Ejecutar carga requiere un control implementado.
4. Integrante 4 prepara vectores de traza manuales, correctos
   e incorrectos, para desarrollar el verificador.

## Primera comprobacion

En Linux o WSL:
    make
    ./rw

El programa inicial solo imprime un mensaje y retorna 2.
Los algoritmos pendientes no deben usarse como controles reales.

Las pruebas con main propio deben compilarse por separado.
Cada integrante escribe tambien su seccion del informe.

## Recordatorios

Monitor: no mantener su mutex durante el trabajo sobre el registro.
Atomicos: una palabra y CAS para admitir; prioridad de escritores.
FIFO: turnos explicitos y lectoras consecutivas concurrentes.
Principal: validar overflow antes de multiplicar y gestionar join.
Verificador: comprobar reglas y contar evidencia de R6.
Experimentos: E1 y E2 segun el PDF; no inventar resultados.
