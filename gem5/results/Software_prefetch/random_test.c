#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MEMORY_SIZE (1024 * 1024 * 1000) // 10MB
#define PAGE_SIZE 4096 // 4KB
#define NUM_PAGES (MEMORY_SIZE / PAGE_SIZE) // 2048ページ
#define NUM_ACCESSES 10000 // アクセス回数

void random_page_access(char* memory) {
    srand((unsigned int)time(NULL)); // 乱数シードを現在時刻で初期化

    for (size_t i = 0; i < NUM_ACCESSES; ++i) {
        size_t random_page = (size_t)(rand() % NUM_PAGES) * PAGE_SIZE; // ランダムなページ
        size_t random_offset = (size_t)(rand() % PAGE_SIZE); // ランダムなオフセット
        memory[random_page + random_offset] = (char)(i % 256); // ランダムな位置に書き込み
    }
}

int main() {
    char* memory = (char*)malloc(MEMORY_SIZE); // 10MBのメモリを確保
    if (!memory) {
        fprintf(stderr, "メモリの確保に失敗しました。\n");
        return EXIT_FAILURE;
    }

    clock_t start = clock(); // 開始時間を記録
    random_page_access(memory);
    clock_t end = clock(); // 終了時間を記録

    double duration = (double)(end - start) / CLOCKS_PER_SEC * 1000000; // マイクロ秒に変換
    printf("ランダムページアクセスの実行時間: %.2f マイクロ秒\n", duration);
    printf("アクセスしたページ数: %zu\n", NUM_PAGES);

    free(memory); // メモリを解放
    return 0;
}
