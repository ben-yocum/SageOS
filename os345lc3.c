// os345lc3.c - LC-3 Simulator
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345lc3.h"
#include "os345fat.h"

// ***********************************************************************
// lc3 variables
extern unsigned short int memory[];	// lc3 memory
extern char inChar;						// last entered character
extern int charFlag;						// 0 => buffered input
extern int curTask;						// current task #
extern Semaphore* charReady;			// character has been entered

// ***********************************************************************
// lc3 functions and tasks
int loadLC3Program(char**);
void initLC3Memory(int startFrame, int endFrame);
void checkMemoryBounds(int* address);
void setMemoryData(int address, int value);
int getMemoryData(int address);
int getCharacter(void);

extern unsigned short int *getMemAdr(int va, int rwFlg);
extern void setFrameTableBits(int flg, int sf, int ef);
extern int accessPage(int pnum, int frame, int rwnFlg);

// ***********************************************************************
// ***********************************************************************
// lc3 simulator task
//
//	argc = 2
//	argv[0] =	0 - load from system
//					1 - load from FAT
//
int lc3Task(int argc, char* argv[])
{
	int DR, oldpc, ir;   					// local variables
    int i;

	int LC3_REGS[8];							// General purpose registers
	int LC3_CC = 0x02;						// NZP condition codes
	int LC3_PC = 0x3000;						// program counter
	int ips = 0;								// instructions per swap

	// Initialize LC3 Simulator
	// clear registers
	for (i=0; i<8; i++) LC3_REGS[i] = 0;
   // set condition codes to nZp
	LC3_CC = 0x02;
	// load program
	if ((LC3_PC = loadLC3Program(argv)) < 0) return -1;

   // Execute LC3 program
	while(1)
	{
		oldpc = LC3_PC;					   // save old pc for debug message

		ir = getMemoryData(LC3_PC);		// load ir and increment pc
		LC3_PC = LC3_PC + 1;				   // increment program counter
		DR = GET_DR;					      // preload destination register #

		switch(GET_OPCODE)
		{
			case LC3_ADD:                 // ADD instruction
				if (!GET_IMMEDIATE_BIT)
				{	if (LC3_DEBUG&0x01) printf(ADD_MSG, oldpc, DR, GET_SR1, GET_SR2);
					LC3_REGS[DR] = MASKTO16BITS(LC3_REGS[GET_SR1] + LC3_REGS[GET_SR2]);
				}
				else
				{	if (LC3_DEBUG&0x01) printf(ADDI_MSG, oldpc, DR, GET_SR1, MASKTO16BITS(SEXT5(ir)));
					LC3_REGS[DR] = MASKTO16BITS(LC3_REGS[GET_SR1] + SEXT5(ir));
				}
	    		SET_CC(LC3_REGS[DR]);
				break;

			case LC3_AND:                 // AND instruction
				if (!GET_IMMEDIATE_BIT)
				{	if (LC3_DEBUG&0x01) printf(AND_MSG, oldpc, DR, GET_SR1, GET_SR2);
					LC3_REGS[DR] = MASKTO16BITS(LC3_REGS[GET_SR1] & LC3_REGS[GET_SR2]);
				}
				else
				{	if (LC3_DEBUG&0x01) printf(ANDI_MSG, oldpc, DR, GET_SR1, MASKTO16BITS(SEXT5(ir)));
					LC3_REGS[DR] = MASKTO16BITS(LC3_REGS[GET_SR1] & SEXT5(ir));
				}
	    		SET_CC(LC3_REGS[DR]);
	 			break;

			case LC3_BR:                  // BR instruction
	         if (ir == 0)
            {  printf("\n**(%d) Illegal instruction 0x%04x at 0x%04x (frame %d)", LC3_TID, ir, LC3_PC, LC3_PC>>6);
   		      return -1;                 // abort!
            }
				if ((LC3_CC&0x04 && GET_N) || (LC3_CC&0x02 && GET_Z) || (LC3_CC&0x01 && GET_P))
				   LC3_PC = LC3_PC + SEXT9(ir);
				if (LC3_DEBUG&0x01)
				{	char cchr[4];
					cchr[0] = 0;
					if (GET_N) strcat(cchr, "N");
					if (GET_Z) strcat(cchr, "Z");
					if (GET_P) strcat(cchr, "P");
					printf(BR_MSG, oldpc, cchr, LC3_PC);
				}
				break;

			case LC3_JMP:                 // JMP instruction
				if (LC3_DEBUG&0x01)
				{	if (GET_BASER == 7) printf(RET_MSG, oldpc);
					else printf(JMP_MSG, oldpc, GET_BASER);
				}
				LC3_PC = LC3_REGS[GET_BASER];
			   break;

			case LC3_JSR:                 // JSR instruction
				LC3_REGS[7] = MASKTO16BITS(LC3_PC);
				if (GET_ADDR_BIT == 0)
				{	if (LC3_DEBUG&0x01) printf(JSRR_MSG, oldpc, GET_BASER);
					LC3_PC = LC3_REGS[GET_BASER];
				}
				else
				{	if (LC3_DEBUG&0x01) printf(JSR_MSG, oldpc, LC3_PC + SEXT11(ir));
					LC3_PC = LC3_PC + SEXT11(ir);
				}
	    		break;

			case LC3_LD:                  // LD instruction
				if (LC3_DEBUG&0x01) printf(LD_MSG, oldpc, DR, SEXT9(ir));
				LC3_REGS[DR] = MASKTO16BITS(getMemoryData(LC3_PC + SEXT9(ir)));
				SET_CC(LC3_REGS[DR]);
			   break;

			case LC3_LDI:                 // LDI instruction
				if (LC3_DEBUG&0x01) printf(LDI_MSG, oldpc, DR, SEXT9(ir));
				LC3_REGS[DR] = MASKTO16BITS(getMemoryData(getMemoryData(LC3_PC + SEXT9(ir))));
				SET_CC(LC3_REGS[DR]);
			   break;

	 		case LC3_LDR:                 // LDR instruction
				if (LC3_DEBUG&0x01) printf(LDR_MSG, oldpc, DR, GET_BASER, SEXT6(ir));
				LC3_REGS[DR] = MASKTO16BITS(getMemoryData(LC3_REGS[GET_BASER] + SEXT6(ir)));
				SET_CC(LC3_REGS[DR]);
			   break;

			case LC3_LEA:                 // LEA instruction
				if (LC3_DEBUG&0x01) printf(LEA_MSG, oldpc, DR, LC3_PC + SEXT9(ir));
				LC3_REGS[DR] = MASKTO16BITS(LC3_PC + SEXT9(ir));
				SET_CC(LC3_REGS[DR]);
				break;

			case LC3_NOT:                 // NOT instruction
				if (LC3_DEBUG&0x01) printf(NOT_MSG, oldpc, DR, GET_SR1);
				LC3_REGS[DR] = MASKTO16BITS(~LC3_REGS[GET_SR1]);
				SET_CC(LC3_REGS[DR]);
				break;

			case LC3_ST:                  // ST instruction
				if (LC3_DEBUG&0x01) printf(ST_MSG, oldpc, GET_SR, LC3_PC + SEXT9(ir));
				setMemoryData(LC3_PC + SEXT9(ir), LC3_REGS[GET_SR]);
				break;

			case LC3_STI:                 // STI instruction
				if (LC3_DEBUG&0x01) printf(STI_MSG, oldpc, GET_SR, SEXT9(ir));
				setMemoryData(getMemoryData(LC3_PC + SEXT9(ir)), LC3_REGS[GET_SR]);
			   break;

			case LC3_STR:                 // STR instruction
				if (LC3_DEBUG&0x01) printf(STR_MSG, oldpc, GET_SR, GET_BASER, SEXT6(ir));
				setMemoryData(LC3_REGS[GET_BASER] + SEXT6(ir), LC3_REGS[GET_SR]);
			   break;

			case LC3_TRAP:                // TRAP instruction
			{
				int trapv = getMemoryData(GET_TRAPVECT8);		// access trap vector
				getMemoryData(trapv);							// access system routine
				switch(GET_TRAPVECT8)
				{  int tmp, string_address;
					case LC3_GETID:
               {
						// Note: This function not supported by simulator.
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "GETTID");
						//printf("\nLC3_TID = %d (%d)", LC3_TID, curTask);
	               LC3_REGS[0] = LC3_TID;
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						break;
               }
					case LC3_GETC:
               {
						// Note: This does not function quite like GETC.
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "GETC");
						LC3_REGS[0] = MASKTO8BITS(getCharacter());
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						break;
               }
					case LC3_OUT:
               {
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "OUT");
						putchar(LC3_REGS[0]);
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						break;
               }
					case LC3_PUTSP:
               {
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "PUTSP");
						putchar(' ');
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						break;
               }
					case LC3_PUTS:
               {
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "PUTS");
						string_address = LC3_REGS[0];
						while((tmp=getMemoryData(string_address++)) != 0) putchar(tmp);
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						break;
               }
					case LC3_IN:
               {
						// Note: This does not function quite like IN.
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "IN");
						putchar(':');
						LC3_REGS[0] = MASKTO8BITS(getCharacter());
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						break;
               }
					case LC3_HALT:
               {
						LC3_REGS[7] = MASKTO16BITS(LC3_PC);
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "HALT");
						printf("\nProcess #%d Halted at 0x%04x\n", LC3_TID, LC3_PC);
						charFlag = 0;				// release input
						return 0;
               }
					case LC3_getNextDirEntry:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = int *dirNum
                  //       R1 = char* mask
                  //       R2 = DirEntry* dirEntry
                  //       R3 = int cDir
                  // OUT:  R0 = 0-success, otherwise error
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsGetNextDirEntry");
						LC3_REGS[0] = fmsGetNextDirEntry((int*)getMemAdr(LC3_REGS[0], 1),       // int *dirNum
                                                   (char*)getMemAdr(LC3_REGS[1], 0),      // char* mask
                                                   (DirEntry*)getMemAdr(LC3_REGS[2], 1),  // DirEntry* dirEntry
                                                   (short int)LC3_REGS[3]);               // int cDir
						SWAP
						break;
               }

					case LC3_closeFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = int fileDescriptor
                  // OUT:  R0 = 0-success, otherwise error
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsCloseFile");
						LC3_REGS[0] = fmsCloseFile((short int)LC3_REGS[0]);       // int fileDescriptor
						SWAP
						break;
               }

					case LC3_defineFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = char* fileName
                  // OUT:  R0 = 0-success, otherwise error
                  char fileName[32];
                  char* s = (char*)getMemAdr(LC3_REGS[0], 0);
						int j = i = 0;
						if(!s[j]) j++;
						while((fileName[i++] = s[j])) j+=2;
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsCreateFile");
						LC3_REGS[0] = fmsDefineFile(fileName, 0);   // char* fileName
						SWAP
						break;
               }

					case LC3_deleteFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = char* fileName
                  // OUT:  R0 = 0-success, otherwise error
                  char fileName[32];
                  char* s = (char*)getMemAdr(LC3_REGS[0], 0);
						int j = i = 0;
						if(!s[j]) j++;
						while((fileName[i++] = s[j])) j+=2;
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsDeleteFile");
						LC3_REGS[0] = fmsDeleteFile(fileName);       // char* fileName
						SWAP
						break;
               }

					case LC3_openFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = char* fileName
                  //       R1 = int rwMode
                  // OUT:  R0 = 0-success, otherwise error
                  char fileName[32];
                  char* s = (char*)getMemAdr(LC3_REGS[0], 0);
						int j = i = 0;
						if(!s[j]) j++;
						while((fileName[i++] = s[j])) j+=2;
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsOpenFile");
						LC3_REGS[0] = fmsOpenFile(fileName,               // char* fileName
                                            (short int)LC3_REGS[1]); // int rwMode
						SWAP
						break;
               }

					case LC3_readFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = int fileDescriptor
                  //       R1 = char* buffer
                  //       R2 = int nBytes
                  // OUT:  R0 = 0-success, otherwise error
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsReadFile");
						LC3_REGS[0] = fmsReadFile((short int)LC3_REGS[0],              // int fileDescriptor
                                            (char*)getMemAdr(LC3_REGS[1], 0),    // char* buffer
                                            (short int)LC3_REGS[2]);             // int nBytes
						SWAP
						break;
               }

					case LC3_seekFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = int fileDescriptor
                  //       R1 = int index
                  // OUT:  R0 = 0-success, otherwise error
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsSeekFile");
						LC3_REGS[0] = fmsSeekFile((short int)LC3_REGS[0],              // int fileDescriptor
                                            (short int)LC3_REGS[1]);             // int index
						SWAP
						break;
               }

					case LC3_writeFile:
               {
						// Note: This function not supported by simulator.
                  // IN:   R0 = int fileDescriptor
                  //       R1 = char* buffer
                  //       R2 = int nBytes
                  // OUT:  R0 = 0-success, otherwise error
						if (LC3_DEBUG&0x01) printf(TRAP_MSG, oldpc, "fmsWriteFile");
						LC3_REGS[0] = fmsWriteFile((short int)LC3_REGS[0],              // int fileDescriptor
                                             (char*)getMemAdr(LC3_REGS[1], 0),    // char* buffer
                                             (short int)LC3_REGS[2]);             // int nBytes
						SWAP
						break;
               }

					default:
						printf(TRAP_ERROR_MSG, GET_TRAPVECT8);
						break;
				}
				break;
			}

			default:
				printf(UNDEFINED_OPCODE_MSG, GET_OPCODE);
		      return -1;                 // abort!
		}
		if (LC3_DEBUG&0x02)              // debug mode
		{	char cchr[4];
	 		//      \n--PC:3000 IR:193b Nzp - 0000 0001 0002 0003 0004 0005 0006 0007
			printf("\n--PC:%04x IR:%04x ",LC3_PC, ir);
			cchr[0] = (LC3_CC&0x04)?'N':'n';
			cchr[1] = (LC3_CC&0x02)?'Z':'z';
			cchr[2] = (LC3_CC&0x01)?'P':'p';
			cchr[3] = 0;
			printf("%s -", &cchr[0]);
			for (i=0; i<8; i++) printf(" %04x", LC3_REGS[i]);
			//getchar();					// enable to single step
		}
		// swap tasks every INSTRUCTIONS_PER_SWAP instructions
		if (ips++ > INSTRUCTIONS_PER_SWAP)
		{	ips = 0;
			SWAP
		}
	} // end while(1) execution loop
	return 0;
} // end LC3Task


// ***********************************************************************
// Memory operations
// ***********************************************************************

void initLC3Memory(int startFrame, int endFrame)
{
	int i;
   // zero all of memory
  	for (i=0; i<(LC3_MEM_END); i++) memory[i] = 0;
   // write trap vectors
	for (i=0; i<256; i++) memory[i] = i;
   // initialize frame table
   setFrameTableBits(0, startFrame, endFrame);
   // initialize paged memory
   accessPage(0, 0, PAGE_INIT);
	return;
} // end initLC3Memory


// ***********************************************************************
// Asserts that address is a valid memory address within the range [0,MAX_MEMORY].
void checkMemoryBounds(int* address)
{
   //lc3Debug = ((address>=0x6000) && (address<0x7000))?0x08:0;
	if (*address < 0 || *address > LC3_MAX_MEMORY)
	{
		printf(MEM_OUT_OF_BOUNDS_MSG, *address);
    	*address = 0;
	}
} // end checkMemoryBounds


// ***********************************************************************
// return memory data
int getMemoryData(int address)
{
	int memoryData;
	checkMemoryBounds(&address);
	memoryData = (int)*getMemAdr(address, 0);
	if (LC3_DEBUG&0x04) printf("\n  Get Memory[0x%04x] = 0x%04x", address, memoryData);
	return memoryData;
} // end getMemoryData


// ***********************************************************************
// set memory data
void setMemoryData(int address, int value)
{
	checkMemoryBounds(&address);
	*getMemAdr(address, 1) = value;
	if (LC3_DEBUG&0x04) printf("\n  Set Memory[0x%04x] = 0x%04x", address, value);
	return;
} // end setMemoryData


// ***********************************************************************
// get a character
int getCharacter()
{
	charFlag = 1;					// enable character inputs
	SEM_WAIT(charReady)
	if (inChar == 13) inChar = 10;	//if (inChar == CR) inChar = LF
	charFlag = 0;					// disable character inputs
	return inChar;
} // end waitOnChar


// ***********************************************************************
// load LC3 program
int loadLC3Program(char* argv[])
{
	int startPC = -1;
	int data, address, error, i, fd;
	char buf[32];
	FILE *infile;

  	printf("\nLoad \"%s\"", argv[1]);

	switch (INTEGER(argv[0]))
	{
		// load LC3 program using fopen/fscanf
		case 0:
		{
			printf(" from system");
   		if ((infile = fopen(argv[1], "r")) == NULL)
   		{
	   		printf("\nfopen was unsuccessful!");
				return -1;
   		}
  			fscanf(infile, "%x", &address);
  			startPC = address;
  			while (fscanf(infile, "%x", &data) != EOF)
  			{
   			setMemoryData(address++, data);
  			}
  			fclose(infile);
			break;
		}
		// load LC3 program using fmsOpenFile/fmsReadFile
		case 1:
		{
			printf(" from FAT");
			if ((fd = fmsOpenFile(argv[1], 0)) < 0)
			{
				fmsError(fd);
				return -1;
			}
			// read 1st line (load address)
			i = 0;
			while ((error = fmsReadFile(fd, &buf[i], 1) > 0) && (buf[i++] != '\n'));
			buf[i] = '\0';
			sscanf(buf, "%x", &address);
			startPC = address;

			// load program
			while (error > 0)
			{	// program read line
				i = 0;
				while ((error = fmsReadFile(fd, &buf[i], 1) > 0) && (buf[i++] != '\n'));
				if (i < 4) break;
				buf[i] = '\0';
				sscanf(buf, "%x", &data);
				setMemoryData(address++, data);
			}
			if ((error != ERR66) && error) fmsError(error);
			error = fmsCloseFile(fd);
			if (error) fmsError(error);
			break;
		}
		default:
		{
			printf("\nMemory NOT loaded!  Illegal file type=%d", INTEGER(argv[1]));
			return -1;
		}
	}
	printf("\nMemory loaded!  PC=0x%04x", (unsigned int)startPC);
	return startPC;
} // end loadLC3Program
