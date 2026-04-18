# AlgoritmosBusqueda (BigDecimal + benchmark con timespec_get)

Proyecto actualizado para comparar algoritmos de búsqueda con:

- Medición de tiempo con `timespec_get()` (nanosegundos).
- Vectores de prueba de hasta 50.000.000 de elementos.
- Hasta 1.000.000 de búsquedas por ejecución.
- Timeout automático de 30 segundos por ejecución.

## Nombres de archivos (estándar snake_case)

- `big_decimal.c`, `big_decimal.h`
- `dynamic_vector.c`, `dynamic_vector.h`
- `search.c`, `search.h`
- `quick_sort.c`, `quick_sort.h`
- `tests/test_suite.c`

## Ejecutar tests

Linux/WSL:

```bash
make check
```

Windows PowerShell:

```powershell
./run_tests.ps1
```

## Ejecutar benchmark

Linux/WSL:

```bash
make benchmark
```

Salida CSV (en `results/`):

- `seq.csv`
- `seq_cent.csv`
- `bin.csv`

Formato de cada CSV:

`n,searches_requested,searches_executed,time_seconds,status`

Estados posibles:

- `OK`
- `TIMEOUT`
- `ALLOC_FAIL`

## Gráfica e informe comparativo en Markdown

No requiere dependencias externas de Python.

Acceso directo al informe:

- [INFORME.md](INFORME.md)

Vista rápida de la gráfica:

![Gráfica de benchmark](results/benchmark_plot.svg)

Generar gráfica e informe desde CSV existentes:

```bash
python3 plot_benchmarks.py
```

Ejecutar benchmark + gráfica + informe en un solo paso:

```bash
python3 plot_benchmarks.py --run-cmd "./build/bench/benchmark"
```

Archivos generados:

- `results/benchmark_plot.svg`
- `results/benchmark_report.md`

## Complejidad teórica

- Búsqueda secuencial: CPU `O(n)`, memoria extra `O(1)`.
- Búsqueda secuencial con centinela: CPU `O(n)`, memoria extra `O(1)`.
- Búsqueda binaria: CPU `O(log n)` para buscar (si ya está ordenado), memoria extra `O(1)` en la búsqueda.
