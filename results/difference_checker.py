import os
import math

# ファイルのパスを指定
file1_path = os.path.join(os.getcwd(), 'stats1.txt')
file2_path = os.path.join(os.getcwd(), 'stats2.txt')

# ファイルからデータを読み込む
def parse_file(filepath):
    stats = {}
    with open(filepath, 'r') as file:
        for line in file:
            # 行が空白か、# で始まる行はスキップ
            if not line.strip() or line.startswith('#'):
                continue
            
            # 行を空白で分割し、最初の要素をキー、2番目の要素を値とする
            parts = line.split()
            if len(parts) < 2:
                continue  # データが不足している行はスキップ

            key = parts[0]
            value = parts[1]

            # 数値に変換可能な場合のみ辞書に格納
            try:
                stats[key] = float(value)
            except ValueError:
                continue  # 数値でない場合はスキップ

    return stats

# ファイルからデータを読み込む
file1_stats = parse_file(file1_path)
file2_stats = parse_file(file2_path)

# 差を計算
differences = {}
for key, value1 in file1_stats.items():
    if key in file2_stats:
        value2 = file2_stats[key]
        # ゼロ除算とnanのチェック
        if max(value1, value2) == 0:
            continue
        diff = abs(value1 - value2) / max(value1, value2) * 100
        if not math.isnan(diff):  # nanでない場合のみ追加
            differences[key] = (diff, value1, value2)

# 差を大きい順に出力
sorted_differences = sorted(differences.items(), key=lambda x: x[1][0], reverse=True)

for key, (diff,value1,value2) in sorted_differences:
    print(f"{key}: {diff:.4f}% file1: {value1} file2: {value2}")

