#!/usr/bin/env python3
"""plot.py -- turn a bench CSV into plots and summary tables.

    python3 benchmarking/plot.py results/results.csv --out results/plots
    python3 benchmarking/plot.py results/params.csv --out results/plots_bfprt --algos '^bfprt' --tag bfprt

Options:
    --type int|string   which element type to plot (default int)
    --rank median       rank used for the time/comparison-vs-n plots
    --algos REGEX       only algorithms whose label matches (default: all)
    --families REGEX    only families whose label matches (default: all)
    --prefix N          prefix length for string runs in the vs-n plots (default 0)
    --tag NAME          filename prefix, so several studies can share an output dir

Outputs (PNG + CSV) in --out:
    <tag>time_vs_n.png       ns per element vs n, one panel per input family
    <tag>cmps_vs_n.png       comparisons per element vs n (dotted lines at 1.5 and 2)
    <tag>rank_sweep.png      ns per element by target rank at the largest n
    <tag>time_vs_cmps.png    ns per element vs comparisons per element (largest n)
    <tag>strings_prefix.png  (string runs) time and ns/comparison vs prefix length
    <tag>summary_time.csv, <tag>summary_cmps.csv   pivot tables at the largest n
Lines show the median over repetitions/seeds; shaded bands are the interquartile range.
"""
import argparse
import os
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

RANK_ORDER = ["min", "p1", "p25", "median", "p75", "p99", "max"]


def load(args):
    df = pd.read_csv(args.csv)
    df = df[df["type"] == args.type]
    if args.algos:
        df = df[df["algo"].str.contains(args.algos, regex=True)]
    if args.families:
        df = df[df["family"].str.contains(args.families, regex=True)]
    return df


def aggregate(df, col):
    d = df[df[col] >= 0]
    g = d.groupby(["algo", "family", "n", "prefix", "rank"])[col]
    out = g.agg(med="median",
                lo=lambda s: s.quantile(0.25),
                hi=lambda s: s.quantile(0.75)).reset_index()
    return out


def color_map(algos):
    cmap = plt.get_cmap("tab20")
    return {a: cmap(i % 20) for i, a in enumerate(sorted(algos))}


def panels(families):
    nf = len(families)
    ncols = min(3, nf)
    nrows = int(np.ceil(nf / ncols))
    fig, axes = plt.subplots(nrows, ncols, figsize=(5.2 * ncols, 3.8 * nrows), squeeze=False)
    return fig, axes.flatten()


def vs_n_plot(agg, colors, ylabel, fname, refs=(), logy=True):
    fams = sorted(agg["family"].unique())
    if not fams:
        return
    fig, axes = panels(fams)
    for ax, fam in zip(axes, fams):
        sub = agg[agg["family"] == fam]
        for algo, g in sub.groupby("algo"):
            g = g.sort_values("n")
            ax.plot(g["n"], g["med"] / g["n"], marker="o", ms=3, lw=1.3,
                    label=algo, color=colors[algo])
            ax.fill_between(g["n"], g["lo"] / g["n"], g["hi"] / g["n"],
                            color=colors[algo], alpha=0.15, lw=0)
        for r in refs:
            ax.axhline(r, ls=":", color="gray", lw=0.8)
        ax.set_xscale("log")
        if logy:
            ax.set_yscale("log")
        ax.set_title(fam, fontsize=10)
        ax.set_xlabel("n")
        ax.set_ylabel(ylabel)
        ax.grid(alpha=0.25)
    for ax in axes[len(fams):]:
        ax.axis("off")
    h, l = axes[0].get_legend_handles_labels()
    fig.legend(h, l, loc="lower center", ncol=min(len(l), 5), fontsize=8)
    fig.tight_layout(rect=(0, 0.07, 1, 1))
    fig.savefig(fname, dpi=150)
    plt.close(fig)


def rank_sweep_plot(t, colors, fname):
    fams = sorted(t["family"].unique())
    if not fams:
        return
    fig, axes = panels(fams)
    for ax, fam in zip(axes, fams):
        sub = t[t["family"] == fam]
        nmax = sub["n"].max()
        sub = sub[sub["n"] == nmax]
        ranks = [r for r in RANK_ORDER if r in set(sub["rank"])] or sorted(set(sub["rank"]))
        for algo, g in sub.groupby("algo"):
            g = g.set_index("rank").reindex(ranks)
            ax.plot(range(len(ranks)), g["med"] / nmax, marker="o", ms=3, label=algo, color=colors[algo])
        ax.set_xticks(range(len(ranks)))
        ax.set_xticklabels(ranks, fontsize=8)
        ax.set_yscale("log")
        ax.set_title(f"{fam} (n={int(nmax):,})", fontsize=10)
        ax.set_ylabel("ns per element")
        ax.grid(alpha=0.25)
    for ax in axes[len(fams):]:
        ax.axis("off")
    h, l = axes[0].get_legend_handles_labels()
    fig.legend(h, l, loc="lower center", ncol=min(len(l), 5), fontsize=8)
    fig.tight_layout(rect=(0, 0.07, 1, 1))
    fig.savefig(fname, dpi=150)
    plt.close(fig)


def time_vs_cmps_plot(t, c, colors, rank, fname):
    m = t[t["rank"] == rank].merge(c[c["rank"] == rank],
                                   on=["algo", "family", "n", "prefix", "rank"],
                                   suffixes=("_t", "_c"))
    if m.empty:
        return
    fig, ax = plt.subplots(figsize=(7, 5))
    markers = "osD^vP*Xh<>"
    fams = sorted(m["family"].unique())
    for fi, fam in enumerate(fams):
        s = m[m["family"] == fam]
        s = s[s["n"] == s["n"].max()]
        for _, r in s.iterrows():
            ax.scatter(r["med_c"] / r["n"], r["med_t"] / r["n"], color=colors[r["algo"]],
                       marker=markers[fi % len(markers)], s=40, alpha=0.85)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("comparisons per element")
    ax.set_ylabel("ns per element")
    ax.set_title(f"Time vs comparisons at largest n (rank={rank})\ncolour = algorithm, marker = family")
    ax.grid(alpha=0.25)
    algo_h = [plt.Line2D([], [], marker="o", ls="", color=colors[a], label=a) for a in sorted(colors)]
    fam_h = [plt.Line2D([], [], marker=markers[i % len(markers)], ls="", color="k", label=f)
             for i, f in enumerate(fams)]
    l1 = ax.legend(handles=algo_h, fontsize=7, loc="upper left", title="algorithm")
    ax.add_artist(l1)
    ax.legend(handles=fam_h, fontsize=7, loc="lower right", title="family")
    fig.tight_layout()
    fig.savefig(fname, dpi=150)
    plt.close(fig)


def strings_plot(t, c, colors, fname):
    fams = sorted(t["family"].unique())
    if not fams or t["prefix"].nunique() < 2:
        return
    fig, axes = plt.subplots(2, len(fams), figsize=(5 * len(fams), 7), squeeze=False)
    for j, fam in enumerate(fams):
        sub = t[(t["family"] == fam) & (t["rank"] == t["rank"].iloc[0])]
        nmax = sub["n"].max()
        sub = sub[sub["n"] == nmax]
        csub = c[(c["family"] == fam) & (c["rank"] == sub["rank"].iloc[0]) & (c["n"] == nmax)]
        for algo, g in sub.groupby("algo"):
            g = g.sort_values("prefix")
            axes[0][j].plot(g["prefix"] + 1, g["med"] / nmax, marker="o", ms=3, label=algo, color=colors[algo])
            cc = csub[csub["algo"] == algo].set_index("prefix")["med"]
            gg = g.set_index("prefix")["med"]
            both = pd.concat([gg, cc], axis=1, keys=["t", "c"]).dropna()
            axes[1][j].plot(both.index + 1, both["t"] / both["c"], marker="o", ms=3, color=colors[algo])
        axes[0][j].set_title(f"{fam} (n={int(nmax):,})", fontsize=10)
        axes[0][j].set_ylabel("ns per element")
        axes[1][j].set_ylabel("ns per comparison")
        for i in (0, 1):
            axes[i][j].set_xscale("log")
            axes[i][j].set_yscale("log")
            axes[i][j].set_xlabel("shared prefix length + 1")
            axes[i][j].grid(alpha=0.25)
    h, l = axes[0][0].get_legend_handles_labels()
    fig.legend(h, l, loc="lower center", ncol=min(len(l), 5), fontsize=8)
    fig.tight_layout(rect=(0, 0.05, 1, 1))
    fig.savefig(fname, dpi=150)
    plt.close(fig)


def summaries(t, c, rank, outdir, tag):
    for name, a in (("time", t), ("cmps", c)):
        s = a[a["rank"] == rank]
        if s.empty:
            continue
        s = s[s["n"] == s.groupby("family")["n"].transform("max")]
        piv = s.pivot_table(index="family", columns="algo", values="med")
        piv = piv.div(s.groupby("family")["n"].first(), axis=0)  # per element
        piv.round(3).to_csv(os.path.join(outdir, f"{tag}summary_{name}.csv"))
        print(f"\n== {name} per element at largest n, rank={rank} "
              f"({'ns' if name == 'time' else 'comparisons'}) ==")
        print(piv.round(2).to_string())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--out", default="results/plots")
    ap.add_argument("--type", default="int")
    ap.add_argument("--rank", default="median")
    ap.add_argument("--algos", default="")
    ap.add_argument("--families", default="")
    ap.add_argument("--prefix", type=int, default=0)
    ap.add_argument("--tag", default="")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    tag = args.tag + "_" if args.tag else ""

    df = load(args)
    if df.empty:
        raise SystemExit("no rows match the filters")
    t_all = aggregate(df, "time_ns") if (df["time_ns"] >= 0).any() else pd.DataFrame()
    c_all = aggregate(df, "comparisons") if (df["comparisons"] >= 0).any() else pd.DataFrame()
    colors = color_map(df["algo"].unique())
    out = lambda f: os.path.join(args.out, tag + f)

    def at_prefix(a):
        return a[a["prefix"] == args.prefix] if not a.empty else a

    if not t_all.empty:
        t = at_prefix(t_all)
        vs_n_plot(t[t["rank"] == args.rank], colors, "ns per element", out("time_vs_n.png"))
        rank_sweep_plot(t, colors, out("rank_sweep.png"))
    if not c_all.empty:
        c = at_prefix(c_all)
        vs_n_plot(c[c["rank"] == args.rank], colors, "comparisons per element",
                  out("cmps_vs_n.png"), refs=(1.5, 2.0))
    if not t_all.empty and not c_all.empty:
        time_vs_cmps_plot(at_prefix(t_all), at_prefix(c_all), colors, args.rank, out("time_vs_cmps.png"))
        if args.type == "string":
            strings_plot(t_all, c_all, colors, out("strings_prefix.png"))
        summaries(at_prefix(t_all), at_prefix(c_all), args.rank, args.out, tag)
    print(f"\nwrote plots/tables to {args.out}")


if __name__ == "__main__":
    main()
