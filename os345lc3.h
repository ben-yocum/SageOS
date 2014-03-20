// os345lc3.h
#ifndef __os345lc3_h__
#define __os345lc3_h__
// ---------------------------------------------------------------------
// TODO: Halt on errors
//
// LC3_DEBUG   x01
//             x02
//             x04
//             x08   paging

#define LC3_DEBUG				0
#define LC3_MAX_HEX_SIZE   8192
#define LC3_TID				curTask

// maximum LC-3 memory (2^16) words
#define LC3_MAX_MEMORY 65536
// frame/page size (2^6) words
#define LC3_FRAME_SIZE 64
// max frames in LC3 (2^16 - 2^6 = 2^10)
#define LC3_FRAMES     1024

#define LC3_FBT     0x2000
#define LC3_RPT     0x2400
#define LC3_RPT_END 0x3000
#define LC3_MEM     0x3000
#define LC3_MEM_END 0x10000

#define LC3_MAX_PAGE        (LC3_FRAMES<<3)
#define LC3_MAX_SWAP_MEMORY (LC3_MAX_PAGE<<6)

#define INSTRUCTIONS_PER_SWAP	10

// ---------------------------------------------------------------------

#define LC3_FBT_FRAME     (LC3_FBT>>6)
#define LC3_RPT_FRAME     (LC3_RPT>>6)
#define LC3_RPT_END_FRAME (LC3_RPT_END>>6)
#define LC3_MEM_FRAME     (LC3_MEM>>6)
#define LC3_MEM_END_FRAME (LC3_MEM_END>>6)

// parts of a virtual address
#define RPTI(va)          (((va)&BITS_15_11_MASK)>>10)
#define UPTI(va)          (((va)&BITS_10_6_MASK)>>5)
#define FRAMEOFFSET(va)   ((va)&BITS_5_0_MASK)

// definitions within a root or user table page
#define DEFINED(e1)       ((e1)&BIT_15_MASK)
#define DIRTY(e1)         ((e1)&BIT_14_MASK)
#define REFERENCED(e1)    ((e1)&BIT_13_MASK)
#define PINNED(e1)        ((e1)&BIT_12_MASK)
#define FRAME(e1)         ((e1)&BITS_9_0_MASK)
#define PAGED(e2)         ((e2)&BIT_15_MASK)
#define SWAPPAGE(e2)      ((e2)&BITS_12_0_MASK)

#define MEMWORD(a)        (memory[a])
#define MEMLWORD(a)       ((memory[a]<<16)+memory[(a)+1])

#define SET_DEFINED(e1)   ((e1)|BIT_15_MASK)
#define SET_DIRTY(e1)     ((e1)|BIT_14_MASK)
#define SET_REF(e1)       ((e1)|BIT_13_MASK)
#define SET_PINNED(e1)    ((e1)|BIT_12_MASK)
#define SET_PAGED(e2)     ((e2)|BIT_15_MASK)

#define CLEAR_DEFINED(e1) ((e1)&~BIT_15_MASK)
#define CLEAR_DIRTY(e1)   ((e1)&~BIT_14_MASK)
#define CLEAR_REF(e1)     ((e1)&~BIT_13_MASK)
#define CLEAR_PINNED(e1)  ((e1)&~BIT_12_MASK)

// ---------------------------------------------------------------------
// Bit Masks
#define BITS_15_12_MASK	0x0000f000
#define BITS_11_9_MASK	0x00000e00
#define BITS_8_6_MASK	0x000001c0

#define BIT_4_MASK		0x00000010
#define BIT_5_MASK		0x00000020
#define BIT_8_MASK		0x00000100
#define BIT_9_MASK		0x00000200
#define BIT_10_MASK		0x00000400
#define BIT_11_MASK		0x00000800
#define BIT_12_MASK		0x00001000
#define BIT_13_MASK		0x00002000
#define BIT_14_MASK		0x00004000
#define BIT_15_MASK		0x00008000

#define BITS_2_0_MASK	0x00000007
#define BITS_3_0_MASK	0x0000000f
#define BITS_4_0_MASK	0x0000001f
#define BITS_5_0_MASK	0x0000003f
#define BITS_7_0_MASK	0x000000ff
#define BITS_8_0_MASK	0x000001ff
#define BITS_10_0_MASK	0x000007ff
#define BITS_12_0_MASK	0x00001fff
#define BITS_15_0_MASK	0x0000ffff

#define BITS_15_11_MASK	0x0000f800
#define BITS_10_6_MASK	0x000007c0
#define BITS_9_0_MASK	0x000003ff

#define MASKTO8BITS(v) (BITS_7_0_MASK&(v))
#define MASKTO16BITS(v) (BITS_15_0_MASK&(v))

// ---------------------------------------------------------------------
// Error Messages
#define MEM_OUT_OF_BOUNDS_MSG "\nERROR: memory address %#x out range"
#define TRAP_ERROR_MSG        "\nERROR: Unknown trap vector %#x"
#define UNDEFINED_OPCODE_MSG  "\nERROR: Unkown opcode %#x"

// ---------------------------------------------------------------------
// Debug Messages
#define ADD_MSG               "\n%04x ADD R%d,R%d,R%d"
#define ADDI_MSG              "\n%04x ADD R%d,R%d,%#x"
#define AND_MSG               "\n%04x AND R%d,R%d,R%d"
#define ANDI_MSG              "\n%04x AND R%d,R%d,%#x"
#define BR_MSG                "\n%04x BR%s %#x"
#define LD_MSG                "\n%04x LD R%d,%#x"
#define LDI_MSG               "\n%04x LDI R%d,%#x"
#define LDR_MSG               "\n%04x LDR R%d,R%d,%#x"
#define ST_MSG                "\n%04x ST R%d,%#x"
#define STI_MSG               "\n%04x STI R%d,%#x"
#define STR_MSG               "\n%04x STR R%d,R%d,%#x"
#define LEA_MSG               "\n%04x LEA R%d,%#x"
#define NOT_MSG               "\n%04x NOT R%d,R%d"
#define JSR_MSG               "\n%04x JSR %#x"
#define JSRR_MSG              "\n%04x JSRR R%d"
#define JMP_MSG               "\n%04x JMP R%d"
#define RET_MSG               "\n%04x RET"
#define TRAP_MSG              "\n%04x %s"


// return trap vector bits [7-0] (BITS_7_0_MASK) of ir in range (0-255)
#define GET_TRAPVECT8 (ir&BITS_7_0_MASK)
// return DR bits [11-9] (BITS_11_9_MASK) of ir in range (0-7)
#define GET_DR ((ir&BITS_11_9_MASK)>>9)
// return SR bits [11-9] (BITS_11_9_MASK) of ir in range (0-7)
#define GET_SR ((ir&BITS_11_9_MASK)>>9)
// return op-code bits [15-12] (BITS_15_12_MASK) of ir in range (0-16)
#define GET_OPCODE ((ir&BITS_15_12_MASK)>>12)
// return SR1 bits [8-6] (BITS_8_6_MASK) of ir in range (0-7)
#define GET_SR1 ((ir&BITS_8_6_MASK)>>6)
// return SR2 bits [2-0] (BITS_2_0_MASK) of ir in range (0-7)
#define GET_SR2 (ir&BITS_2_0_MASK)
// return Base Register bits [8-6] (BITS_8_6_MASK) of ir in range (0-7)
#define GET_BASER ((ir&BITS_8_6_MASK)>>6)
// return Immediatge bit [5] (BIT_5_MASK) of ir in range (0-1)
#define GET_IMMEDIATE_BIT ((ir&BIT_5_MASK)>>5)
// return Addressing Mode bit [11] (BIT_11_MASK) of ir in range (0-1)
#define GET_ADDR_BIT ((ir&BIT_11_MASK)>>11)
// return n condition code
#define GET_N ((ir&BIT_11_MASK)>>11)
// return z condition code
#define GET_Z ((ir&BIT_10_MASK)>>10)
// return p condition code
#define GET_P ((ir&BIT_9_MASK)>>9)

// sign extend bits[4-0], return range (-16 to 15)
#define SEXT5(v) (((v)&BIT_4_MASK)?((v)&BITS_4_0_MASK)|(-1<<4):(v)&BITS_4_0_MASK)
// sign extend bits[5-0], return range (-32 to 31)
#define SEXT6(v) (((v)&BIT_5_MASK)?((v)&BITS_5_0_MASK)|(-1<<5):(v)&BITS_5_0_MASK)
// sign extend bits[8-0], return range (-256 to 255)
#define SEXT9(v) (((v)&BIT_8_MASK)?((v)&BITS_8_0_MASK)|(-1<<8):(v)&BITS_8_0_MASK)
// sign extend bits[10-0], return range (-1024 to 1023)
#define SEXT11(v) (((v)&BIT_10_MASK)?((v)&BITS_10_0_MASK)|(-1<<10):(v)&BITS_10_0_MASK)
// sign extend bits[15-0], return range (-32768 to 32767)
#define SEXT16(v) (((v)&BIT_15_MASK)?((v)&BITS_15_0_MASK)|(-1<<15):(v)&BITS_15_0_MASK)

// set condition codes by contents of reg_address
#define SET_CC(v) LC3_CC=(((SEXT16(v)<0)?1:0)<<2)+((((v)==0)?1:0)<<1)+((SEXT16(v)>0)?1:0);

// ---------------------------------------------------------------------
// LC-3 Instructions
#define LC3_ADD  0x1
#define LC3_AND  0x5
#define LC3_BR   0x0
#define LC3_JMP  0xC     /* Note: JMP and RET have the same op-code       */
#define LC3_JSR  0x4     /* Note: JSR and JSRR have the same op-code      */
#define LC3_LD   0x2
#define LC3_LDI  0xA
#define LC3_LDR  0x6
#define LC3_LEA  0xE
#define LC3_NOT  0x9
#define LC3_RTI  0x8
#define LC3_ST   0x3
#define LC3_STI  0xB
#define LC3_STR  0x7
#define LC3_TRAP 0xF

// Define traps
#define LC3_GETID    0x0A
#define LC3_GETC     0x20
#define LC3_OUT      0x21
#define LC3_PUTS     0x22
#define LC3_IN       0x23
#define LC3_PUTSP    0x24
#define LC3_HALT     0x25

enum fmsTraps {   LC3_x30 = 0x30,			// TRAP x30
                  LC3_getNextDirEntry,		// TRAP x31
                  LC3_x32,						// TRAP x32
                  LC3_x33,						// TRAP x33
                  LC3_closeFile,          // TRAP x34
                  LC3_x35,						// TRAP x35
                  LC3_defineFile,         // TRAP x36
                  LC3_x37,						// TRAP x37
                  LC3_deleteFile,         // TRAP x38
                  LC3_openFile,           // TRAP x39
                  LC3_readFile,           // TRAP x3a
                  LC3_x3b,      		   	// TRAP x3b
                  LC3_seekFile,           // TRAP x3c
                  LC3_writeFile };        // TRAP x3d


typedef int PC;

// ---------------------------------------------------------------------
// prototypes
void initLC3Memory(int startFrame, int endFrame);

unsigned short int *getMemAdr(int va, int rwFlg);
int accessPage(int pnum, int frame, int rwnFlg);
void setFrameTableBits(int flg, int sf, int ef);
int getAvailableFrame();
void outPTE(char* s, int pte);

#endif // __os345lc3_h__

