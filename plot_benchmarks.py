#!/usr/bin/env python3
import argparse
import csv
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
                    "searches_requested": int(row.get("searches_requested", "0")),
                    "searches_executed": int(row.get("searches_executed", "0")),
                    "time_seconds": float(row["time_seconds"]),
                    "status": row.get("status", "OK"),
                }
            )
    return rows


def plot_all(results_dir, output_path):
    files = {
        "Secuencial": "seq.csv",
        "Secuencial+Centinela": "seq_cent.csv",
        "Binaria": "bin.csv",
    }

    width = 1100
    height = 650
    margin_left = 90
    margin_right = 30
    margin_top = 50
    margin_bottom = 80
    chart_w = width - margin_left - margin_right
    chart_h = height - margin_top - margin_bottom

    palette = {
        "Secuencial": "#1f77b4",
        "Secuencial+Centinela": "#ff7f0e",
        "Binaria": "#2ca02c",
    }

    series = {}
    all_sizes = []
    all_times = []

    for label, filename in files.items():
        path = os.path.join(results_dir, filename)
        if not os.path.exists(path):
            raise FileNotFoundError(f"No existe {path}. Ejecuta primero el benchmark.")
        rows = read_csv(path)
        series[label] = rows
        for row in rows:
            all_sizes.append(row["n"])
            all_times.append(row["time_seconds"])

    if not all_sizes or not all_times:
        raise RuntimeError("No hay datos para graficar")

    min_n = min(all_sizes)
    max_n = max(all_sizes)
    max_t = max(all_times)
    if max_t <= 0.0:
        max_t = 1.0

    def x_pos(n):
        if max_n == min_n:
            return margin_left + (chart_w / 2)
        return margin_left + ((n - min_n) / (max_n - min_n)) * chart_w

    def y_pos(t):
        return margin_top + (1.0 - (t / max_t)) * chart_h

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">')
    svg.append('<rect width="100%" height="100%" fill="#ffffff"/>')
    svg.append(f'<text x="{width/2}" y="30" text-anchor="middle" font-size="22" fill="#111">Benchmark de busquedas sobre BigDecimal</text>')

    for i in range(6):
        t = (max_t / 5.0) * i
        y = y_pos(t)
        svg.append(f'<line x1="{margin_left}" y1="{y:.2f}" x2="{width-margin_right}" y2="{y:.2f}" stroke="#e5e7eb" stroke-width="1"/>')
        svg.append(f'<text x="{margin_left-10}" y="{y+4:.2f}" text-anchor="end" font-size="12" fill="#374151">{t:.1f}s</text>')

    x_ticks = sorted(set(all_sizes))
    for n in x_ticks:
        x = x_pos(n)
        svg.append(f'<line x1="{x:.2f}" y1="{margin_top}" x2="{x:.2f}" y2="{height-margin_bottom}" stroke="#f3f4f6" stroke-width="1"/>')
        svg.append(f'<text x="{x:.2f}" y="{height-margin_bottom+20}" text-anchor="middle" font-size="11" fill="#374151">{n}</text>')

    svg.append(f'<line x1="{margin_left}" y1="{height-margin_bottom}" x2="{width-margin_right}" y2="{height-margin_bottom}" stroke="#111" stroke-width="1.5"/>')
    svg.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{height-margin_bottom}" stroke="#111" stroke-width="1.5"/>')
    svg.append(f'<text x="{width/2}" y="{height-20}" text-anchor="middle" font-size="14" fill="#111">Tamano del vector (n)</text>')
    svg.append(f'<text x="25" y="{height/2}" transform="rotate(-90 25 {height/2})" text-anchor="middle" font-size="14" fill="#111">Tiempo (segundos)</text>')

    legend_y = margin_top + 5
    for label, rows in series.items():
        color = palette[label]
        ok_rows = [r for r in rows if r["status"] == "OK"]
        timeout_rows = [r for r in rows if r["status"] == "TIMEOUT"]

        if ok_rows:
            pts = " ".join(f"{x_pos(r['n']):.2f},{y_pos(r['time_seconds']):.2f}" for r in ok_rows)
            svg.append(f'<polyline points="{pts}" fill="none" stroke="{color}" stroke-width="2.5"/>')
            for r in ok_rows:
                svg.append(f'<circle cx="{x_pos(r["n"]):.2f}" cy="{y_pos(r["time_seconds"]):.2f}" r="4" fill="{color}"/>')

        for r in timeout_rows:
            x = x_pos(r["n"])
            y = y_pos(r["time_seconds"])
            svg.append(f'<line x1="{x-5:.2f}" y1="{y-5:.2f}" x2="{x+5:.2f}" y2="{y+5:.2f}" stroke="{color}" stroke-width="2"/>')
            svg.append(f'<line x1="{x-5:.2f}" y1="{y+5:.2f}" x2="{x+5:.2f}" y2="{y-5:.2f}" stroke="{color}" stroke-width="2"/>')

        svg.append(f'<rect x="{width-330}" y="{legend_y}" width="14" height="14" fill="{color}"/>')
        svg.append(f'<text x="{width-310}" y="{legend_y+12}" font-size="13" fill="#111">{label}</text>')
        legend_y += 22

    svg.append('</svg>')

    with open(output_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(svg))

    print(f"Grafica guardada en: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Genera grafica de benchmarks.")
    parser.add_argument("--run-cmd", default=None, help="Comando opcional para ejecutar benchmark antes de graficar")
    parser.add_argument("--results", default="results", help="Directorio de CSV de benchmark")
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

    report_cmd = f'python3 generate_report.py --results "{args.results}" --plot "{args.output}" --output "{args.report}"'
    report_rc = subprocess.call(report_cmd, shell=True)
    if report_rc != 0:
        print("No se pudo generar el informe markdown automaticamente.", file=sys.stderr)
        return report_rc

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
