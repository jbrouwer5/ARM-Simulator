/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 */

#ifndef _BP_H_
#define _BP_H_
#include "shell.h"

typedef struct
{
    uint64_t address; 
    int valid; 
    int conditional; 
    uint64_t branch; 
} btb_entry; 

typedef struct
{
    /* gshare */
    // 8 bits which holds the last 8 branchings 
    // lowest bit is the most recent branch 
    // 1 means took, 0 means didn't
    uint32_t gshare; 

    // pht which holds either 00, 01, 10, or 11
    uint32_t pht[256]; 

    /* BTB */
    // Holds a struct with a 64 bit address, a valid bit, a cond bit, and 
    // a 64 bit destination address
    btb_entry btb[1024]; 
} bp_t;


 
void bp_predict(uint64_t currPC);
void bp_update(uint64_t currPC, int cond, int correct, uint64_t result);

#endif
