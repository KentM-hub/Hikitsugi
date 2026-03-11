#include <iostream>
#include <cstdlib>
#include <chrono>

const size_t MEMORY_SIZE = 1024 * 1024 * 10; // 100MB
const size_t PAGE_SIZE = 4096; // 一般的なページサイズ (4KB)
const size_t NUM_ACCESSES = MEMORY_SIZE / PAGE_SIZE; // ページごとのアクセス回数

void page_stride_access(char* memory, size_t size) {
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        memory[i] = static_cast<char>(i); // 各ページの先頭に書き込み
    }
}

int main() {
    char* memory = new char[MEMORY_SIZE];
    if (!memory) {
        std::cerr << "メモリの確保に失敗しました。" << std::endl;
        return EXIT_FAILURE;
    }

    std::fill(memory, memory + MEMORY_SIZE, 0); // 初期化（オプション）

    auto start = std::chrono::high_resolution_clock::now();
    page_stride_access(memory, MEMORY_SIZE);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "ページごとのアクセスの実行時間: " << duration.count() << "ミリ秒" << std::endl;
    std::cout << NUM_ACCESSES <<std::endl;
    delete[] memory;
    return EXIT_SUCCESS;
}

