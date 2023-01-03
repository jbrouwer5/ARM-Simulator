/*
 * CMSC 22200
 * 
 * ARM pipeline timing simulator
 * Mickey Claffey (mclaffey) and Jackson Brouwer (jbrouwer)
 */
 
#include "pipe.h"
#include "shell.h"
#include "cache.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "stdbool.h"
#include "bp.h"

/* global pipeline state */
CPU_State CURRENT_STATE;
IF2ID_t IF2ID;
ID2EX_t ID2EX;
EX2MEM_t EX2MEM;
MEM2WB_t MEM2WB;
PendMem_t PendMem;
int b_instructions[] = {0x5};
int cb_instructions[] = {0xB4, 0xB5, 0x54};
int d_instructions[] = {0x7C2, 0x1C2, 0x3C2, 0x7C0, 0x1C0, 0x3C0};
int i_instructions[] = {0x488, 0x588, 0x69A, 0x69B, 0x1C4, 0x688, 0x788};
int iw_instructions[] = {0x694, 0x6A2};
int r_instructions[] = {0x458, 0x558, 0x450, 0x750, 0x650, 0x550, 0x658, 0x758, 0x4D8, 0x359,0x6B0};
int load_instructions[] = {0x7C2, 0x1C2, 0x3C2};
// 0x458 = ADD extended register (R)
// 0x5 = B (B)
// 0xB5 = CBNZ (CB)
// 0x488 = ADD immediate (I)
// 0x558 = ADDS extended register (R)
// 0x588 = ADDS immediate (I) 
// 0xB4 = CBZ (CB) 
// 0x450 = AND shifted register (R)
// 0x750 = ANDS shifted register (R)  
// 0x650 = EOR shifted register (R) 
// 0x550 = ORR shifted register (R) 
// 0x7C2 = LDUR (D)
// 0x1C2 = LDURB (D) 
// 0x3C2 = LDURH (D) 
// 0x69B = LSLI immediate (I) 
// 0x69A = LSRI immediate (I) 
// 0x694 = MOVZ (IW) 
// 0x7C0 = STUR (D) 
// 0x1C0 = STURB (D) 
// 0x3C0 = STURH (D) 
// 0x658 = SUB (R)
// 0x758 = SUBS (R) 
// 0x688 = SUBI (I)
// 0x788 = SUBIS (I) 
// 0x4D8 = MUL (R) 
// 0x6A2 = HLT (IW) 
// 0x359 = CMP (R)
// 0x1C4 = CMPI (I) 
// 0x6B0 = BR (R)
// 0X54 = b.cond
// beq
// bne 
// bgt
// blt
// bge
// ble 

//value from bp.c
uint64_t nextPC;
int flushed; 
int fetchflushed; 
int finished; 
int stall; 

//caches
cache_t *instruction_cache; 
cache_t *data_cache; 

int cache_hit; 

// stall cycles left, set index, and tag for fetch
int fetchStallCycles = 0;
int fetchsetindex; 
int fetchtag; 

// stall cycles left, set index, tag, and result for fetch
int memStallCycles = 0; 
int memsetindex; 
int memtag; 
int memdata; 

// dealing with control flow
int controlFlowInterruptionFlag = 0;

// data from the caches
uint32_t *requestedData_instr_cache;
uint32_t *requestedData_data_cache;

//flushing the pipe by setting the instruction and opcodes to 0
void flush(uint64_t target){
    if ((CURRENT_STATE.PC & 0xFFFFFFFFFFFFFFE0) != (target & 0xFFFFFFFFFFFFFFE0)){
        fetchflushed = 1; 
        fetchStallCycles = 0; 
    }
    else{
        fetchflushed = 0; 
    }
    CURRENT_STATE.PC = target; 
    IF2ID.instr = 0;
    ID2EX.opcode = 0; 
    flushed = 1; 
}

bool contains(int array[], int arraylen, uint32_t value){
    for (int i = 0; i < arraylen; i++){
        if (array[i] == value){
            return true;  
        }
    }
    return false; 
}

void pipe_init()
{
    finished = 0; 
    flushed = 0; 
    stall = 0; 
    instruction_cache = cache_new(64, 4, 4);
    data_cache = cache_new(256, 8, 8);
}

void pipe_cycle()
{
    pipe_stage_wb();
    pipe_stage_mem();
    pipe_stage_execute();
    pipe_stage_decode();
    pipe_stage_fetch();
}

void pipe_stage_wb()
{
    if (memStallCycles > 0){
        return; 
    }

    switch(MEM2WB.opcode){
        case 0x458: //ADD extended register
        {  
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x488: //ADD immediate
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x558: //ADDS extended register
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N;  
            break;
        }
        case 0x588: //ADDS immediate
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N;  
            break;
        }
        case 0xB5: //CBNZ
        {
            CURRENT_STATE.PC = MEM2WB.result;
            break;
        }
        case 0xB4: //CBZ
        {
            CURRENT_STATE.PC = MEM2WB.result;
            break;
        }
        case 0x450: //AND
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x750: //ANDS
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N; 
            break;
        }
        case 0x650: //EOR
        {
            if(MEM2WB.dest!=31){
                MEM2WB.dest = MEM2WB.result;
                MEM2WB.RegWrite = 1; 
            }
            break;
        }
        case 0x550: //ORR
        {
            if(MEM2WB.dest!=31){
                MEM2WB.dest = MEM2WB.result;
                MEM2WB.RegWrite = 1; 
            }
            break;
        }
        case 0x7C2: //LDUR
        {
            //loading 64 bits of data, load from offset and address right after offset, and shift that by 32 
            if(MEM2WB.dest!=31){ 
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1; 
            } 
            break; 
        } 
        case 0x1C2: //LDURB
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x3C2: //LDURH
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x69B: //LSLI
        {
            CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
            MEM2WB.RegWrite = 1; 
            break;
        }
        case 0x69A: //LSRI
        {
            CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
            MEM2WB.RegWrite = 1; 
            
            break;
        }
        case 0x694: //MOVZ
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x658: //SUB extended register
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x688: //SUB immediate
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x758: //SUBS extended register
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N;  
            break;
        }
        case 0x788: //SUBS immediate
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N;  
            break;
        }
        case 0x4D8: //MUL
        {
            if(MEM2WB.dest!=31){
                CURRENT_STATE.REGS[MEM2WB.dest] = MEM2WB.result;
                MEM2WB.RegWrite = 1;
            }
            break;
        }
        case 0x6A2: //HLT
        {
            RUN_BIT = EX2MEM.result;
            break;
        }
        case 0x359: //CMP extended register
        {
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N;
            MEM2WB.RegWrite = 1; 
            break; 
        }
        case 0x1C4: //CMP immediate
        {
            CURRENT_STATE.FLAG_Z = MEM2WB.FLAG_Z; 
            CURRENT_STATE.FLAG_N = MEM2WB.FLAG_N; 
            MEM2WB.RegWrite = 1;
            break; 
        }
    }

    if (MEM2WB.opcode != 0){
        stat_inst_retire++;
        MEM2WB.opcode = 0;
    }
}

void pipe_stage_mem()
{

    if (memStallCycles > 0){
        if (memStallCycles == 1){
            cache_update(data_cache, EX2MEM.addy, 0, 0); 
            memStallCycles--; 
            printf("\n\nopcode = %04x\n", MEM2WB.opcode); 
            printf("\naddy = %04x\n", MEM2WB.addy); 
            printf("\nresult = %04x\n\n", MEM2WB.result); 
        }
        else{
            memStallCycles--; 
            return; 
        }
    }
    else{
        MEM2WB.opcode = EX2MEM.opcode; 
        MEM2WB.result = EX2MEM.result; 
        MEM2WB.dest = EX2MEM.dest; 
        MEM2WB.FLAG_N = EX2MEM.FLAG_N; 
        MEM2WB.FLAG_Z = EX2MEM.FLAG_Z;
        MEM2WB.addy = EX2MEM.addy;
            
        if (contains(d_instructions, sizeof(d_instructions)/sizeof(d_instructions[0]), EX2MEM.opcode)){
            memsetindex = (EX2MEM.addy & 0x1FE0) >> 5; // 12:5 bits
            memtag = (EX2MEM.addy & 0xFFFFFFFFFFFFF000) >> 12; 

            cache_hit = 0; 
            for (int i = 0; i < data_cache->ways; i++){
                if (data_cache->setlist[memsetindex].blocklist[i].valid
                && (data_cache->setlist[memsetindex].blocklist[i].tag == memtag)){
                    cache_hit = 1; 
                    cache_update(data_cache, EX2MEM.addy, 1, i); 
                }
            }
            if (cache_hit == 0){
                memStallCycles = 50; 
                return; 
            }
        }
    }

    
    switch(MEM2WB.opcode){
        
        case 0x7C2: //LDUR
        {
            //loading 64 bits of data, load from offset and address right after offset, and shift that by 32 
            MEM2WB.result = mem_read_32(MEM2WB.addy) | (((int64_t)(mem_read_32(MEM2WB.addy+4)))<<32);
            MEM2WB.RegWrite = 1;
            break; 
        } 
        case 0x1C2: //LDURB
        {
    
            //loading only a byte of data, load from offset, and with ff
            MEM2WB.result = mem_read_32(MEM2WB.addy)&0x000000ff;
            MEM2WB.RegWrite = 1;
            break;
        }
        case 0x3C2: //LDURH
        {
            //loading 2 bytes of data, load from offset, and with ffff
            MEM2WB.result = mem_read_32(MEM2WB.addy)&0x0000ffff;
            MEM2WB.RegWrite = 1;
            break;
        }
        case 0x7C0: //STUR
        {
            mem_write_32(MEM2WB.addy, MEM2WB.result);
            mem_write_32(MEM2WB.addy+4, MEM2WB.result>>32);
            break;
        }
        case 0x1C0: //STURB
            {
            mem_write_32(MEM2WB.addy, MEM2WB.result);
            break;
        }
        case 0x3C0: //STURH
        {
            mem_write_32(MEM2WB.addy, MEM2WB.result);
            break;
        }
    }
}

void pipe_stage_execute()
{
    if (memStallCycles > 0){
        return; 
    }
    // if(stallCycles > 2){
    //     stallCycles--
    //     return;
    // }
    // if(ID2EX.stall == 1){
    //     return;
    // } else {
        //EX2MEM.dest = ID2EX.dest; 
		//forwarding:
    if(EX2MEM.RegWrite == 1 && EX2MEM.dest == ID2EX.regA && ID2EX.opcode != 0x54){
        
        // EX2MEM.forwardResult1 = 2;
        ID2EX.A = EX2MEM.result;
        //first register data comes from current EX2MEM registers
    }
    if(EX2MEM.RegWrite == 1 && EX2MEM.dest == ID2EX.regB){
        // EX2MEM.forwardResult2 = 2;
        ID2EX.B = EX2MEM.result;
        //second register data comes from current EX2MEM registers
    }
    if(EX2MEM.flagWrite == 1){
        ID2EX.FLAG_Z = EX2MEM.FLAG_Z; 
        ID2EX.FLAG_N = EX2MEM.FLAG_N; 
    }
    //else
    if(MEM2WB.RegWrite == 1 && MEM2WB.dest == ID2EX.regA)
    {
        
        // EX2MEM.forwardResult1 = 1;
        //first register data comes from current MEM2WB registers
    }
    // if(MEM2WB.RegWrite == 1 && MEM2WB.dest == ID2EX.regB && (EX2MEM.dest != ID2EX.regB || EX2MEM.RegWrite == 0))
    // {
    //     // EX2MEM.forwardResult2 = 1;
    //     ID2EX.B = MEM2WB.result; 
    //     printf("balls2"); 
    //     printf("\n\ndest = %04x\n", MEM2WB.dest); 
    //     //second register data comes from current MEM2WB registers
    // } 

    EX2MEM.opcode = ID2EX.opcode; 
    EX2MEM.dest = ID2EX.dest; 
    EX2MEM.imm = ID2EX.imm; 
    EX2MEM.FLAG_N = ID2EX.FLAG_N; 
    EX2MEM.FLAG_Z = ID2EX.FLAG_Z; 

    EX2MEM.RegWrite = 0; 
    EX2MEM.flagWrite = 0;

    switch(EX2MEM.opcode){
        case 0x458: //ADD extended register
        {
            EX2MEM.result = ID2EX.A + ID2EX.B;
            EX2MEM.RegWrite = 1; 
            break;
        }
        case 0x488: //ADD immediate
        {
            EX2MEM.result = ID2EX.A + ID2EX.imm;
            EX2MEM.RegWrite = 1; 
            break;
        }
        case 0x558: //ADDS extended register
        {
            EX2MEM.result = ID2EX.A + ID2EX.B;
            EX2MEM.RegWrite = 1; 
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break;
        }
        case 0x588: //ADDS immediate
        {
            EX2MEM.result = ID2EX.A + ID2EX.imm;
            EX2MEM.RegWrite = 1; 
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break;
        }
        case 0xB5: //CBNZ
        {
            if(ID2EX.A!=0){
                EX2MEM.result = ID2EX.PC + ((ID2EX.imm)*4);
                //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
            } else{
                EX2MEM.result = ID2EX.PC + 4;
                //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
            }
            if (EX2MEM.result != ID2EX.predicted_PC){
                
                bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                flush(EX2MEM.result);
            } else{
                bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
            }
            break;
        }
        case 0xB4: //CBZ
        {
            if(ID2EX.A==0){
                EX2MEM.result = ID2EX.PC + (ID2EX.imm*4);
                //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
            } else{
                EX2MEM.result = ID2EX.PC + 4;
                //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
            }
            if (EX2MEM.result != ID2EX.predicted_PC){
                bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                flush(EX2MEM.result);
            } else{
                bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
            }
            break;
        }
        case 0x450: //AND
        {
            if(ID2EX.dest!=31){
                EX2MEM.result = ID2EX.A & ID2EX.B;
                EX2MEM.RegWrite == 1;
            }
            break;
        }
        case 0x750: //ANDS
        {
            EX2MEM.result = ID2EX.A & ID2EX.B;
            EX2MEM.RegWrite == 1;
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break;
        }
        case 0x650: //EOR
        {
            EX2MEM.result = ID2EX.A & ID2EX.B;
            EX2MEM.RegWrite == 1;
            break;
        }
        case 0x550: //ORR
        {
            EX2MEM.result = ID2EX.A | ID2EX.B;
            EX2MEM.RegWrite == 1;
            break;
        }
        // if it is a load, we set MemRead = 1;
        case 0x7C2: //LDUR
        {
            if (ID2EX.imm & 0x0000000000000100){
                ID2EX.imm = ID2EX.imm | 0xffffffffffffff00; 
            }
            //loading 64 bits of data, load from offset and address right after offset, and shift that by 32 
            EX2MEM.addy = ID2EX.A + ID2EX.imm; 
            EX2MEM.RegWrite = 1; 
            break; 
        } 
        case 0x1C2: //LDURB
        {
            if (ID2EX.imm & 0x0000000000000100){
                ID2EX.imm = ID2EX.imm | 0xffffffffffffff00; 
            }
            //loading 64 bits of data, load from offset and address right after offset, and shift that by 32 
            EX2MEM.addy = ID2EX.A + ID2EX.imm; 
            EX2MEM.RegWrite = 1; 
            break; 
        }
        case 0x3C2: //LDURH
        {
            if (ID2EX.imm & 0x0000000000000100){
                ID2EX.imm = ID2EX.imm | 0xffffffffffffff00; 
            }
            //loading 64 bits of data, load from offset and address right after offset, and shift that by 32 
            EX2MEM.addy = ID2EX.A + ID2EX.imm; 
            EX2MEM.RegWrite = 1; 
            break; 
        }
        case 0x69B: //LSLI
        {
            if ((ID2EX.imm & 0x3F) != 0x3F){
                EX2MEM.result = ID2EX.A << (63 - (ID2EX.imm));
                EX2MEM.RegWrite == 1;
            }
            else {
                EX2MEM.result = ID2EX.A;
            }
            break;
        }
        case 0x69A: //LSRI
        {
            if ((ID2EX.imm & 0x3F) == 0x3F){
                EX2MEM.result = ID2EX.A >> (ID2EX.imm);
                EX2MEM.RegWrite == 1;
            }
            else {
                EX2MEM.result = ID2EX.A;
            }
            break;
        }
        case 0x694: //MOVZ
        {
            EX2MEM.result = ID2EX.imm;
            EX2MEM.RegWrite = 1; 
            break;
        }
        case 0x7C0: //STUR
        {
            if (ID2EX.imm & 0x0000000000000100) {
                ID2EX.imm = ID2EX.imm | 0x1111111111100; 
            }
            EX2MEM.result = ID2EX.B;
            EX2MEM.addy = ID2EX.A + ID2EX.imm; 
            break;
        }
        case 0x1C0: //STURB
        {
            if (ID2EX.imm & 0x0000000000000100) {
                ID2EX.imm = ID2EX.imm | 0x1111111111100; 
            }
            EX2MEM.result = ID2EX.B&0xFF;
            EX2MEM.addy = ID2EX.A + ID2EX.imm; 
            break;
        }
        case 0x3C0: //STURH
        {
            if (ID2EX.imm & 0x0000000000000100) {
                ID2EX.imm = ID2EX.imm | 0x1111111111100; 
            }
            EX2MEM.result = ID2EX.B&0xFFFF;
            EX2MEM.addy = ID2EX.A + ID2EX.imm; 
            break;
        }
        case 0x658: //SUB extended register
        {
            EX2MEM.result = ID2EX.A - ID2EX.B;
            EX2MEM.RegWrite = 1; 
            break;
        }
        case 0x688: //SUB immediate
        {
            EX2MEM.result = ID2EX.A - ID2EX.imm;
            EX2MEM.RegWrite = 1; 
            break;
        }
        case 0x758: //SUBS extended register
        {
            EX2MEM.result = ID2EX.A - ID2EX.B;
            EX2MEM.RegWrite = 1; 
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break;
        }
        case 0x788: //SUBS immediate
        {
            EX2MEM.result = ID2EX.A - ID2EX.imm;
            EX2MEM.RegWrite = 1; 
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break;
        }
        case 0x4D8: //MUL
        {
            EX2MEM.result = ID2EX.A * ID2EX.B;
            EX2MEM.RegWrite = 1; 
            break;
        }
        case 0x6A2: //HLT
        {
            EX2MEM.result = 0; 
            break;
        }
        case 0x359: //CMP extended register
        {
            EX2MEM.result = ID2EX.A - ID2EX.B; 
            
            EX2MEM.RegWrite = 1;
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break; 
        }
        case 0x1C4: //CMP immediate
        {
            
            EX2MEM.result = ID2EX.imm - ID2EX.A; 
            EX2MEM.RegWrite = 1;
            EX2MEM.flagWrite = 1; 
            if(EX2MEM.result==0){
                EX2MEM.FLAG_Z = 1;
                EX2MEM.FLAG_N = 0;
            }
            else if(EX2MEM.result<0){
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 1;
            } else {
                EX2MEM.FLAG_Z = 0;
                EX2MEM.FLAG_N = 0;
            }
            break; 
        }
        case 0x5: //B
        {
            EX2MEM.result = ID2EX.PC + ID2EX.imm*4; 
            //bp_update(ID2EX.PC, 0, 1, EX2MEM.result);
            if (EX2MEM.result != ID2EX.predicted_PC){
                bp_update(ID2EX.PC, 0, 0, EX2MEM.result);
                flush(EX2MEM.result);
            } else{
                bp_update(ID2EX.PC, 0, 1, EX2MEM.result);
            }
            break; 
        }
        case 0x6B0: //BR
        {
            EX2MEM.result = ID2EX.A; 
            //bp_update(ID2EX.PC, 0, 1, EX2MEM.result);
            if (EX2MEM.result != ID2EX.predicted_PC){
                bp_update(ID2EX.PC, 0, 0, EX2MEM.result);
                flush(EX2MEM.result);
            }else{
                bp_update(ID2EX.PC, 0, 1, EX2MEM.result);
            }
            break;
        }
        case 0x54: //B.cond
        {
            int64_t cond = ID2EX.A;
            switch(cond){
                
                //BEQ
                case(0): 
                    if (ID2EX.FLAG_Z == 1)
                    {EX2MEM.result = ID2EX.PC + ID2EX.imm*4;
                    //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }else{
                        EX2MEM.result = ID2EX.PC + 4;
                        //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                    }
                    if (EX2MEM.result != ID2EX.predicted_PC){
                        bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                        flush(EX2MEM.result);
                    } else{
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    break;
                //BNE
                case(1):
                    if (ID2EX.FLAG_Z == 0)
                    {EX2MEM.result = ID2EX.imm*4;
                    }else{
                        EX2MEM.result = ID2EX.PC + 4;
                        bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                    }

                    if (EX2MEM.result != ID2EX.predicted_PC){
                        bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                        flush(EX2MEM.result);
                    }else{
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    break;
                //BGE
                case(10):
                    if ((ID2EX.FLAG_Z == 1) || (ID2EX.FLAG_N == 0))
                    {EX2MEM.result = ID2EX.imm*4;
                    //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }else{
                        EX2MEM.result = ID2EX.PC + 4;
                        //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                    }

                    if (EX2MEM.result != ID2EX.predicted_PC){
                        bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                        flush(EX2MEM.result);
                    } else {
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    break;
                //BLT
                case(11):
                    if ((ID2EX.FLAG_N == 1) && (ID2EX.FLAG_Z == 0))
                    {EX2MEM.result = ID2EX.imm*4;
                    //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }else{
                        EX2MEM.result = ID2EX.PC + 4;
                        //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                    }

                    if (EX2MEM.result != ID2EX.predicted_PC){
                        bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                        flush(EX2MEM.result);
                    } else {
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    break;
                //BGT
                case(12):
                //BGT IS MORE CORRECT HERE THAN THE OTHER BRANCHES, FORMAT THE REST LIKE THIS
                    if ((ID2EX.FLAG_N == 0) && (ID2EX.FLAG_Z == 0))
                    {
                        int temp = (int)ID2EX.imm; 
                        EX2MEM.result = ID2EX.PC + temp*4;
                    //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    else{
                        EX2MEM.result = ID2EX.PC + 4;
                        //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                    }
                    if (EX2MEM.result != ID2EX.predicted_PC){
                        
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                        flush(EX2MEM.result);
                    } else{
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    break;
                //BLE
                case(13):
                    if ((ID2EX.FLAG_Z == 1) || (ID2EX.FLAG_N == 1))
                    {EX2MEM.result = ID2EX.imm*4;
                    //bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }else{
                        EX2MEM.result = ID2EX.PC + 4;
                        //bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                    }

                    if (EX2MEM.result != ID2EX.predicted_PC){
                        bp_update(ID2EX.PC, 1, 0, EX2MEM.result);
                        flush(EX2MEM.result);
                    } else{
                        bp_update(ID2EX.PC, 1, 1, EX2MEM.result);
                    }
                    break;
                    }
            break;
        }
    }
	
}

void pipe_stage_decode()
{
    if (memStallCycles > 0){
        return; 
    }
    // if(stallCycles > 1){
    //     stallCycles--;
    //     return;
    // }
    // if(IF2ID.stall == 0 && IF2ID.instr != 0){
	// 	uint64_t tempA = ID2EX.A;
    // uint64_t tempB = ID2EX.B;
    
    if(contains(b_instructions, sizeof(b_instructions)/sizeof(b_instructions[0]), IF2ID.instr >> 26)||
              contains(cb_instructions, sizeof(cb_instructions)/sizeof(cb_instructions[0]), IF2ID.instr >> 24)){
        ID2EX.PC = IF2ID.PC; 
        ID2EX.FLAG_N = CURRENT_STATE.FLAG_N; 
        ID2EX.FLAG_Z = CURRENT_STATE.FLAG_Z; 
        ID2EX.branch_stall = 1;
		ID2EX.branching = 1;
        ID2EX.predicted_PC = IF2ID.predicted_PC;
        //if contains branch instruction, we will have to stall
        //perform branch decode here
              } else{
				  ID2EX.branching = 0;
			  }
    // checks if the instruction is a b instruction 
    if (contains(b_instructions, sizeof(b_instructions)/sizeof(b_instructions[0]), IF2ID.instr >> 26)){
        ID2EX.opcode = IF2ID.instr >> 26; 
        ID2EX.imm = IF2ID.instr & 0x3FFFFFF; 
        if (ID2EX.imm & 0x2000000){
                ID2EX.imm |= 0xFC000000; 
            }
    }
    // checks if the instruction is a cb instruction 
    else if (contains(cb_instructions, sizeof(cb_instructions)/sizeof(cb_instructions[0]), IF2ID.instr >> 24)){
        ID2EX.opcode = IF2ID.instr >> 24; 
        ID2EX.imm = (IF2ID.instr >> 5) & 0x7FFFF; 
        //for cb i looked on the pdf and i think we need to shift by 5 and & with 19 bits to get the immediate
        // ID2EX.A = CURRENT_STATE.REGS[IF2ID.instr & 0x1F];
        ID2EX.A = IF2ID.instr & 0xF; 
        if (ID2EX.imm & 0x40000){
                ID2EX.imm |= 0xFFF80000; 
            }
    }
        // checks if the instruction is an i instruction 
    else if (contains(i_instructions, sizeof(i_instructions)/sizeof(i_instructions[0]), IF2ID.instr >> 21)){
        ID2EX.opcode = IF2ID.instr >> 21; 
        ID2EX.imm = (IF2ID.instr >> 10) & 0xFFF; 
        ID2EX.A = CURRENT_STATE.REGS[(IF2ID.instr >> 5) & 0x1F]; 
        ID2EX.regA = (IF2ID.instr >> 5) & 0x1F;
        ID2EX.dest = IF2ID.instr & 0x1F; 
		ID2EX.branch_stall = 0;
    }
    // checks if the instruction is a d instruction 
    else if (contains(d_instructions, sizeof(d_instructions)/sizeof(d_instructions[0]), IF2ID.instr >> 21)){
        ID2EX.opcode = IF2ID.instr >> 21; 
        ID2EX.imm = (IF2ID.instr >> 12) & 0x1FF; 
        ID2EX.A = CURRENT_STATE.REGS[(IF2ID.instr >> 5) & 0x1F]; 
        ID2EX.B = CURRENT_STATE.REGS[IF2ID.instr & 0x1F];
        ID2EX.regA = (IF2ID.instr >> 5) & 0x1F;
        ID2EX.regB = IF2ID.instr & 0x1F;
        ID2EX.dest = IF2ID.instr & 0x1F;
		ID2EX.branch_stall = 0;
		//ID2EX.branching = 0;
        
        // this is a subset of d that is loads, here we set memread to 1 so we know to stall later
        if(contains(load_instructions, sizeof(d_instructions)/sizeof(load_instructions[0]), IF2ID.instr >> 21)){
            ID2EX.MemRead = 1;
        } 
        if (ID2EX.imm&0x0000000000000100){ 
                ID2EX.imm = (ID2EX.imm|0xffffffffffffff00); 
            } 
    }
    // checks if the instruction is an iw instruction 
    else if (contains(iw_instructions, sizeof(iw_instructions)/sizeof(iw_instructions[0]), IF2ID.instr >> 21)){
        ID2EX.opcode = IF2ID.instr >> 21; 
        ID2EX.imm = (IF2ID.instr >> 5) & 0xFFFF; 
        ID2EX.dest = IF2ID.instr & 0x1F; 
		ID2EX.branch_stall = 0;
    }
    // checks if the instruction is an r instruction 
    else if (contains(r_instructions, sizeof(r_instructions)/sizeof(r_instructions[0]), IF2ID.instr >> 21)){
        ID2EX.opcode = IF2ID.instr >> 21; 
        ID2EX.A = CURRENT_STATE.REGS[(IF2ID.instr >> 16) & 0x1F];  
        ID2EX.regA = (IF2ID.instr >> 16) & 0x1F;
        ID2EX.imm = (IF2ID.instr >> 10) & 0x3F; 
        ID2EX.B = CURRENT_STATE.REGS[(IF2ID.instr >> 5) & 0x1F]; 
        ID2EX.regB = (IF2ID.instr >> 5) & 0x1F;
        ID2EX.dest = IF2ID.instr & 0x1F;    
		ID2EX.branch_stall = 0;
    }
    else{
        ID2EX.opcode = 0; 
        ID2EX.A = 0;  
        ID2EX.regA = 0; 
        ID2EX.imm = 0;  
        ID2EX.B = 0; 
        ID2EX.regB = 0; 
        ID2EX.dest = 0; 
		ID2EX.branch_stall = 0;
    }

    // if (MEM2WB.RegWrite && MEM2WB.dest == ID2EX.regA && ID2EX.opcode != 0x54 && MEM2WB.opcode != 0x54){
    //     ID2EX.A = MEM2WB.result; 
    //     printf("\nA = %04x\n", ID2EX.A); 
    // }
    // if (MEM2WB.RegWrite && MEM2WB.dest == ID2EX.regB && ID2EX.opcode != 0x54 && MEM2WB.opcode != 0x54){
    //     ID2EX.B = MEM2WB.result; 
    //     printf("\nB = %04x\n", ID2EX.B); 
    // }
}

void pipe_stage_fetch()
{
    if (memStallCycles > 0 && fetchStallCycles < 2){
        return; 
    }

    if(fetchStallCycles > 0 ){
        if (fetchStallCycles == 1){
            IF2ID.instr = mem_read_32(CURRENT_STATE.PC); 
            cache_update(instruction_cache, CURRENT_STATE.PC, 0, 0); 
            fetchStallCycles --; 
        }
        else{
            fetchStallCycles--;
            return;
        }
    }
    else if(!finished && !fetchflushed){
        IF2ID.PC = CURRENT_STATE.PC; 

        fetchsetindex = (IF2ID.PC & 0x7E0) >> 5; // 10:5 bits
        fetchtag = (IF2ID.PC & 0xFFFFFFFFFFFFFC00) >> 10; 
        cache_hit = 0; 

        for (int i = 0; i < instruction_cache->ways; i++){
            // printf("\n\n valid = %04x ", instruction_cache->setlist[fetchsetindex]->blocklist[i]->valid ); 
            // printf("\n\n tag = %04x ", instruction_cache->setlist[fetchsetindex]->blocklist[i]->tag ); 
            // printf("\n\n fetchtag = %04x ", fetchtag ); 
            // printf("\n\n stallcycles = %04x\n\n", fetchStallCycles); 

            if (instruction_cache->setlist[fetchsetindex].blocklist[i].valid 
            && (fetchtag == instruction_cache->setlist[fetchsetindex].blocklist[i].tag)){
                IF2ID.instr = instruction_cache->setlist[fetchsetindex].blocklist[i].data[(IF2ID.PC & 0x1F) / 4]; 
                cache_hit = 1; 
                cache_update(instruction_cache, CURRENT_STATE.PC, 1, i); 
            }
        
        }

        if (cache_hit == 0){
            fetchStallCycles = 50; 
            return; 
        }
    }
    
     

    if (!finished && !fetchflushed){
        if (IF2ID.instr == 0){
            finished = 1; 
        }

        bp_predict(CURRENT_STATE.PC);
        //need yet to deal with incorrect bp predict
        CURRENT_STATE.PC = nextPC;
        IF2ID.predicted_PC = nextPC;
    }
    else{
        fetchflushed = 0; 
    }

    if(!finished){
        flushed = 0;
    }
}