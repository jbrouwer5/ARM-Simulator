/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include <stdlib.h>
#include <stdio.h>


cache_t *cache_new(int sets, int ways, int block)
{
    cache_t *new_cache = (cache_t *)malloc(sizeof(cache_t)); 
    new_cache->sets = sets; 
    new_cache->ways = ways; 

    new_cache->setlist = (set_t *)malloc(sizeof(set_t) * sets); 

    for (int i = 0; i < sets; i++){
        new_cache->setlist[i].fullblocks = 0; 
        new_cache->setlist[i].blocklist = (block_t *)malloc(sizeof(block_t) * ways);
    }

    return new_cache; 
}

void cache_destroy(cache_t *c)
{

    for (int i = 0; i < c->sets; i++){
        free(c->setlist[i].blocklist);  
    }

    free(c->setlist); 
    free(c); 
}

void cache_update(cache_t *c, uint64_t addr, int hit, int index)
{
    int setindex; 
    int tag; 

    if (c->sets == 64){
        setindex = (addr & 0x7E0) >> 5; // 10:5 bits
        tag = (addr & 0xFFFFFFFFFFFFFC00) >> 10; 
    }
    else{
        setindex = (addr & 0x1FE0) >> 5; // 12:5 bits
        tag = (addr & 0xFFFFFFFFFFFFF000) >> 12; 
    }
    

    if (hit){
        for (int j = 0; j < c->ways; j++){
            if (j == index){
                c->setlist[setindex].blocklist[j].age = 0; 
            }
            else if (c->setlist[setindex].blocklist[j].valid){
                c->setlist[setindex].blocklist[j].age += 1; 
            }
        }
    }
    else{
        int replacementindex = 0; 
        if (c->setlist[setindex].fullblocks < c->ways){
            replacementindex = c->setlist[setindex].fullblocks; 
            c->setlist[setindex].fullblocks += 1; 
        }
        else{
            for (int i = 0; i < c->ways; i++){
                if (c->setlist[setindex].blocklist[i].age > c->setlist[setindex].blocklist[replacementindex].age){
                    replacementindex = i; 
                }
            }
        }

        c->setlist[setindex].blocklist[replacementindex].valid = 1;
        c->setlist[setindex].blocklist[replacementindex].offset = addr & 0x1F; 

        for (int j = 0; j < c->ways; j++){
            if (j == replacementindex){
                c->setlist[setindex].blocklist[j].age = 0; 
            }
            else if (c->setlist[setindex].blocklist[j].valid){
                c->setlist[setindex].blocklist[j].age += 1; 
            }
        }

        c->setlist[setindex].blocklist[replacementindex].tag = tag; 
        int start = addr - (addr % 32); 

        c->setlist[setindex].blocklist[replacementindex].data[0] = mem_read_32(start); 
        c->setlist[setindex].blocklist[replacementindex].data[1] = mem_read_32(start + 0x4); 
        c->setlist[setindex].blocklist[replacementindex].data[2] = mem_read_32(start + 0x8); 
        c->setlist[setindex].blocklist[replacementindex].data[3] = mem_read_32(start + 0xC); 
        c->setlist[setindex].blocklist[replacementindex].data[4] = mem_read_32(start + 0x10); 
        c->setlist[setindex].blocklist[replacementindex].data[5] = mem_read_32(start + 0x14); 
        c->setlist[setindex].blocklist[replacementindex].data[6] = mem_read_32(start + 0x18); 
        c->setlist[setindex].blocklist[replacementindex].data[7] = mem_read_32(start + 0x1C); 
    }
}

