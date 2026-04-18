#!/usr/bin/env python3
import argparse
import csv
import os
from statistics import mean


def read_rows(path):
    rows = []
    with open(path, "r", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            rows.append(
                {
                    "n": int(row["n"]),
                    "searches_requested": int(row.get("searches_requested", "0")),
                    "searches_executed": int(row.get("searches_executed", "0")),
                    "time_seconds": float(row["time_seconds"]),
                    "status": row.get("status", "OK"),
                }
            )
    return rows


def summarize(rows):
    ok_rows = [r for r in rows if r["status"] == "OK"]
    timeout_rows = [r for r in rows if r["status"] == "TIMEOUT"]
    alloc_rows = [r for r in rows if r["status"] == "ALLOC_FAIL"]

    if ok_rows:
        avg_time = mean(r["time_seconds"] for r in ok_rows)
        max_n = max(r["n"] for r in ok_rows)
    else:
        avg_time = 0.0
        max_n = 0

    return {
        "ok_count": len(ok_rows),
        "timeout_count": len(timeout_rows),
        "alloc_count": len(alloc_rows),
        "avg_time": avg_time,
        "max_n_ok": max_n,
    }


def complexity_table_md():
    return """| Algoritmo | CPU (promedio) | Memoria extra |
|---|---|---|
| Búsqueda secuencial | O(n) | O(1) |
| Búsqueda secuencial con centinela | O(n) | O(1) |
| Búsqueda binaria (vector ordenado) | O(log n) búsqueda, O(n log n) ordenación previa | O(1) in-place (quicksort promedio) |
"""


def build_markdown(results_dir, plot_path, seq_sum, cent_sum, bin_sum):
    _ = results_dir
    plot_name = os.path.basename(plot_path)

    return f"""# Informe de Benchmark de Búsquedas

## Configuración

- Medición temporal: `timespec_get()` con precisión de nanosegundos.
- Tamaño máximo objetivo del vector: 50.000.000 elementos.
- Búsquedas máximas por ejecución: 1.000.000.
- Timeout automático por ejecución: 30 segundos.

## Gráfica comparativa

![Gráfica de benchmark]({plot_name})

## Resumen observado

| Algoritmo | Ejecuciones OK | Timeouts | Fallos de memoria | Tiempo medio (s) | n máximo completado |
|---|---:|---:|---:|---:|---:|
| Secuencial | {seq_sum['ok_count']} | {seq_sum['timeout_count']} | {seq_sum['alloc_count']} | {seq_sum['avg_time']:.6f} | {seq_sum['max_n_ok']} |
| Secuencial + Centinela | {cent_sum['ok_count']} | {cent_sum['timeout_count']} | {cent_sum['alloc_count']} | {cent_sum['avg_time']:.6f} | {cent_sum['max_n_ok']} |
| Binaria | {bin_sum['ok_count']} | {bin_sum['timeout_count']} | {bin_sum['alloc_count']} | {bin_sum['avg_time']:.6f} | {bin_sum['max_n_ok']} |

## Complejidad general (CPU y memoria)

{complexity_table_md()}

## Conclusiones

1. La búsqueda binaria escala mejor en CPU para tamaños grandes, siempre que el vector esté previamente ordenado.
2. La secuencial con centinela mantiene complejidad O(n), pero reduce comprobaciones en el bucle interno y suele mejorar ligeramente tiempos.
3. En memoria extra, los tres métodos de búsqueda son O(1); el coste principal de memoria en este proyecto proviene del almacenamiento del propio vector de `BigDecimal`.
4. El timeout de 30 s evita ejecuciones descontroladas y permite comparar algoritmos de forma reproducible en hardware actual.
"""


def main():
    parser = argparse.ArgumentParser(description="Genera informe Markdown de benchmark")
    parser.add_argument("--results", default="results", help="Directorio de CSV")
    parser.add_argument("--plot", default=os.path.join("results", "benchmark_plot.svg"), help="Ruta de imagen")
    parser.add_argument("--output", default=os.path.join("results", "benchmark_report.md"), help="Ruta del informe MD")
    args = parser.parse_args()

    seq_rows = read_rows(os.path.join(args.results, "seq.csv"))
    cent_rows = read_rows(os.path.join(args.results, "seq_cent.csv"))
    bin_rows = read_rows(os.path.join(args.results, "bin.csv"))

    content = build_markdown(
        args.results,
        args.plot,
        summarize(seq_rows),
        summarize(cent_rows),
        summarize(bin_rows),
    )

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as fh:
        fh.write(content)

    print(f"Informe generado en: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
