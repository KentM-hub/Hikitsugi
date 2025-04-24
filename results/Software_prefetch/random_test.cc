#include <iostream>
#include <cstdlib>
#include <chrono>
#include <random>

const size_t MEMORY_SIZE = 1024 * 1024 * 10; // 10MB
const size_t PAGE_SIZE = 4096; // 4KB
const size_t NUM_PAGES = MEMORY_SIZE / PAGE_SIZE; // 2048ページ
const size_t NUM_ACCESSES = 10000; 

void random_page_access(char* memory) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> page_dist(0, NUM_PAGES - 1); // どのページか
    std::uniform_int_distribution<size_t> offset_dist(0, PAGE_SIZE - 1); // ページのどの位置か

    for (size_t i = 0; i < NUM_ACCESSES; ++i) {
        size_t random_page = page_dist(gen) * PAGE_SIZE; // ランダムなページ
        size_t random_offset = offset_dist(gen); // ランダムなオフセット
        memory[random_page + random_offset] = static_cast<char>(i); // ランダムな位置にキャストして書き込み
    }
}

int main() {
    char* memory = new char[MEMORY_SIZE];
    if (!memory) {
        std::cerr << "メモリの確保に失敗しました。" << std::endl;
        return EXIT_FAILURE;
    }

    std::fill(memory, memory + MEMORY_SIZE, 0);

    auto start = std::chrono::high_resolution_clock::now();
    random_page_access(memory);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "ランダムページアクセスの実行時間: " << duration.count() << " マイクロ秒" << std::endl;
    std::cout << "アクセスしたページ数: " << NUM_PAGES << std::endl;

    delete[] memory;
    return 0;
}

