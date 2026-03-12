import sys
import os

def extract_page_mapping(vaddr_paddr_file, pc_mem_file, output_file, page_size=0x1000):
    """
    PC_MEM.txtの仮想アドレスから、対応する物理ページの先頭アドレスのみを抽出し、出力します。
    """
    
    # 1. 仮想ページ -> 物理ページのマッピングを読み込む
    vpage_to_ppage = {}
    print(f"Reading page mappings from {vaddr_paddr_file}...")
    
    try:
        with open(vaddr_paddr_file, 'r') as f:
            for line in f:
                # 行からVADDR_PAGEとPADDR_PAGEを抽出 (スペース区切り)
                parts = line.strip().split()
                if len(parts) < 2:
                    continue
                
                try:
                    # 16進数文字列を整数に変換 (0xプレフィックスなしに対応)
                    vaddr_page = int(parts[0], 16)
                    paddr_page = int(parts[1], 16)
                    
                    # ページの割当て対応付けを辞書に保存
                    vpage_to_ppage[vaddr_page] = paddr_page
                except ValueError:
                    # 不正なアドレス文字列を含む行は無視
                    continue

    except FileNotFoundError:
        print(f"Error: Mapping file not found at {vaddr_paddr_file}")
        sys.exit(1)

    print(f"Loaded {len(vpage_to_ppage)} page mappings.")
    
    # 2. アクセストレースを読み込み、アドレスを変換して出力
    print(f"Processing access trace from {pc_mem_file}...")
    
    access_count = 0
    translated_count = 0

    try:
        with open(pc_mem_file, 'r') as infile, open(output_file, 'w') as outfile:
            
            # ヘッダーの書き込み
            outfile.write("PC_ADDR VADDR_ACCESS PADDR_PAGE_START\n")
            outfile.write("-" * 40 + "\n")

            for line in infile:
                access_count += 1
                
                # PC_MEM.txtの形式: PC VADDR_ACCESS (2列のみ)
                parts = line.strip().split()
                if len(parts) < 2:
                    continue

                pc_str, vaddr_str = parts[0], parts[1]
                
                try:
                    # VADDRとPCを整数に変換
                    vaddr = int(vaddr_str, 16)
                    pc_addr = int(pc_str, 16)
                except ValueError:
                    # 不正なアドレス文字列を含む行はスキップ
                    continue
                
                # ----------------------------------------------------
                # VADDRをページ境界に揃えて物理ページ開始アドレスを検索
                # ----------------------------------------------------
                # VADDRをページ開始アドレスに切り詰める (ページ番号の計算)
                vpage_start = vaddr & ~(page_size - 1)
                
                # マッピングを検索
                if vpage_start in vpage_to_ppage:
                    # 物理ページ開始アドレスを取得
                    ppage_start = vpage_to_ppage[vpage_start]
                    
                    # 結果の出力 (物理ページ開始アドレスのみを出力)
                    outfile.write(f"{hex(pc_addr)} {hex(vaddr)} {hex(ppage_start)}\n")
                    translated_count += 1
                else:
                    # マッピングが見つからない場合
                    outfile.write(f"{hex(pc_addr)} {hex(vaddr)} NOT_MAPPED\n")
                        
    except FileNotFoundError:
        print(f"Error: Trace file or mapping file not found.")
        sys.exit(1)
        
    print("-" * 40)
    print(f"Finished processing. Total access lines: {access_count}")
    print(f"Successfully translated addresses: {translated_count}")
    print(f"Output saved to {output_file}")


if __name__ == "__main__":
    
    # ファイル名を設定
    VADDR_PADDR_FILE = "vaddr_paddr.txt"
    PC_MEM_FILE = "PC_MEM.txt"
    OUTPUT_FILE = "physical_page_start_trace.txt"
    
    # ページサイズ (X86の一般的な4KB=0x1000をデフォルトとする)
    PAGE_SIZE = 0x1000 
    
    extract_page_mapping(VADDR_PADDR_FILE, PC_MEM_FILE, OUTPUT_FILE, PAGE_SIZE)
