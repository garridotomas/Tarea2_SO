# Resultados experimentales

## E1: corrección

Guardar en `e1/` los resultados de las cuatro variantes: monitor con lectores,
escritores y FIFO, y versión atómica con escritores. Ejecutar las cinco
configuraciones del enunciado al menos cinco veces por variante: 100 ejecuciones.
Conservar invariantes, verificación de políticas, evidencia y reintentos según modo.

## E2: rendimiento

Guardar en `e2/` las mediciones de prioridad de escritores en modos cond y atomic,
con `notraza`. Ejecutar las cinco configuraciones cinco veces por modo: 50 ejecuciones.
En la configuración con 8 lectoras, 8 escritoras y duraciones cero, usar K=10000.
Conservar datos individuales y promedios; indicar equipo y entorno utilizados.

Los fragmentos seleccionados de traza para el informe van en `docs/trazas/`.
Preferir CSV o texto para los datos; los registros temporales `.log` se ignoran.
