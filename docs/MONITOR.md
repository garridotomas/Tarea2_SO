# Monitor: prioridad de lectores y escritores

Esta parte implementa R3, R4 y la generacion de trazas de R6 en
`include/monitor_rw.h` y `src/monitor_rw.cpp`.

## Estado y condiciones de entrada

El mutex `m` protege los contadores, la bandera de escritura y la traza.
`lectoras_activas` cuenta permisos de lectura que todavia no se devuelven.
Los contadores `lectoras_esperando` y `escritoras_esperando` cuentan solicitudes
registradas que aun no entran. `escritora_activa` indica un permiso de escritura.

| Politica | Puede entrar una lectora si... | Puede entrar una escritora si... |
|---|---|---|
| Lectores | No hay escritora activa. | No hay escritora activa, lectoras activas ni lectoras esperando. |
| Escritores | No hay escritora activa ni escritoras esperando. | No hay escritora activa ni lectoras activas. |

Las esperas usan `condition_variable::wait` con predicado, sin espera activa.
Las salidas notifican a todas las hebras cuando pueden habilitar nuevas entradas.
El mutex se libera al retornar de `entrar_*`: el trabajo sobre el registro ocurre
fuera del mutex, con el permiso vigente hasta `salir_*`. Esto permite lectoras
concurrentes. Estas politicas pueden postergar al tipo de hebra sin prioridad;
no garantizan FIFO ni ausencia de inanicion.

## Uso desde las hebras

Todas las hebras usan el mismo objeto. Cada una asigna primero un `id_hebra`
unico y empareja cada `entrar_lectura()` con `salir_lectura()`, o cada
`entrar_escritura()` con `salir_escritura()`, desde esa misma hebra.
El trabajo sobre el registro va entre ambas llamadas. No se admiten entradas
anidadas ni convertir un permiso de lectura a escritura.

Los eventos `Pide`, `Entra` y `Sale` se agregan bajo `m`, usando `id_hebra`
y turno `-1`. Una lectora habilitada en prioridad de lectores registra `Pide`
y `Entra` consecutivos. `obtener_traza()` devuelve una copia protegida por `m`;
el principal debe obtener la traza completa despues de todos los `join()`.
Desactivar la traza no cambia las condiciones de admision.

## Pruebas independientes

### Includes en VS Code

La configuracion local `.vscode/c_cpp_properties.json` indica a IntelliSense
la carpeta `include/` y C++17. En Windows usa las cabeceras del compilador portatil
de `build/toolchain/ziglang/` y el modo compatible con MinGW. Se desactiva la
consulta automatica del compilador para evitar que el editor use por error
cabeceras de Microsoft. Esto solo afecta al analisis del editor.
La configuracion y el compilador son locales y no se suben al repositorio;
quien use otro compilador debe configurar `compilerPath` y sus propias rutas.
En Linux o WSL la configuracion `Linux` usa `/usr/bin/g++`.
Abrir la carpeta `Tarea2_SO` para usar su configuracion local.
Si VS Code conserva los avisos anteriores, ejecutar `C/C++: Reset IntelliSense Database`
desde la paleta de comandos.

### Ejecucion

En Linux o WSL, desde la raiz del repositorio:

```sh
make test-monitor
```

No requiere el principal, el control atomico ni el verificador pendientes.
Las pruebas comprueban lectoras simultaneas, operaciones concurrentes sobre el
registro, orden y exclusividad en la traza, reglas de ambas prioridades,
identificadores, turnos y funcionamiento sin traza. La concurrencia se coordina
con variables de condicion, sin pausas. Un limite de 30 segundos detecta bloqueos.
La cantidad de casos de prioridad observados depende de la planificacion;
las pruebas de carga no constituyen una demostracion formal de correccion.

## Integracion pendiente

- FIFO corresponde a otro integrante. Por ahora, construir el monitor con
  `Politica::Fifo` lanza un error explicito. Al agregar esa politica hay que
  reemplazar esa comprobacion, incorporar sus turnos y condiciones, y actualizar
  la prueba que comprueba este error. No basta con retirar la comprobacion.
- El principal debe crear las hebras, asignar sus identificadores, operar el
  registro y recoger la traza despues de los `join()`.
- El verificador R6 se desarrolla por separado. La comprobacion de trazas en
  `tests/prueba_monitor.cpp` solo sirve como prueba de este componente.
