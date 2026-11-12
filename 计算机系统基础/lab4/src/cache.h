#ifndef CACHE_H
#define CACHE_H
#include"mips.h"
#include <stdint.h>
#include <stdbool.h>
#include<stdio.h>

#define ICACHE_SIZE (8 * 1024)  // 8 KB
#define ICACHE_WAYS 4
#define ICACHE_BLOCK_SIZE 32
#define ICACHE_SETS 64

#define DCACHE_SIZE (64 * 1024) // 64 KB
#define DCACHE_WAYS 8
#define DCACHE_BLOCK_SIZE 32
#define DCACHE_SETS 256

typedef struct {
	bool valid;//标志位，表示该缓存行是否有效
	uint32_t tag;//缓存行的标签，通常是地址的高位，用来区分不同的内存块
	uint32_t lru;//最近最少使用（Least Recently Used）计数器，用来跟踪缓存行的使用情况
	uint32_t data[32];
} icache_line;

typedef struct {
	icache_line lines[ICACHE_WAYS];//该集合有4个缓存行
} icache_set;

typedef struct {
	icache_set sets[ICACHE_SETS];//指令缓存共有64个集合
} ICache;
extern ICache icache;

typedef struct {
	bool valid;
	bool dirty;
	uint32_t tag;
	uint32_t lru;
	uint32_t data[32];
} dcache_line;

typedef struct {
	dcache_line lines[DCACHE_WAYS];
} dcache_set;

typedef struct {
	dcache_set sets[DCACHE_SETS];
} DCache;


extern DCache dcache;
extern int icache_delay;
extern int dcache_delay;


void icache_init();
void dcache_init();

void icache_write(uint32_t address);
uint32_t icache_read(uint32_t address);

void dcache_write(uint32_t address,uint32_t write);
uint32_t dcache_read(uint32_t address);
void dcache_LRU(uint32_t address);

#endif
