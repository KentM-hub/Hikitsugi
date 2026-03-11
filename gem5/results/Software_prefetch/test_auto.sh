#!/bin/bash

# --- 設定 ---
GEM5_OPT="/home/kento/Documents/gem5/build/X86/gem5.opt"
CONFIG_SCRIPT="/home/kento/Documents/gem5/configs/tutorial/part1/two_level.py"
CACHE_PY="/home/kento/Documents/gem5/configs/tutorial/part1/caches.py" # ファイル名確認
BASE_RESULTS_DIR="/home/kento/Documents/gem5/results/Software_prefetch"
OUTPUT_TABLE_FILE="${BASE_RESULTS_DIR}/prefetcher_comparison_results.tsv"
CACHE_PY_BACKUP="${CACHE_PY}.$(date +%Y%m%d_%H%M%S).bak"
CACHE_PY_ERROR_COPY="${BASE_RESULTS_DIR}/caches.py.error_$(date +%Y%m%d_%H%M%S)"

# 変更するプリフェッチャのリスト
PREFETCHERS=(
    "StridePrefetcher()"
    "MultiPrefetcher()"
    "TaggedPrefetcher()"
    "IndirectMemoryPrefetcher()"
    "SignaturePathPrefetcher()"
    "SignaturePathPrefetcherV2(use_virtual_addresses=False)"
    "AMPMPrefetcher()"
    "DCPTPrefetcher()"
    "IrregularStreamBufferPrefetcher()"
    "SlimAMPMPrefetcher()"
    "BOPPrefetcher()"
    "SBOOEPrefetcher()"
    "STeMSPrefetcher()"
    "PIFPrefetcher()"
)

# --- 関数: awk を使って複数行定義を置換 (構文修正版) ---
# $1: 入力ファイル
# $2: 出力ファイル
# $3: 置換対象の開始行番号 (空の場合はパターンマッチ)
# $4: 新しい prefetcher 設定文字列 (例: "prefetcher = StridePrefetcher()")
update_cache_py_multiline() {
    local infile="$1"
    local outfile="$2"
    local start_line_num="$3"
    local new_config_line="$4"
    local temp_file="${infile}.tmp"

    # awk スクリプト実行
    awk -v start_line="$start_line_num" \
        -v new_config="$new_config_line" \
        '
        BEGIN {
            in_prefetcher_def = 0;  # 削除モード中か？
            start_indent = "";      # 開始行のインデント
            found_start = 0;        # 開始行を見つけたか？
            line_processed = 0;     # 現在行が処理済みか？
            in_l2cache_scope = 0;   # L2Cacheクラス定義内にいるか？(パターンマッチ用)
        }

        # --- メイン処理 ---

        # 0. 行番号指定があり、現在行がその番号なら、処理を試みる
        if (start_line != "" && NR == start_line && /^\s*prefetcher = /) {
            match($0, /^(\s*)/);
            start_indent = substr($0, RSTART, RLENGTH);
            print start_indent new_config; # 新しい行を挿入
            in_prefetcher_def = 1;          # 削除モード開始
            found_start = 1;
            line_processed = 1;             # この行は処理済み (printしないように)
            next;                           # 次の行へ
        }

        # 1. 行番号指定がない場合、L2Cacheスコープ内で prefetcher = を探す
        if (start_line == "" && !found_start) {
            # スコープ判定 (簡易)
            if (/class L2Cache/) { in_l2cache_scope = 1 }
            # スコープ内で prefetcher = を探す
            if (in_l2cache_scope && /^\s*prefetcher = /) {
                match($0, /^(\s*)/);
                start_indent = substr($0, RSTART, RLENGTH);
                print start_indent new_config; # 新しい行を挿入
                in_prefetcher_def = 1;          # 削除モード開始
                found_start = 1;
                line_processed = 1;             # この行は処理済み
                # スコープ終了判定 (簡易) - 次のクラス/関数定義など
                # 注意: L2Cacheクラスの終わりを正確に判定するのは難しい
                #       ここでは tags = の行で無理やり終了させてみる
                if (/tags = BaseSetAssoc()/) { in_l2cache_scope = 0 }
                next; # 次の行へ
            }
            # スコープ終了判定 (簡易)
            if (/tags = BaseSetAssoc()/) { in_l2cache_scope = 0 }
        }

        # 2. 削除モード中の処理 (複数行定義の残りを削除)
        if (in_prefetcher_def) {
            # インデントを取得
            match($0, /^(\s*)/);
            current_indent = substr($0, RSTART, RLENGTH);

            # 開始インデントと同じか、より浅いインデントが見つかったら削除モード終了
            # かつ、空行やコメント行でないことを確認 (\S は空白以外の文字)
            if (length(current_indent) <= length(start_indent) && /\S/) {
                 in_prefetcher_def = 0; # 削除モード終了
                 # この行は定義の外なので、次のステップで出力される
            } else {
                 # インデントが深い、または空行/コメント行などは定義の一部とみなし読み飛ばす
                 line_processed = 1; # この行は処理済み (printしないように)
                 next;               # 次の行へ
            }
        }

        # 3. 削除モード外、かつ未処理の行を出力
        if (!in_prefetcher_def && !line_processed) {
            print $0;
        }

        # 4. line_processed フラグをリセット
        line_processed = 0;

        ' "$infile" > "$temp_file"

    # awk が成功したかチェック
    if [ $? -ne 0 ]; then
        echo "Error: awk command failed to process $infile. Check awk script syntax."
        rm -f "$temp_file"
        return 1
    fi

    # 一時ファイルを元のファイルに上書き
    mv "$temp_file" "$outfile"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to move $temp_file to $outfile."
        return 1
    fi

    return 0 # 成功
}


# --- 準備 (変更なし) ---
mkdir -p "$BASE_RESULTS_DIR" || { echo "Error: Failed to create results directory: $BASE_RESULTS_DIR"; exit 1; }
if [ ! -f "$CACHE_PY" ]; then echo "Error: Python script not found: $CACHE_PY"; exit 1; fi
cp "$CACHE_PY" "$CACHE_PY_BACKUP" || { echo "Error: Failed to backup $CACHE_PY"; exit 1; }
echo "Backed up original $CACHE_PY to $CACHE_PY_BACKUP"
echo -e "Prefetcher\tSimSeconds\tL2PrefetcherAccuracy\tL2PrefetcherCoverage" > "$OUTPUT_TABLE_FILE"

# --- メインループ (変更なし) ---
original_prefetcher_line_num=$(grep -n "prefetcher = " "$CACHE_PY" | head -n 1 | cut -d: -f1)
if [ -z "$original_prefetcher_line_num" ]; then
    echo "Warning: Could not reliably determine the starting line number for 'prefetcher ='. Using pattern matching (less precise)."
fi

for prefetcher_config in "${PREFETCHERS[@]}"; do
    prefetcher_name=$(echo "$prefetcher_config" | sed -e 's/([^)]*)//g' -e 's/ *$//')

    echo ""
    echo "=========================================================="
    echo " Starting simulation for Prefetcher: $prefetcher_name"
    echo "=========================================================="

    # 1. caches.py を awk で書き換える
    new_prefetcher_line="prefetcher = $prefetcher_config"
    echo "Updating $CACHE_PY using awk (multiline replace): Setting $new_prefetcher_line"

    update_cache_py_multiline "$CACHE_PY" "$CACHE_PY" "$original_prefetcher_line_num" "$new_prefetcher_line"
    update_status=$?

    if [ $update_status -ne 0 ]; then
        echo "Error: Failed to update $CACHE_PY for $prefetcher_name. Restoring backup and exiting."
        cp "$CACHE_PY_BACKUP" "$CACHE_PY"
        #rm "$CACHE_PY_BACKUP" # Exitするのでバックアップは残す
        exit 1
    fi

    # (デバッグ用コメントアウト)
    # echo "--- Content around prefetcher line in $CACHE_PY ---"
    # grep -C 5 "prefetcher = " "$CACHE_PY"
    # echo "--------------------------------------------------"

    # 2. Python構文チェック
    echo "Checking Python syntax..."
    python3 -m py_compile "$CACHE_PY"
    compile_status=$?

    if [ $compile_status -ne 0 ]; then
       echo "Error: Python syntax error detected in $CACHE_PY after modification for $prefetcher_name."
       error_file_copy_path="${CACHE_PY_ERROR_COPY}_${prefetcher_name}.py"
       cp "$CACHE_PY" "$error_file_copy_path"
       echo ">>> The modified file with the error has been saved to: $error_file_copy_path"
       echo "Restoring backup and skipping this prefetcher."
       cp "$CACHE_PY_BACKUP" "$CACHE_PY"
       echo -e "${prefetcher_name}\tSYNTAX_ERROR\tSYNTAX_ERROR\tSYNTAX_ERROR" >> "$OUTPUT_TABLE_FILE"
       continue
    fi

    # 3. gem5 シミュレーション実行 (以降変更なし)
    output_dir="${BASE_RESULTS_DIR}/m5out_${prefetcher_name}"
    echo "Running gem5 simulation... Output directory: $output_dir"
    # rm -rf "$output_dir"

    command_line="$GEM5_OPT -d \"$output_dir\" \"$CONFIG_SCRIPT\""
    echo "Executing: $command_line"

    eval $command_line

    if [ $? -ne 0 ]; then
        echo "Error: gem5 simulation failed for $prefetcher_name. Check gem5 output above. Skipping."
        sim_seconds="FAIL"; accuracy="FAIL"; coverage="FAIL"
        echo -e "${prefetcher_name}\t${sim_seconds}\t${accuracy}\t${coverage}" >> "$OUTPUT_TABLE_FILE"
        echo "Restoring $CACHE_PY from backup before next iteration..."
        cp "$CACHE_PY_BACKUP" "$CACHE_PY"
        continue
    fi

    # 4. stats.txt から情報を抽出
    stats_file="${output_dir}/stats.txt"
    echo "Extracting stats from $stats_file..."
    if [ ! -f "$stats_file" ]; then
        echo "Warning: stats.txt not found at $stats_file."
        sim_seconds="NO_STATS"; accuracy="NO_STATS"; coverage="NO_STATS"
    else
        sim_seconds=$(grep -m 1 "simSeconds" "$stats_file" | awk '{print $2}' || echo "N/A")
        accuracy=$(grep -m 1 "system\.l2\.prefetcher\.accuracy" "$stats_file" | awk '{print $2}' || echo "N/A")
        coverage=$(grep -m 1 "system\.l2\.prefetcher\.coverage" "$stats_file" | awk '{print $2}' || echo "N/A")
        if [[ "$accuracy" == "N/A" ]]; then accuracy=$(grep -m 1 "system\.l2cache\.prefetcher\.accuracy" "$stats_file" | awk '{print $2}' || echo "N/A"); fi
        if [[ "$coverage" == "N/A" ]]; then coverage=$(grep -m 1 "system\.l2cache\.prefetcher\.coverage" "$stats_file" | awk '{print $2}' || echo "N/A"); fi
    fi
    echo "  SimSeconds: $sim_seconds"; echo "  L2 Prefetcher Accuracy: $accuracy"; echo "  L2 Prefetcher Coverage: $coverage"

    # 5. 結果を出力ファイルに追記
    echo -e "${prefetcher_name}\t${sim_seconds}\t${accuracy}\t${coverage}" >> "$OUTPUT_TABLE_FILE"

    echo "Finished simulation for Prefetcher: $prefetcher_name"
    echo "=========================================================="

    # 次のループのためにファイルを元に戻す
    echo "Restoring $CACHE_PY from backup before next iteration..."
    cp "$CACHE_PY_BACKUP" "$CACHE_PY"

done

# --- 後処理 (変更なし) ---
echo ""
echo "Ensuring original $CACHE_PY is restored from $CACHE_PY_BACKUP..."
cp "$CACHE_PY_BACKUP" "$CACHE_PY"
rm "$CACHE_PY_BACKUP"
echo "Original $CACHE_PY restored. Backup file removed."

echo ""
echo "==================== Experiment Summary ===================="
echo "Results saved in TSV format to: $OUTPUT_TABLE_FILE"
echo ""
echo "--- Comparison Table ---"
column -t -s $'\t' "$OUTPUT_TABLE_FILE" | head -n 25
if ls "${BASE_RESULTS_DIR}/caches.py.error_"* > /dev/null 2>&1; then
  echo ""
  echo ">>> NOTE: Syntax errors occurred. Copies of the problematic files were saved to:"
  ls "${BASE_RESULTS_DIR}/caches.py.error_"*
fi
echo "=========================================================="
echo "Script finished."

exit 0

