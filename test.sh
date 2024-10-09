#!/bin/bash
# カウンタを初期化
segfault_count=0
success_count=0
# 100回ループ
for i in {1..100}
do
  # ./a.outを実行し、結果を変数に保存
  build/X86/gem5.opt configs/tutorial/part1/two_level.py --l2_size=1MB --l1d_size=128kB
  exit_code=$?
  # 終了コードが139（セグメンテーションフォルト）かどうかをチェック
  if [ $exit_code -eq 139 ]; then
    segfault_count=$((segfault_count + 1))
  else
    success_count=$((success_count + 1))
  fi
done
# 結果を出力
echo “Segmentation faults: $segfault_count” > result.txt
echo “Successful runs: $success_count” >> result.txt
