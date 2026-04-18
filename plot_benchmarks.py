#!/usr/bin/env python3
import argparse
import csv
import math
import os
import subprocess
import sys


def read_csv(path):
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


def plot_all(results_dir, output_path):
    files = [
        ("Secuencial 100k", "BSEC100000.DAT", "#1f77b4"),
        ("Secuencial 5M", "BSEC5000000.DAT", "#17becf"),
        ("Secuencial 500M", "BSEC500000000.DAT", "#0f4c5c"),
        ("Secuencial+Cent 100k", "BSECCEN100000.DAT", "#ff7f0e"),
        ("Secuencial+Cent 5M", "BSECCEN5000000.DAT", "#f4a261"),
        ("Secuencial+Cent 500M", "BSECCEN500000000.DAT", "#bc6c25"),
        ("Binaria 100k", "BBIN100000.DAT", "#2ca02c"),
        ("Binaria 5M", "BBIN5000000.DAT", "#5a189a"),
        ("Binaria 500M", "BBIN500000000.DAT", "#6a994e"),
    ]

    width = 1260
    height = 760
    margin_left = 90
    margin_right = 250
    margin_top = 60
    margin_bottom = 90
    chart_w = width - margin_left - margin_right
    chart_h = height - margin_top - margin_bottom

    series = {}
    all_sizes = []
    all_times = []

    for label, filename, color in files:
        path = os.path.join(results_dir, filename)
        if not os.path.exists(path):
            raise FileNotFoundError(f"No existe {path}. Ejecuta primero el benchmark.")
        rows = read_csv(path)
        if not rows:
            raise RuntimeError(f"No hay filas en {path}")

        series[label] = {"rows": rows, "color": color}
        for row in rows:
            all_sizes.append(row["n"])
            all_times.append(row["time_seconds"])

    min_n = min(all_sizes)
    max_n = max(all_sizes)
    min_t = min(t for t in all_times if t > 0.0)
    max_t = max(all_times)

    min_log_t = math.log10(min_t)
    max_log_t = math.log10(max_t)
    if max_log_t <= min_log_t:
        max_log_t = min_log_t + 1.0

    def x_pos(n):
        if max_n == min_n:
            return margin_left + (chart_w / 2.0)
        return margin_left + ((n - min_n) / (max_n - min_n)) * chart_w

    def y_pos(t):
        t_safe = max(t, min_t)
        norm = (math.log10(t_safe) - min_log_t) / (max_log_t - min_log_t)
        return margin_top + (1.0 - norm) * chart_h

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">')
    svg.append('<rect width="100%" height="100%" fill="#ffffff"/>')
    svg.append(f'<text x="{width / 2}" y="32" text-anchor="middle" font-size="24" fill="#111">Comparativa de 9 configuraciones de busqueda (modo reducido)</text>')
    svg.append(f'<text x="{width / 2}" y="52" text-anchor="middle" font-size="13" fill="#4b5563">Binaria incluye ordenacion quicksort + busquedas. Eje Y en log10.</text>')

    min_exp = int(math.floor(min_log_t))
    max_exp = int(math.ceil(max_log_t))
    for exp in range(min_exp, max_exp + 1):
        t = 10 ** exp
        y = y_pos(t)
        svg.append(f'<line x1="{margin_left}" y1="{y:.2f}" x2="{width - margin_right}" y2="{y:.2f}" stroke="#e5e7eb" stroke-width="1"/>')
        svg.append(f'<text x="{margin_left - 10}" y="{y + 4:.2f}" text-anchor="end" font-size="12" fill="#374151">{t:.0e}s</text>')

    x_ticks = sorted(set(all_sizes))
    for n in x_ticks:
        x = x_pos(n)
        svg.append(f'<line x1="{x:.2f}" y1="{margin_top}" x2="{x:.2f}" y2="{height - margin_bottom}" stroke="#f3f4f6" stroke-width="1"/>')
        svg.append(f'<text x="{x:.2f}" y="{height - margin_bottom + 22}" text-anchor="middle" font-size="11" fill="#374151">{n}</text>')

    svg.append(f'<line x1="{margin_left}" y1="{height - margin_bottom}" x2="{width - margin_right}" y2="{height - margin_bottom}" stroke="#111" stroke-width="1.5"/>')
    svg.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{height - margin_bottom}" stroke="#111" stroke-width="1.5"/>')
    svg.append(f'<text x="{(margin_left + width - margin_right) / 2}" y="{height - 25}" text-anchor="middle" font-size="14" fill="#111">Tamano del vector (n)</text>')
    svg.append(f'<text x="30" y="{height / 2}" transform="rotate(-90 30 {height / 2})" text-anchor="middle" font-size="14" fill="#111">Tiempo total (segundos, log10)</text>')

    legend_y = margin_top + 12
    for label, info in series.items():
        rows = info["rows"]
        color = info["color"]
        points = " ".join(f"{x_pos(r['n']):.2f},{y_pos(r['time_seconds']):.2f}" for r in rows)
        svg.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2.5"/>')
        for r in rows:
            svg.append(f'<circle cx="{x_pos(r["n"]):.2f}" cy="{y_pos(r["time_seconds"]):.2f}" r="3.5" fill="{color}"/>')

        svg.append(f'<rect x="{width - margin_right + 20}" y="{legend_y}" width="14" height="14" fill="{color}"/>')
        svg.append(f'<text x="{width - margin_right + 42}" y="{legend_y + 12}" font-size="12" fill="#111">{label}</text>')
        legend_y += 21

    svg.append('</svg>')

    with open(output_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(svg))

    print(f"Grafica guardada en: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Genera grafica de benchmarks (9 configuraciones).")
    parser.add_argument("--run-cmd", default=None, help="Comando opcional para ejecutar benchmark antes de graficar")
    parser.add_argument("--results", default="results", help="Directorio de DAT")
    parser.add_argument("--output", default=os.path.join("results", "benchmark_plot.svg"), help="Ruta del SVG de salida")
    parser.add_argument("--report", default=os.path.join("results", "benchmark_report.md"), help="Ruta de informe MD")
    args = parser.parse_args()

    if args.run_cmd:
        print(f"Ejecutando benchmark: {args.run_cmd}")
        rc = subprocess.call(args.run_cmd, shell=True)
        if rc != 0:
            print("El benchmark fallo. No se generan graficas.", file=sys.stderr)
            return rc

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    plot_all(args.results, args.output)

    report_cmd = [
        sys.executable,
        "generate_report.py",
        "--results",
        args.results,
        "--plot",
        args.output,
        "--output",
        args.report,
    ]
    report_rc = subprocess.call(report_cmd)
    if report_rc != 0:
        print("No se pudo generar el informe markdown automaticamente.", file=sys.stderr)
        return report_rc

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
