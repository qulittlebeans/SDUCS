#include "cache.h"
#include "pipe.h"
#include "shell.h"
#include "mips.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include<math.h>

ICache icache;  // 全局的指令缓存
DCache dcache;  // 全局的数据缓存

int icache_delay =0;  // icache 访问延迟，单位：cycle
int dcache_delay = 0;  // dcache 访问延迟，单位：cycle

// 初始化 icache
void icache_init() {
     int i =0, j = 0;
    for(i=0;i<64;i++){
        for(j=0;j<4;j++){
            icache.sets[i].lines[j].valid = 0;
            icache.sets[i].lines[j].lru = 0;
        }
    }
}

// 初始化 dcache
void dcache_init() {
     int i =0, j = 0;
    for(i=0;i<256;i++)
    {
        for(j=0;j<8;j++)
        {
            dcache.sets[i].lines[j].valid = 0;
            dcache.sets[i].lines[j].lru = 0;
           dcache.sets[i].lines[j].dirty = 0;
        }
    }
}


// 读取 icache 中的数据
uint32_t icache_read(uint32_t address) {
    //地址的高位部分用来确定缓存集的位置。这里的 (address >> 5) 将地址右移 5 位，去除低 5 位（字节偏移部分），然后通过 & 0x3F 保留低 6 位，意味着缓存共有 64 个集合（64 sets）。
   uint32_t set = (address >>5) &(0x3F);
   // 地址的低 5 位决定了在缓存行内的字节位置。每个缓存行存储 32 字节（5 位表示位置，2^5 = 32 字节）。通过 address & 0x1F 可以得到该地址在缓存行中的具体位置。
	uint32_t inoffset = address & (0x1f);
    //剩余的高位部分作为标签来区分不同的缓存块
	uint32_t tag = address >> 11 ;//tag 21 bytes
	uint32_t word=0;//用于存储从缓存中读取的数据
	int flag_lru=0;	//用于记录当前行的 LRU（Least Recently Used）标志，以便在缓存替换时使用
	int write_line=0;//用于记录发生缓存命中的缓存行索引。
    for(int i=0;i<4;i++){
        if(icache.sets[set].lines[i].valid==1){//judge is valid
            if(icache.sets[set].lines[i].tag == tag){//judge is tag right
                flag_lru=icache.sets[set].lines[i].lru;        
                write_line=i;//缓存行的索引
                uint32_t j=inoffset;//根据偏移量 inoffset，从缓存行的 data 数组中提取目标字（32 位数据）
                 word=  (icache.sets[set].lines[i].data[j]<<0)    |
                        (icache.sets[set].lines[i].data[j+1]<<8)  |
                        (icache.sets[set].lines[i].data[j+2]<<16) |
                        (icache.sets[set].lines[i].data[j+3]<<24);  
                break;
            }
        }
    }
    //cache hit缓存命中
    if(write_line!=-1){
   	 	for(int i=0;i<4;i++){
    		if(i!=write_line){
    			if(icache.sets[set].lines[i].lru<flag_lru){
    				icache.sets[set].lines[i].lru++;
    			}//对于其他不相关的缓存行（i != write_line），如果它们的 LRU 值小于当前缓存行的 LRU 值（flag_lru），则将其 LRU 值加 1。
    		}
    	}
    	return word;
    }
    //cache miss
    icache_delay = 49;
    icache_write(address);//数据从内存中读取到缓存
    return 0;
}

// 写入 icache（写时可能会忽略，因为指令缓存通常为只读）
void icache_write(uint32_t address) {
   uint32_t inoffset= address & 0x1F;//loc of line ,5 bytes
    uint32_t set= (address >>5) &(0x3F);//6 bytes of block   
    uint32_t tag=address >>11;//tag 21 bytes
    uint32_t start_line= (address >> 5)<<5;//the loc of write line
    int write_line= -1;
    int flag_lru=0;
    for(int i=0;i<4;i++){//查找空缓存行并写入
        if(icache.sets[set].lines[i].valid==0){//find the line in the set which isn't been written,initialize the line
        	flag_lru=icache.sets[set].lines[i].lru;
            icache.sets[set].lines[i].tag = tag;//new line ,the lru is zero        
            icache.sets[set].lines[i].valid = 1;//the line is changed
            icache.sets[set].lines[i].lru = 0;//update the lru
            write_line = i;            
            break;
        }
    }
    //cache hit,every lru++,expect write line
    if(write_line!=-1){//更新LRU值（缓存命中）
    	for(int i=0;i<4;i++){
       	  if(write_line!=i){
       	  	if(flag_lru>icache.sets[set].lines[i].lru)
       	     	icache.sets[set].lines[i].lru++;
       	  }
   		}
   	}
	else if(write_line==-1){//cache miss,the cache is full now,it should be replaced with one line
        uint8_t max_lru = 0;
        for(int i=0;i<4;i++){
            icache.sets[set].lines[i].lru++;//every lru ++,because cache miss
            if(max_lru<=icache.sets[set].lines[i].lru){//find the max_lru and write_line index in the lines of sets 
               max_lru = icache.sets[set].lines[i].lru;
               write_line = i;
            }
        }//找最大LRU将他替换为新的数据
        //init the replaced line again
        icache.sets[set].lines[write_line].tag = tag;//replace the origional tag with new tag
        icache.sets[set].lines[write_line].valid = 1;//the line is changed
        icache.sets[set].lines[write_line].lru = 0; //new line ,the lru is zero       
    }
    //从内存读取数据并写入缓存行
    for(int i = 0;i<8;i++){
        uint32_t word = mem_read_32(start_line);//the mem block size is 32 bytes
        icache.sets[set].lines[write_line].data[i*4]   = (uint8_t)(word>>0) &0xFF;//load low 8 bytes total 32 bytes each times from memory to write_lines
        icache.sets[set].lines[write_line].data[i*4+1] = (uint8_t)(word>>8) &0xFF;
        icache.sets[set].lines[write_line].data[i*4+2] = (uint8_t)(word>>16)&0xFF;
        icache.sets[set].lines[write_line].data[i*4+3] = (uint8_t)(word>>24)&0xFF;
        start_line+=4;
    }
}

uint32_t dcache_read(uint32_t address){
    uint32_t inoffset = address & (0x1F);//get the low 5 bytes
    uint32_t set = (address >>5) &(0xFF);//get 5-12 bytes
    uint32_t tag = address >> 13 ;//get tag bytes
    for(int i=0;i<8;i++){
        if(dcache.sets[set].lines[i].valid==1){
            if(dcache.sets[set].lines[i].tag == tag){
                uint32_t word = (dcache.sets[set].lines[i].data[inoffset]<<0)    |
                                (dcache.sets[set].lines[i].data[inoffset+1]<<8)  |
                                (dcache.sets[set].lines[i].data[inoffset+2]<<16) |
                                (dcache.sets[set].lines[i].data[inoffset+3]<<24);  
                return word;               
            }
        }
    }
    //cache miss,read and write from main memory
    dcache_delay = 49;
    dcache_LRU(address);
    return 0;
}

// 写入 dcache 数据
void dcache_write(uint32_t address, uint32_t write) {
   uint32_t inoffset = address & (0x1F);//get the low 5 bytes
    uint32_t set = (address >>5) &(0xFF);//get 5-12 bytes
    uint32_t tag = address >> 13 ;//get tag bytes
     
    for(int i=0;i<8;i++){
        if(dcache.sets[set].lines[i].valid==1){
            if(dcache.sets[set].lines[i].tag == tag){
                dcache.sets[set].lines[i].dirty = 1;
                dcache.sets[set].lines[i].data[inoffset]   = (uint8_t)(write>>0)&0xff;
                dcache.sets[set].lines[i].data[inoffset+1] = (uint8_t)(write>>8)&0xff;
                dcache.sets[set].lines[i].data[inoffset+2] = (uint8_t)(write>>16)&0xff;
                dcache.sets[set].lines[i].data[inoffset+3] = (uint8_t)(write>>24)&0xff;
                return;           
            }
        }
    }
}

// LRU 替换策略：更新缓存行的 LRU 计数器
void dcache_LRU(uint32_t address) {
    //get a block from the address  
	uint32_t inoffset = address & (0x1F);//get the low 5 bytes
    uint32_t set = (address >>5) &(0xFF);//get 5-12 bytes
    uint32_t tag = address >> 13 ;//get tag bytes
    uint32_t start_line = (address >> 5)<<5;//the begin of memory in the main memory
    int write_line = -1;
    uint32_t write_tag = 0;//replace tag
    int flag_lru=0;
    for(int i=0;i<8;i++){//查找一个空闲缓存行
        if(dcache.sets[set].lines[i].valid==0){
            write_line = i; 
            flag_lru=dcache.sets[set].lines[i].lru;//record the lru of line
            dcache.sets[set].lines[i].tag = tag;                    
            dcache.sets[set].lines[i].valid = 1;
            dcache.sets[set].lines[i].lru = 0;
            break;
        }
    }
	//缓存命中，LRU更新
	if(write_line!=-1){
    	for(int i=0;i<8;i++){
         	if(write_line!=i){
         		if(dcache.sets[set].lines[i].lru<flag_lru)
         			dcache.sets[set].lines[i].lru++;
         		else continue;
        	 }
  	  	}
    }
    //缓存未命中
    else if(write_line==-1){
        uint8_t max_lru = 0;
        for(int i=0;i<8;i++){
            dcache.sets[set].lines[i].lru++;
            if(max_lru<dcache.sets[set].lines[i].lru){
               max_lru = dcache.sets[set].lines[i].lru;
               write_tag = dcache.sets[set].lines[i].tag;
               write_line = i;               
            }
        }
       //if dirty!=0 , write to main memory
        if(dcache.sets[set].lines[write_line].dirty != 0){
        	uint32_t maddress = 0;
        	maddress = (write_tag<<13)|(set<<5);
        	for(int i=0;i<8;i++){
          	  	uint32_t word = dcache_read(maddress);
            	mem_write_32(maddress,word);
            	maddress+=4;
       	 	}//如果被替换的行是脏的（dirty != 0），则需要将其内容写回主内存
            //通过计算主内存地址（maddress）来将缓存数据写回到主内存
		}
        //update the line
        dcache.sets[set].lines[write_line].tag = tag;        
        dcache.sets[set].lines[write_line].valid = 1;
        dcache.sets[set].lines[write_line].lru = 0;        
    }
    //将数据从主存加载到缓存
    for(int i=0;i<8;i++){
        uint32_t word = mem_read_32(start_line);
       dcache.sets[set].lines[write_line].data[i*4]   = (uint8_t)(word>>0)&0xff;
        dcache.sets[set].lines[write_line].data[i*4+1] = (uint8_t)(word>>8)&0xff;
        dcache.sets[set].lines[write_line].data[i*4+2] = (uint8_t)(word>>16)&0xff;
        dcache.sets[set].lines[write_line].data[i*4+3] = (uint8_t)(word>>24)&0xff;
        start_line+=4;
    }


}
