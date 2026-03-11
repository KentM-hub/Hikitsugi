import os
import re
import shutil
from pathlib import Path

# 各種パス設定
gem5_exec = "/home/kento/Documents/gem5/build/X86/gem5.opt"
config_script = "/home/kento/Documents/gem5/configs/tutorial/part1/two_level.py"
cache_py_path = "/home/kento/Documents/gem5/configs/tutorial/part1/caches.py"
output_dir = "/home/kento/Documents/gem5/results/Software_prefetch"
m5out_dir = os.path.join(output_dir, "m5out")
results_file = os.path.join(output_dir, "prefetch_comparison.tsv")

# プリフェッチャ一覧
prefetchers = [
    "StridePrefetcher",
    "MultiPrefetcher",
    "TaggedPrefetcher",
    "IndirectMemoryPrefetcher",
    "SignaturePathPrefetcher",
    "SignaturePathPrefetcherV2",
    "AMPMPrefetcher",
    "DCPTPrefetcher",
    "IrregularStreamBufferPrefetcher",
    "SlimAMPMPrefetcher",
    "BOPPrefetcher",
    "SBOOEPrefetcher",
    "STeMSPrefetcher",
    "PIFPrefetcher",
]

def replace_prefetcher(prefetcher_name):
    with open(cache_py_path, "r") as f:
        content = f.read()

    new_content = re.sub(
        r'prefetcher = .*?\(\)',
        f'prefetcher = {prefetcher_name}()',
        content
    )

    with open(cache_py_path, "w") as f:
        f.write(new_content)

def run_simulation():
    os.system(f"{gem5_exec} {config_script}")

def extract_valid_stats(stats_path, keys=None):
    results = {}
    if not os.path.exists(stats_path):
        return results
    with open(stats_path, "r") as f:
        for line in f:
            match = re.match(r"(\S+)\s+(\S+)", line)
            if match:
                key, value = match.groups()
                if re.fullmatch(r"[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?", value):
                    if keys is None or key in keys:
                        results[key] = value
    return results

def main():
    os.makedirs(output_dir, exist_ok=True)

    # 最初のプリフェッチャでキーリスト抽出
    replace_prefetcher(prefetchers[0])
    if os.path.exists(m5out_dir):
        shutil.rmtree(m5out_dir)
    run_simulation()
    first_stats = extract_valid_stats(os.path.join(m5out_dir, "stats.txt"))
    keys = sorted(first_stats.keys())

    with open(results_file, "w") as f:
        f.write("Prefetcher\t" + "\t".join(keys) + "\n")

        for prefetcher in prefetchers:
            print(f"[INFO] Running simulation with {prefetcher}...")
            replace_prefetcher(prefetcher)

            if os.path.exists(m5out_dir):
                shutil.rmtree(m5out_dir)

            run_simulation()
            stats = extract_valid_stats(os.path.join(m5out_dir, "stats.txt"), keys)

            row = [prefetcher] + [stats.get(key, "N/A") for key in keys]
            f.write("\t".join(row) + "\n")
            print(f"[DONE] {prefetcher} finished.\n")

    print(f"\u2705 実験結果を {results_file} に保存しました。")

if __name__ == "__main__":
    main()

