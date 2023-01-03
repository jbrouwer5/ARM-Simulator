/*
 * ARM pipeline timing simulator
 *
 * CMSC 22200, Fall 2016
 * Gushu Li and Reza Jokar
 */

#include "bp.h"
#include "pipe.h"
#include <stdlib.h>
#include <stdio.h>
// Initialize bp struct
bp_t bp_state; 

uint64_t nextPC;

void bp_predict(uint64_t currPC)
{
    //moving this to top of file
    //uint64_t nextPC; // What we update the PC to

    /* Predict next PC */

    // BTB index is [11:2] of the PC 
    uint32_t btb_index = currPC & 0xFFC; 

    // PHT index is [9:2] of the PC indexed with gshare
    uint32_t pht_index = bp_state.gshare ^ (currPC & 0x3FC); 

    // If invalid or address isn't the same as our PC
    // Simply add 4 instead of branching 
    if (bp_state.btb[btb_index].valid == 0 || bp_state.btb[btb_index].address != currPC){
        nextPC = currPC + 4; 
    }
    // If unconditional or btb says to take it 
    // Take the branch
    else if(bp_state.btb[btb_index].conditional == 0 || bp_state.pht[pht_index] >= 2){
        nextPC = bp_state.btb[btb_index].branch; 
    }
    else{
        nextPC = currPC + 4; 
    }
}

void bp_update(uint64_t currPC, int cond, int taken, uint64_t result)
{
    uint32_t btb_index = currPC & 0xFFC; 
    // If the branch is conditional
    if (cond) {
        /* Update BTB */
        bp_state.btb[btb_index].conditional = cond; 
        bp_state.btb[btb_index].address = currPC; 
        bp_state.btb[btb_index].valid = 1; 
        bp_state.btb[btb_index].branch = result;

        /* Update gshare directional predictor */
        // Find the index again
        uint32_t pht_index = bp_state.gshare ^ (currPC & 0x3FC); 

        // If the prediction was correct add 1 to the entry
        // Otherwise subtract 1
        if (taken && bp_state.pht[pht_index] < 3){
            bp_state.pht[pht_index] += 1; 
        }
        else if ((!taken) && (bp_state.pht[pht_index] > 0)){
            bp_state.pht[pht_index] -= 1;
        }

        // Shift left and make lowest but the recent outcome 
        /* Update global history register */
        bp_state.gshare = ((bp_state.gshare << 1) + taken) & 0xFF; 
    }
    else{
        // If unconditional update btb to represent that 
        bp_state.btb[btb_index].conditional = cond; 
    }
}
