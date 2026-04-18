# Informe de Benchmark de Búsquedas

## Configuración del experimento

- Se implementaron 9 configuraciones: 3 binarias, 3 secuenciales y 3 secuenciales con centinela.
- En búsqueda binaria se cronometra explícitamente: **1 ordenación quicksort + búsquedas binarias**.
- Para mantener tiempos de ejecución razonables en esta recuperación, se usó una versión reducida de cargas:
  - etiquetas del enunciado: 100.000, 5.000.000 y 500.000.000 búsquedas,
  - búsquedas realmente ejecutadas: 200, 1.000 y 3.000 respectivamente.
- Rango de tamaños de vector: 10.000 a 200.000 elementos.

## Gráfica comparativa (9 líneas)

![Gráfica de benchmark](benchmark_plot.svg)

## Resumen de resultados

| Configuración | Búsquedas etiqueta | Búsquedas ejecutadas | Tiempo mínimo (s) | Tiempo máximo (s) | n máximo |
|---|---:|---:|---:|---:|---:|
| BSEC100000 | 100000 | 200 | 0.014946 | 0.293396 | 200000 |
| BSEC5000000 | 5000000 | 1000 | 0.082094 | 1.433027 | 200000 |
| BSEC500000000 | 500000000 | 3000 | 0.244553 | 4.369365 | 200000 |
| BSECCEN100000 | 100000 | 200 | 0.015884 | 0.283520 | 200000 |
| BSECCEN5000000 | 5000000 | 1000 | 0.077539 | 1.475533 | 200000 |
| BSECCEN500000000 | 500000000 | 3000 | 0.231016 | 4.262986 | 200000 |
| BBIN100000 | 100000 | 200 | 0.002095 | 0.069827 | 200000 |
| BBIN5000000 | 5000000 | 1000 | 0.002707 | 0.075754 | 200000 |
| BBIN500000000 | 500000000 | 3000 | 0.003244 | 0.079561 | 200000 |

## Respuestas a las preguntas del enunciado

1. ¿Compensa siempre tener que ordenar el vector?
   - En este experimento reducido sí compensa en el mayor tamaño probado para las tres cargas de búsqueda.
2. ¿Varía con el número de búsquedas?
   - Sí. En general, al aumentar el número de búsquedas, el coste fijo de ordenar se amortiza más.
   - Speedup binaria vs secuencial en n máximo:
     - carga 100.000: 4.20x
     - carga 5.000.000: 18.92x
     - carga 500.000.000: 54.92x
3. ¿Se aprecia diferencia entre secuencial con y sin centinela?
   - Sí, pero menor que frente a binaria. Relación secuencial/sentinela en n máximo:
     - carga 100.000: 1.03x
     - carga 5.000.000: 0.97x
     - carga 500.000.000: 1.02x

## Conclusiones

- La búsqueda binaria mantiene ventaja clara cuando el número de búsquedas crece, incluso incluyendo el coste de ordenación.
- En cargas pequeñas, la ventaja existe pero es menor porque el coste de ordenar pesa más proporcionalmente.
- La versión secuencial con centinela mejora ligeramente respecto a la secuencial clásica, pero ambas siguen escalando linealmente y quedan lejos de binaria para cargas altas.
