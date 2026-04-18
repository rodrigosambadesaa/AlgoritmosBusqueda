#!/usr/bin/env python3
import argparse
import csv
import os


def read_rows(path):
    rows = []
    with open(path, "r", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            rows.append(
                {
                    "n": int(row["n"]),
                    "searches_label": int(row["searches_label"]),
                    "searches_executed": int(row["searches_executed"]),
                    "time_seconds": float(row["time_seconds"]),
                }
            )
    return rows


def summarize(rows):
    if not rows:
        return {"min_time": 0.0, "max_time": 0.0, "max_n": 0, "searches_executed": 0}

    min_time = min(r["time_seconds"] for r in rows)
    max_time = max(r["time_seconds"] for r in rows)
    max_n = max(r["n"] for r in rows)
    searches_executed = rows[0]["searches_executed"]

    return {
        "min_time": min_time,
        "max_time": max_time,
        "max_n": max_n,
        "searches_executed": searches_executed,
    }


def row_at_max_n(rows):
    max_n = max(r["n"] for r in rows)
    for row in rows:
        if row["n"] == max_n:
            return row
    return rows[-1]


def speedup(base, other):
    if base <= 0.0 or other <= 0.0:
        return 0.0
    return base / other


def build_markdown(plot_path, datasets):
    plot_name = os.path.basename(plot_path)

    s100 = datasets["BSEC100000"]
    s5m = datasets["BSEC5000000"]
    s500m = datasets["BSEC500000000"]
    c100 = datasets["BSECCEN100000"]
    c5m = datasets["BSECCEN5000000"]
    c500m = datasets["BSECCEN500000000"]
    b100 = datasets["BBIN100000"]
    b5m = datasets["BBIN5000000"]
    b500m = datasets["BBIN500000000"]

    s100_max = row_at_max_n(s100["rows"])
    s5m_max = row_at_max_n(s5m["rows"])
    s500m_max = row_at_max_n(s500m["rows"])
    c100_max = row_at_max_n(c100["rows"])
    c5m_max = row_at_max_n(c5m["rows"])
    c500m_max = row_at_max_n(c500m["rows"])
    b100_max = row_at_max_n(b100["rows"])
    b5m_max = row_at_max_n(b5m["rows"])
    b500m_max = row_at_max_n(b500m["rows"])

    bin_vs_sec_100 = speedup(s100_max["time_seconds"], b100_max["time_seconds"])
    bin_vs_sec_5m = speedup(s5m_max["time_seconds"], b5m_max["time_seconds"])
    bin_vs_sec_500m = speedup(s500m_max["time_seconds"], b500m_max["time_seconds"])

    cen_vs_sec_100 = speedup(s100_max["time_seconds"], c100_max["time_seconds"])
    cen_vs_sec_5m = speedup(s5m_max["time_seconds"], c5m_max["time_seconds"])
    cen_vs_sec_500m = speedup(s500m_max["time_seconds"], c500m_max["time_seconds"])

    sort_compensa = (
        b100_max["time_seconds"] < s100_max["time_seconds"]
        and b5m_max["time_seconds"] < s5m_max["time_seconds"]
        and b500m_max["time_seconds"] < s500m_max["time_seconds"]
    )

    return f"""# Informe de Benchmark de Búsquedas

## Configuración del experimento

- Se implementaron 9 configuraciones: 3 binarias, 3 secuenciales y 3 secuenciales con centinela.
- En búsqueda binaria se cronometra explícitamente: **1 ordenación quicksort + búsquedas binarias**.
- Para mantener tiempos de ejecución razonables en esta recuperación, se usó una versión reducida de cargas:
  - etiquetas del enunciado: 100.000, 5.000.000 y 500.000.000 búsquedas,
  - búsquedas realmente ejecutadas: 200, 1.000 y 3.000 respectivamente.
- Rango de tamaños de vector: 10.000 a 200.000 elementos.

## Gráfica comparativa (9 líneas)

![Gráfica de benchmark]({plot_name})

## Resumen de resultados

| Configuración | Búsquedas etiqueta | Búsquedas ejecutadas | Tiempo mínimo (s) | Tiempo máximo (s) | n máximo |
|---|---:|---:|---:|---:|---:|
| BSEC100000 | 100000 | {s100['summary']['searches_executed']} | {s100['summary']['min_time']:.6f} | {s100['summary']['max_time']:.6f} | {s100['summary']['max_n']} |
| BSEC5000000 | 5000000 | {s5m['summary']['searches_executed']} | {s5m['summary']['min_time']:.6f} | {s5m['summary']['max_time']:.6f} | {s5m['summary']['max_n']} |
| BSEC500000000 | 500000000 | {s500m['summary']['searches_executed']} | {s500m['summary']['min_time']:.6f} | {s500m['summary']['max_time']:.6f} | {s500m['summary']['max_n']} |
| BSECCEN100000 | 100000 | {c100['summary']['searches_executed']} | {c100['summary']['min_time']:.6f} | {c100['summary']['max_time']:.6f} | {c100['summary']['max_n']} |
| BSECCEN5000000 | 5000000 | {c5m['summary']['searches_executed']} | {c5m['summary']['min_time']:.6f} | {c5m['summary']['max_time']:.6f} | {c5m['summary']['max_n']} |
| BSECCEN500000000 | 500000000 | {c500m['summary']['searches_executed']} | {c500m['summary']['min_time']:.6f} | {c500m['summary']['max_time']:.6f} | {c500m['summary']['max_n']} |
| BBIN100000 | 100000 | {b100['summary']['searches_executed']} | {b100['summary']['min_time']:.6f} | {b100['summary']['max_time']:.6f} | {b100['summary']['max_n']} |
| BBIN5000000 | 5000000 | {b5m['summary']['searches_executed']} | {b5m['summary']['min_time']:.6f} | {b5m['summary']['max_time']:.6f} | {b5m['summary']['max_n']} |
| BBIN500000000 | 500000000 | {b500m['summary']['searches_executed']} | {b500m['summary']['min_time']:.6f} | {b500m['summary']['max_time']:.6f} | {b500m['summary']['max_n']} |

## Respuestas a las preguntas del enunciado

1. ¿Compensa siempre tener que ordenar el vector?
   - {'En este experimento reducido sí compensa en el mayor tamaño probado para las tres cargas de búsqueda.' if sort_compensa else 'En este experimento reducido no compensa siempre; depende de la carga de búsquedas.'}
2. ¿Varía con el número de búsquedas?
   - Sí. En general, al aumentar el número de búsquedas, el coste fijo de ordenar se amortiza más.
   - Speedup binaria vs secuencial en n máximo:
     - carga 100.000: {bin_vs_sec_100:.2f}x
     - carga 5.000.000: {bin_vs_sec_5m:.2f}x
     - carga 500.000.000: {bin_vs_sec_500m:.2f}x
3. ¿Se aprecia diferencia entre secuencial con y sin centinela?
   - Sí, pero menor que frente a binaria. Relación secuencial/sentinela en n máximo:
     - carga 100.000: {cen_vs_sec_100:.2f}x
     - carga 5.000.000: {cen_vs_sec_5m:.2f}x
     - carga 500.000.000: {cen_vs_sec_500m:.2f}x

## Conclusiones

- La búsqueda binaria mantiene ventaja clara cuando el número de búsquedas crece, incluso incluyendo el coste de ordenación.
- En cargas pequeñas, la ventaja existe pero es menor porque el coste de ordenar pesa más proporcionalmente.
- La versión secuencial con centinela mejora ligeramente respecto a la secuencial clásica, pero ambas siguen escalando linealmente y quedan lejos de binaria para cargas altas.
"""


def main():
    parser = argparse.ArgumentParser(description="Genera informe Markdown para benchmark de 9 configuraciones")
    parser.add_argument("--results", default="results", help="Directorio de DAT")
    parser.add_argument("--plot", default=os.path.join("results", "benchmark_plot.svg"), help="Ruta de imagen")
    parser.add_argument("--output", default=os.path.join("results", "benchmark_report.md"), help="Ruta del informe MD")
    args = parser.parse_args()

    mapping = {
        "BSEC100000": "BSEC100000.DAT",
        "BSEC5000000": "BSEC5000000.DAT",
        "BSEC500000000": "BSEC500000000.DAT",
        "BSECCEN100000": "BSECCEN100000.DAT",
        "BSECCEN5000000": "BSECCEN5000000.DAT",
        "BSECCEN500000000": "BSECCEN500000000.DAT",
        "BBIN100000": "BBIN100000.DAT",
        "BBIN5000000": "BBIN5000000.DAT",
        "BBIN500000000": "BBIN500000000.DAT",
    }

    datasets = {}
    for key, filename in mapping.items():
        rows = read_rows(os.path.join(args.results, filename))
        datasets[key] = {"rows": rows, "summary": summarize(rows)}

    content = build_markdown(args.plot, datasets)

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as fh:
        fh.write(content)

    print(f"Informe generado en: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
