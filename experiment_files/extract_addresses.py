import re
import os

# ログファイル名
LOG_FILENAME = "access_trace.log"

# 正規表現パターンを定義
# 1. 実行アドレス (PC): (0x[0-9a-f]+)
# 2. メモリアドレス: A=(0x[0-9a-f]+)
# 両方が含まれる行（メモリアクセスのマイクロオペレーション行）を検索します。
pattern = re.compile(r':\s+(0x[0-9a-f]+)\s+@.*?A=(0x[0-9a-f]+)')

# 抽出結果を格納するリスト
results = []

print(f"--- ファイル {LOG_FILENAME} からデータを読み込み中 ---")

# ファイルの存在を確認
if not os.path.exists(LOG_FILENAME):
    print(f"エラー: ファイル '{LOG_FILENAME}' が見つかりません。")
else:
    try:
        with open(LOG_FILENAME, 'r') as f:
            for line in f:
                # 行からPCとメモリアドレスを検索
                match = pattern.search(line)

                if match:
                    # match.group(1)がPC, match.group(2)がメモリアドレス
                    pc_address = match.group(1)
                    mem_address = match.group(2)
                    results.append(f"{pc_address} {mem_address}")

        # 結果の出力
        print("\n抽出結果:")
        print("実行アドレス (PC)  メモリアドレス (A)")
        print("-----------------------------------")
        if results:
            for result in results:
                print(result)
        else:
            print("メモリアクセスに関連するデータが見つかりませんでした。")

    except Exception as e:
        print(f"ファイルの読み込み中にエラーが発生しました: {e}")
