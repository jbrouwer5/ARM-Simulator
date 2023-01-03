
/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */
#ifndef _PIPE_H_
#define _PIPE_H_
#include "shell.h"
#include "stdbool.h"
#include <limits.h>
// int b_instructions[] = {0x5};
// int cb_instructions[] = {0xB4, 0xB5, 0x54};
// int d_instructions[] = {0x7C2, 0x1C2, 0x3C2, 0x7C0, 0x1C0, 0x3C0};
// int i_instructions[] = {0x488, 0x588, 0x69A, 0x69B, 0x1C4, 0x688, 0x788};
// int iw_instructions[] = {0x694, 0x6A2};
// int r_instructions[] = {0x458, 0x558, 0x450, 0x750, 0x650, 0x550, 0x658, 0x758, 0x4D8, 0x359,0x6B0};
// int load_instructions[] = {0x7C2, 0x1C2, 0x3C2};
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
bool contains(); 
typedef struct CPU_State {
        /* register file state */
        int64_t REGS[ARM_REGS];
        int FLAG_N;        /* flag N */
        int FLAG_Z;        /* flag Z */
        /* program counter in fetch stage */
        uint64_t PC;
        
} CPU_State;
typedef struct IF2ID_t {
        uint32_t instr;
        uint64_t PC;
        uint64_t predicted_PC;
        int stall;
        int writeEnable;
} IF2ID_t;
typedef struct ID2EX_t {
        uint64_t PC;
        uint64_t predicted_PC;
        uint32_t opcode;
        uint64_t A;
        uint64_t B;
        uint32_t imm;
        uint32_t dest;
        uint64_t destVal;
        uint32_t Bimm;
        int MemtoReg;
        int MemRead;
        int MemWrite;
        int FLAG_N;
        int FLAG_Z;
        int regA;
        int regB;
        uint64_t forwardResult1;
        uint64_t forwardResult2;
        int stall;
        int branch_stall;
	int branching;
} ID2EX_t;
typedef struct EX2MEM_t {
        uint32_t opcode;
        uint64_t result;
        uint32_t dest;
        uint64_t imm;
        uint64_t addy;
        int MemtoReg;
        int RegRead;
        int RegWrite;
        int flagWrite; 
        int flagRead; 
        int MemRead;
        int MemWrite;
        int FLAG_Z;
        int FLAG_N;
        int stall;
        uint64_t predicted_PC;
} EX2MEM_t;
typedef struct MEM2WB_t {
        uint32_t opcode;
        uint64_t result;
        uint32_t dest;
        int MemtoReg;
        uint64_t addy;
        int RegRead;
        int RegWrite;
        int flagWrite; 
        int flagRead; 
        int MemRead;
        int MemWrite;
        int FLAG_Z;
        int FLAG_N;
        int stall;
} MEM2WB_t;
typedef struct PendMem_t {
        uint32_t opcode;
        uint64_t result;
        uint32_t dest;
        uint64_t imm;
        uint64_t addy;
        int MemtoReg;
        int MemRead;
        int MemWrite;
        int FLAG_Z;
        int FLAG_N;
        int stall;
} PendMem_t;
int RUN_BIT;
/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;
/* called during simulator startup */
void pipe_init();
/* this function calls the others */
void pipe_cycle();
/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();
#endif
