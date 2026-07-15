#!/usr/bin/env python3
"""
fdc1004_plot.py - Plot channel-0 capacitance from an fdc1004_logger.py CSV:
raw data with a rolling mean overlaid and a shaded +/-1 std band.

Usage:
    pip install matplotlib numpy
    python fdc1004_plot.py capture.csv
    python fdc1004_plot.py capture.csv --window 20 --output plot.png
"""

import argparse
import csv
import sys

import numpy as np
import matplotlib.pyplot as plt
from numpy.lib.stride_tricks import sliding_window_view


def load_ch0(csv_path):
    """Return (tick_ms, ch0_pF) arrays, skipping rows with missing/NaN ch0."""
    tick_ms = []
    ch0 = []
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                t = int(row["device_tick_ms"])
                c0 = float(row["ch0_pF"])
            except (KeyError, ValueError):
                continue
            if np.isnan(c0):
                continue  # sensor was disconnected/erroring for this sample
            tick_ms.append(t)
            ch0.append(c0)
    return np.array(tick_ms, dtype=float), np.array(ch0, dtype=float)


def rolling_mean_std(values, window):
    """Sliding-window mean and std; output is shorter by (window - 1)."""
    if len(values) < window:
        return np.array([]), np.array([])
    windows = sliding_window_view(values, window)
    return windows.mean(axis=1), windows.std(axis=1)


def csv_path_basename(path):
    return path.rsplit("/", 1)[-1].rsplit("\\", 1)[-1]


def main():
    parser = argparse.ArgumentParser(
        description="Plot FDC1004 channel-0 capacitance: raw + rolling mean/std band.")
    parser.add_argument("csv_path", help="CSV file produced by fdc1004_logger.py")
    parser.add_argument("--window", type=int, default=10,
                         help="Rolling window size, in samples (default: 10)")
    parser.add_argument("--output", default=None,
                         help="Save the plot to this file instead of showing it interactively")
    args = parser.parse_args()

    tick_ms, ch0 = load_ch0(args.csv_path)
    if len(ch0) == 0:
        print(f"No valid ch0 samples found in {args.csv_path}")
        sys.exit(1)

    t_s = (tick_ms - tick_ms[0]) / 1000.0  # elapsed seconds since first sample

    means, stds = rolling_mean_std(ch0, args.window)
    # sliding_window_view drops (window-1) leading samples; align the
    # rolling curve to the end of each window so it lines up with t_s.
    t_roll = t_s[args.window - 1:] if len(means) > 0 else np.array([])

    plt.style.use("default")  # force matplotlib's light theme regardless of local rcParams
    fig, ax = plt.subplots(figsize=(11, 6))
    fig.patch.set_facecolor("white")
    ax.set_facecolor("white")

    ax.plot(t_s, ch0, color="#4C72B0", linewidth=0.9, alpha=0.45, label="Raw")

    if len(means) > 0:
        ax.plot(t_roll, means, color="#C44E52", linewidth=2.0,
                label=f"{args.window}-sample rolling mean")
        ax.fill_between(t_roll, means - stds, means + stds,
                         color="#C44E52", alpha=0.18, label="+/-1 std")
    else:
        print(f"Not enough samples ({len(ch0)}) for a {args.window}-sample rolling window.")

    ax.set_xlabel("Time (s)", fontsize=11)
    ax.set_ylabel("Capacitance (pF)", fontsize=11)
    ax.set_title("FDC1004 - Channel 0 Capacitance", fontsize=14, fontweight="bold", pad=12)
    ax.grid(True, alpha=0.3, linestyle="--")
    ax.legend(loc="upper right", framealpha=0.9, fontsize=9)
    ax.tick_params(labelsize=9)
    for spine in ("top", "right"):
        ax.spines[spine].set_visible(False)

    stats_text = (
        f"file: {csv_path_basename(args.csv_path)}\n"
        f"n = {len(ch0)} samples\n"
        f"duration = {t_s[-1]:.1f} s\n"
        f"mean = {ch0.mean():.4f} pF\n"
        f"std = {ch0.std():.4f} pF\n"
        f"min / max = {ch0.min():.4f} / {ch0.max():.4f} pF"
    )
    ax.text(0.01, 0.98, stats_text, transform=ax.transAxes,
            fontsize=8.5, va="top", ha="left", family="monospace",
            bbox=dict(boxstyle="round", facecolor="white", edgecolor="#999999", alpha=0.9))

    fig.tight_layout()

    if args.output:
        fig.savefig(args.output, dpi=150, facecolor="white")
        print(f"Saved plot to {args.output}")
    else:
        plt.show()


if __name__ == "__main__":
    main()
