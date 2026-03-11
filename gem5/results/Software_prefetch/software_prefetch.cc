#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <chrono>

#define PAGE_SIZE 4096
#define MAX_SIZE PAGE_SIZE * 10240

char  *getpage;
char  *getpage2;
const char *message = "Hello world";

void roop() {
    puts("begin");
    getpage = static_cast<char*> (mmap(NULL, MAX_SIZE, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
    if (getpage == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    int sum = 0;

    // 実行時間測定開始
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < MAX_SIZE; i += 32) {
        getpage[i] = '1';
        sum += getpage[i] - '0';
    }

    // 実行時間測定終了
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printf("getpageの合計:  %dページ\n", sum * 32 / 4096);
    printf("先頭仮想アドレスは%lxです。\n", (unsigned long)getpage);
    printf("処理時間: %ldマイクロ秒\n", duration.count());
}

void roop_with_prefetch() {
    puts("begin");
    getpage2 = static_cast<char*> (mmap(NULL, MAX_SIZE, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
    if (getpage2 == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    int sum = 0;

    // 実行時間測定開始
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < MAX_SIZE; i += 32) {
        __builtin_prefetch(&getpage2[i + 64], 0, 3); 

        getpage2[i] = '1';
        sum += getpage2[i] - '0';
    }

    // 実行時間測定終了
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printf("getpageの合計:  %dページ\n", sum * 32 / 4096);
    printf("先頭仮想アドレスは%lxです。\n", (unsigned long)getpage2);
    printf("処理時間: %ldマイクロ秒\n", duration.count());
}

int main() {
    roop();
    roop_with_prefetch();
    return 0;
}

