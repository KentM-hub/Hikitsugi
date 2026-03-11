#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MEMORY_SIZE (1024 * 1024 * 10) // 10MB
#define PAGE_SIZE 4096 // 4KB
#define NUM_PAGES (MEMORY_SIZE / PAGE_SIZE) // 2048ページ
#define NUM_ACCESSES 1 // アクセス回数

void random_page_access(char* memory) {
    // srand((unsigned int)time(NULL)); // 不要になったのでコメントアウト
    for(int loop=0;loop<1000;loop++){
      for (size_t i = 0; i < 64; ++i) {
        size_t page = i % NUM_PAGES; // 線形にページを選ぶ
        size_t offset = i % PAGE_SIZE; // 線形にオフセットを選ぶ
        memory[page * PAGE_SIZE + offset] = (char)(i % 257); // 順番に書き込み
        printf("Kento: %p\n", (void*)&memory[page * PAGE_SIZE + offset]);
      }
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
    printf("線形ページアクセスの実行時間: %.2f マイクロ秒\n", duration);
    printf("アクセスしたページ数: %zu\n", NUM_PAGES);

    free(memory); // メモリを解放
    return 0;
}

