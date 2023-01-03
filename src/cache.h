/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include "shell.h"

typedef struct block_t
{
    int valid; 
    int tag; 
    uint32_t data[8]; 
    int offset; 
    int age; 
} block_t;

typedef struct set_t
{
    int fullblocks; 
    block_t *blocklist; 
} set_t;

typedef struct cache_t
{
    int sets; 
    int ways; 
    set_t *setlist;
} cache_t;









cache_t *cache_new(int sets, int ways, int block);
void cache_destroy(cache_t *c);
void cache_update(cache_t *c, uint64_t addr, int hit, int index);

#endif
