# Calculate median time and comparisons per element for each algorithm on random
# vs its own adversarial input, and print each multiplier.

import pandas as pd

main = pd.read_csv("results/main.csv")
adv = pd.read_csv("results/adversary.csv")

n = 100_000
algorithms = ["qs_random", "bfprt", "floyd_rivest", "introselect", "std_nth"]

def median_per_element(df, algo, family, n, metric):
    rows = df[
        (df["algo"] == algo) &
        (df["family"] == family) &
        (df["n"] == n) &
        (df["rank"] == "median") &
        (df["prefix"] == 0)
    ]
    if rows.empty:
        raise ValueError(f"no rows found for {algo=} {family=} {n=}")
    return rows[metric].median() / n

for algorithm in algorithms:
    random_time = median_per_element(main, algorithm, "random", n, "time_ns")
    adversarial_time = median_per_element(
        adv, algorithm, f"mcilroy:{algorithm}", n, "time_ns"
    )
    time_multiplier = adversarial_time / random_time

    random_comparisons = median_per_element(
        main, algorithm, "random", n, "comparisons"
    )
    adversarial_comparisons = median_per_element(
        adv, algorithm, f"mcilroy:{algorithm}", n, "comparisons"
    )
    comparisons_multiplier = adversarial_comparisons / random_comparisons

    print(
        f"{algorithm}:\n"
        f"  time: {time_multiplier:.3f}x "
        f"({adversarial_time:.3f} ns/element adversarial / "
        f"{random_time:.3f} ns/element random)\n"
        f"  comparisons: {comparisons_multiplier:.3f}x "
        f"({adversarial_comparisons:.3f} comparisons/element adversarial / "
        f"{random_comparisons:.3f} comparisons/element random)"
    )