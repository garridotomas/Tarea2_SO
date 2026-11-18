# Tarea2_SO

Tarea 2 de Sistemas Operativos: lectores y escritores con monitores y operaciones atómicas en C++17.

## Acuerdos de trabajo

1. Definir primero `ControlRW`, los tipos de traza, `id_hebra` y la interfaz de mediciones y contadores locales.
2. Mantener el mismo registro y trabajo de las hebras para ambas implementaciones.
3. Usar una rama por componente y revisar los cambios mediante pull requests.
4. Integrar avances pequeños; no esperar al final para juntar los componentes.
5. Cada integrante escribe su sección del informe y revisa otra parte del programa.

Ramas sugeridas: `feat/monitor`, `feat/atomicos`, `feat/ejecucion` y `feat/verificacion`.

## Compilación y ejecución

En Linux o WSL, compilar con `make`. El principal actual valida los argumentos y
crea el control y el registro; la ejecucion de hebras sigue pendiente.

Para probar por separado el monitor de prioridad de lectores y escritores:

```sh
make test-monitor
```

Ver [docs/MONITOR.md](docs/MONITOR.md) para las condiciones de entrada, las trazas
y la integracion pendiente de FIFO.

## Entrega

- Ejecutable `rw` generado mediante `make`.
- Código fuente y `readme.txt` con instrucciones reproducibles.
- Informe PDF de 8 páginas con enlace al repositorio.
- Presentación de 10 minutos más preguntas, con participación de todos.

Fecha de entrega: 2 de diciembre de 2026.
