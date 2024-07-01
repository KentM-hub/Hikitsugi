#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <immintrin.h>
#pragma intrinsic(_mm_lfence)

#define PAGE_SIZE 4096
#define MAX_SIZE PAGE_SIZE * 10240

char  *attackerpage;
char  *victimpage;
const char *message = "Hello world";

void attack() {
    //_mm_lfence();
    puts("attacker");
    //_mm_lfence();
    attackerpage = mmap(NULL, MAX_SIZE, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS,-1, 0);
    if (attackerpage == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    
    int sum = 0;
    for(int i=0;i<MAX_SIZE;i+=32){
	    attackerpage[i] = '1';
	    sum+=attackerpage[i]-'0';
            //memcpy(attackerpage+i*PAGE_SIZE,message,strlen(message)+1);
    }
    printf("accessの合計:  %dページ\n", sum*32/4096);
     //_mm_lfence();
    printf("攻撃者の先頭仮想アドレスは%lxです。\n", attackerpage);
    //int result = munmap(attackerpage,PAGE_SIZE);
   
    //printf("攻撃者が1ページ開放しました。\n");
    
    /*
    sum = 0;
    for(int i=0;i<MAX_SIZE/sizeof(char);i++){
            attackerpage[i] = '1';
            sum+=attackerpage[i];　　　　　　　　　　　　　　　　　
            //memcpy(attackerpage+i*PAGE_SIZE,message,strlen(message)+1);
    }
    printf("2MBの領域の合計: %d\n", sum);
    */
    //void *tmp= mmap(NULL, PAGE_SIZE, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    //memcpy(tmp,message,strlen(message)+1);

}

void victim() {
     //_mm_lfence();
    puts("victim");
    victimpage = mmap(NULL, PAGE_SIZE, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (victimpage == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    printf("被攻撃者がmmapに成功しました。\n");
    int sum = 0;
    for(int i=0;i<PAGE_SIZE;i+=32){
            victimpage[i] = '1';
            sum+=victimpage[i]-'0';
            //memcpy(attackerpage+i*PAGE_SIZE,message,strlen(message)+1);
    }
    printf("accessの領域の合計:  %dページ\n", sum*32/4096);
    printf("被攻撃者の先頭仮想アドレスは%lxです。\n", victimpage);
}

int main() {
    attack();
    victim();
    return 0;
}


