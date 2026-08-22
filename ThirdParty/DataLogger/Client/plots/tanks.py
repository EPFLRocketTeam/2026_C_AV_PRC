#!/usr/bin/env python3
"""
Plot pressure.values[0..2] and pressure.value from one CSV, optionally
together with pressure/pressure_mean from a second CSV, with an
interactive overview/detail view for navigating time.

Usage:
    python pressure_plot.py data.csv
    python pressure_plot.py data.csv --second-csv other.csv
"""

import argparse
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.widgets import SpanSelector

# Columns we actually care about for this plot
PRESSURE_COLS = [
    "pressure.values[0]",
    "pressure.values[1]",
    "pressure.values[2]",
    "pressure.value",
]

SECOND_COLS = ["pressure", "pressure_mean"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", help="Path to the main CSV file")
    parser.add_argument(
        "--second-csv",
        default=None,
        help="Optional path to a second CSV with columns: "
        "ts_us,pressure,pressure_mean",
    )
    args = parser.parse_args()

    df = pd.read_csv(args.csv_path, usecols=["ts_us"] + PRESSURE_COLS)

    df2 = None
    if args.second_csv:
        df2 = pd.read_csv(args.second_csv, usecols=["ts_us"] + SECOND_COLS)

    # Common time reference so both files line up on the same clock,
    # in case the second CSV doesn't start at exactly the same ts_us.
    t0 = df["ts_us"].iloc[0]
    if df2 is not None:
        t0 = min(t0, df2["ts_us"].iloc[0])

    t = (df["ts_us"] - t0) / 1e6
    data = {c: df[c] for c in PRESSURE_COLS}

    t2 = None
    data2 = {}
    if df2 is not None:
        t2 = (df2["ts_us"] - t0) / 1e6
        data2 = {c: df2[c] for c in SECOND_COLS}

    fig, (ax_detail, ax_overview) = plt.subplots(
        2, 1, figsize=(13, 7), gridspec_kw={"height_ratios": [3, 1]}
    )

    styles = {
        "pressure.values[0]": dict(lw=0.8, alpha=0.8, color="tab:blue"),
        "pressure.values[1]": dict(lw=0.8, alpha=0.8, color="tab:cyan"),
        "pressure.values[2]": dict(lw=0.8, alpha=0.8, color="tab:purple"),
        "pressure.value": dict(lw=1.4, color="k"),
        "pressure": dict(lw=0.9, alpha=0.9, color="tab:red"),
        "pressure_mean": dict(lw=1.4, color="tab:orange"),
    }

    for c in PRESSURE_COLS:
        ax_detail.plot(t, data[c], label=c, **styles[c])
        ax_overview.plot(t, data[c], **styles[c])

    if df2 is not None:
        for c in SECOND_COLS:
            ax_detail.plot(t2, data2[c], label=c, **styles[c])
            ax_overview.plot(t2, data2[c], **styles[c])

    ax_detail.set_xlabel("time (s)")
    ax_detail.set_ylabel("pressure")
    ax_detail.set_title(
        "Pressure (zoom/pan with toolbar, or drag a region below)"
    )
    ax_detail.legend(loc="upper right", fontsize=8)
    ax_detail.grid(True, alpha=0.3)

    ax_overview.set_xlabel("time (s)")
    ax_overview.set_title("Overview - click and drag to select a time range")
    ax_overview.grid(True, alpha=0.3)

    def onselect(xmin, xmax):
        if xmin == xmax:
            return
        ax_detail.set_xlim(xmin, xmax)

        mask = (t >= xmin) & (t <= xmax)
        mins, maxs = [], []
        if mask.any():
            mins += [data[c][mask].min() for c in PRESSURE_COLS]
            maxs += [data[c][mask].max() for c in PRESSURE_COLS]

        if t2 is not None:
            mask2 = (t2 >= xmin) & (t2 <= xmax)
            if mask2.any():
                mins += [data2[c][mask2].min() for c in SECOND_COLS]
                maxs += [data2[c][mask2].max() for c in SECOND_COLS]

        if mins and maxs:
            ymin, ymax = min(mins), max(maxs)
            pad = (ymax - ymin) * 0.05 or 1.0
            ax_detail.set_ylim(ymin - pad, ymax + pad)
        fig.canvas.draw_idle()

    # Keep a reference so it isn't garbage-collected
    span = SpanSelector(
        ax_overview,
        onselect,
        "horizontal",
        useblit=True,
        props=dict(alpha=0.3, facecolor="tab:blue"),
        interactive=True,
    )
    fig._span_selector = span  # extra safety against GC

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()