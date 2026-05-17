import pandas as pd

bench = pd.read_csv("benchmark_results.csv")

mean_latency = bench["inference_latency_ms"].mean()

print("Mean Latency:", mean_latency)

if mean_latency < 8.0:
    print("PASS")
else:
    print("FAIL")
