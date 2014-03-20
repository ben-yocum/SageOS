// os345p4.c - Virtual Memory
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

// ***********************************************************************
// project 4 variables
extern TCB tcb[];							// task control block
extern int curTask;						    // current task #

extern int memAccess;
extern int memHits;
extern int memPageFaults;
extern int nextPage;
extern int pageReads;
extern int pageWrites;

extern unsigned short int memory[];
extern int getMemoryData(int);

extern int nextrpt;
extern int nextupt;

// ***********************************************************************
// project 4 functions and tasks
void loadLC3File(char* string);
void dumpMemory(char *s, int sa, int ea);
void dumpVMemory(char *s, int sa, int ea);
void displayFrame(int f);
void displayRPT(int rptNum);
void displayUPT(int rptNum, int uptNum);
void displayPage(int pn);
void displayPT(int pta, int badr, int inc);
void lookVM(int va);

// *****************************************************************************
// project4 command
//
//
int P4_project4(int argc, char* argv[])					// project 5
{
	// initialize lc3 memory
	P4_initMemory(argc, argv);

	// start lc3 tasks
	loadLC3File("memtest.hex");
	loadLC3File("crawler.hex");

	loadLC3File("memtest.hex");
	loadLC3File("crawler.hex");

	loadLC3File("memtest.hex");
	loadLC3File("crawler.hex");

	return 0;
}


// **************************************************************************
// **************************************************************************
// ---------------------------------------------------------------------
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023)
//     / / / /     /                 /__________Swap page defined
//    / / / /     /                 //        __page # (0-4096)
//   / / / /     /                 //        /
//  / / / /     / 	             //        /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

// ---------------------------------------------------------------------
// **************************************************************************
// dm <sa>,<ea>
int P4_dumpLC3Mem(int argc, char* argv[])
{
	int sa, ea;

	printf("\nValidate arguments...");	// ?? validate arguments
	sa = INTEGER(argv[1]);
	ea = sa + 0x0040;

	dumpMemory("LC-3 Memory", sa, ea);
	return 0;
} // end P4_dumpLC3Mem



// **************************************************************************
// **************************************************************************
// vma <a>
int P4_vmaccess(int argc, char* argv[])
{
	unsigned short int adr, rpt, upt;

	printf("\nValidate arguments...");	// ?? validate arguments
	adr = INTEGER(argv[1]);

	printf(" = %04x", getMemAdr(adr, 1)-&MEMWORD(0));
	for (rpt = 0; rpt < 64; rpt+=2)
	{
		if (MEMWORD(rpt+TASK_RPT) || MEMWORD(rpt+TASK_RPT+1))
		{
			outPTE("  RPT  =", rpt+TASK_RPT);
			for(upt = 0; upt < 64; upt+=2)
			{
				if (DEFINED(MEMWORD(rpt+TASK_RPT)) &&
					(DEFINED(MEMWORD((FRAME(MEMWORD(rpt+TASK_RPT))<<6)+upt))
					|| PAGED(MEMWORD((FRAME(MEMWORD(rpt+TASK_RPT))<<6)+upt+1))))
				{
					outPTE("    UPT=", (FRAME(MEMWORD(rpt+TASK_RPT))<<6)+upt);
				}
			}
		}
	}
	printf("\nPages = %d", nextPage);
	return 0;
} // end P4_vmaccess



// **************************************************************************
// **************************************************************************
// pm <#>  Display page frame
int P4_dumpPageMemory(int argc, char* argv[])
{
	int page;

	printf("\nValidate arguments...");	// ?? validate arguments
	page = INTEGER(argv[1]);

	displayPage(page);
	return 0;
} // end P4_dumpPageMemory




/**
* Response to the command im <a>, where a is the number of frames
* Initializes memory
* @param argc
* @param argv
* @return 1 if unsuccessful, 0 if successful
*/
int P4_initMemory(int argc, char* argv[])
{
	int highAdr = 0x8000;

	//set task.RPT in createTask

	printf("\nValidate arguments...");	// ?? validate arguments
	if (!tcb[curTask].RPT)
	{
		printf("\nTask RPT Invalid!");
		return 1;
	}
	if (argc > 1) highAdr = INTEGER(argv[1]);
	if (highAdr < 0x3000) highAdr = (highAdr<<6) + 0x3000;
	if (highAdr > 0xf000) highAdr = 0xf000;
	printf("\nSetting upper memory limit to 0x%04x", highAdr);

	// init LC3 memory
	initLC3Memory(LC3_MEM_FRAME, highAdr>>6);
	printf("\nPhysical Address Space = %d frames (%0.1fkb)",
         (highAdr>>6)-LC3_MEM_FRAME, ((highAdr>>6)-LC3_MEM_FRAME)/8.0);

	memAccess = 0;							// vm statistics
	memHits = 0;
	memPageFaults = 0;
	nextPage = 0;
	pageReads = 0;
	pageWrites = 0;
	nextrpt=0;
	nextupt=0;

	return 0;
} // end P4_initMemory



// **************************************************************************
// **************************************************************************
// dvm <sa>,<ea>
int P4_dumpVirtualMem(int argc, char* argv[])	// dump virtual lc-3 memory
{
	int sa, ea;

	printf("\nValidate arguments...");	// ?? validate arguments
	sa = INTEGER(argv[1]);
	ea = sa + 0x0040;

	dumpVMemory("LC-3 Virtual Memory", sa, ea);
	lookVM(sa);
	return 0;
} // end P4_dumpVirtualMem



// **************************************************************************
// **************************************************************************
// vms
int P4_virtualMemStats(int argc, char* argv[])
{
	double missRate;
	missRate = (memAccess)?(((double)memPageFaults)/(double)memAccess)*100.0:0;
	printf("\nMemory accesses = %d", memAccess);
	printf("\n           hits = %d", memHits);
	printf("\n         faults = %d", memPageFaults);
	printf("\n           rate = %f%%", missRate);
	printf("\n     Page reads = %d", pageReads);
	printf("\n    Page writes = %d", pageWrites);
	printf("\nSwap page count = %d (%d kb)", nextPage, nextPage>>3);
	return 0;
} // end P4_virtualMemStats


// **************************************************************************
// **************************************************************************
// dft
int P4_dumpFrameTable(int argc, char* argv[])
{
	dumpMemory("Frame Bit Table", LC3_FBT, LC3_FBT+0x40);
	return 0;
} // end P4_dumpFrameTable



// **************************************************************************
// **************************************************************************
// dfm <frame>
int P4_dumpFrame(int argc, char* argv[])
{
	int frame;

	printf("\nValidate arguments...");	// ?? validate arguments
	frame = INTEGER(argv[1]);

	displayFrame(frame%LC3_FRAMES);
	return 0;
} // end P4_dumpFrame



// **************************************************************************
// **************************************************************************
// rpt <#>       Display process root page table
int P4_rootPageTable(int argc, char* argv[])
{
	int rpt;

	printf("\nValidate arguments...");	// ?? validate arguments
	rpt = INTEGER(argv[1]);

	displayRPT(rpt);
	return 0;
} // end P4_rootPageTable



// **************************************************************************
// **************************************************************************
// upt <p><#>    Display process user page table
int P4_userPageTable(int argc, char* argv[])
{
	int rpt, upt;

	printf("\nValidate arguments...");	// ?? validate arguments
	rpt = INTEGER(argv[1]);
	upt = INTEGER(argv[2]);

	displayUPT(rpt, upt>>11);
	return 0;
} // P4_userPageTable



// **************************************************************************
// **************************************************************************
void displayFrame(int f)
{
   char mesg[128];
   sprintf(mesg, "Frame %d", f);
   dumpMemory(mesg, f*LC3_FRAME_SIZE, (f+1)*LC3_FRAME_SIZE);
   return;
} // end displayFrame



// **************************************************************************
// **************************************************************************
// display contents of RPT rptNum
void displayRPT(int rptNum)
{
   displayPT(LC3_RPT + (rptNum<<6), 0, 1<<11);
   return;
} // end displayRPT



// **************************************************************************
// **************************************************************************
// display contents of UPT
void displayUPT(int rptNum, int uptNum)
{
   unsigned short int rpte, upt, uptba;
   //unsigned short int upte1, upte2;
   rptNum &= BITS_3_0_MASK;
   uptNum &= BITS_4_0_MASK;

   // index to process <rptNum>'s rpt + <uptNum> index
   rpte = MEMWORD(((LC3_RPT + (rptNum<<6)) + uptNum*2));
   // calculate upt's base address
   uptba = uptNum<<11;
   if (DEFINED(rpte)) upt = FRAME(rpte)<<6;
   else
   {  printf("\nUndefined!");
      return;
   }
   displayPT(upt, uptba, 1<<6);
   return;
} // end displayUPT



// **************************************************************************
// **************************************************************************
// output page table entry
void outPTE(char* s, int pte)
{
	int pte1, pte2;
	char flags[8];

	// read pt
	pte1 = memory[pte];
	pte2 = memory[pte+1];

	// look at appropriate flags
	strcpy(flags, "----");
	if (DEFINED(pte1)) flags[0] = 'F';
	if (DIRTY(pte1)) flags[1] = 'D';
	if (REFERENCED(pte1)) flags[2] = 'R';
	if (PINNED(pte1)) flags[3] = 'P';

	// output pte line
	printf("\n%s x%04x = %04x %04x  %s", s, pte, pte1, pte2, flags);
	if (DEFINED(pte1) || DEFINED(pte2)) printf(" Frame=%d", FRAME(pte1));
	if (DEFINED(pte2)) printf(" Page=%d", SWAPPAGE(pte2));

	return;
} // end outPTE



// **************************************************************************
// **************************************************************************
// display page table entries
void displayPT(int pta, int badr, int inc)
{
	int i;
	char buf[32];

	for (i=0; i<32; i++)
	{
      sprintf(buf, "(x%04x-x%04x) ", badr+ i*inc, badr + ((i+1)*inc)-1);
		outPTE("", (pta + i*2));
	}

   return;
} // end displayPT



// **************************************************************************
// **************************************************************************
// look at virtual memory location va
void lookVM(int va)
{
   unsigned short int rpte1, rpte2, upte1, upte2, pa;

   // get root page table entry
	rpte1 = MEMWORD(LC3_RPT + RPTI(va));
   rpte2 = MEMWORD(LC3_RPT + RPTI(va) + 1);
   if (DEFINED(rpte1))
   {	upte1 = MEMWORD((FRAME(rpte1)<<6) + UPTI(va));
		upte2 = MEMWORD((FRAME(rpte1)<<6) + UPTI(va) + 1);
   }
   else
   {
		// rpte undefined
		printf("\n  RTB[Undefined]");
		return;
	}
  	if (DEFINED(upte1))
	{
		pa = (FRAME(upte1)<<6) + FRAMEOFFSET(va);
	}
   else
   {
		// upte undefined
     	printf("\n  UTB[Undefined]");
		return;
   }
   printf("\n  RPT[0x%04x] = %04x %04x", LC3_RPT + RPTI(va), rpte1, rpte2);
      if (rpte1&BIT_14_MASK) printf(" D");
      if (rpte1&BIT_13_MASK) printf(" R");
      if (rpte1&BIT_12_MASK) printf(" P");
      printf(" Frame=%d", rpte1&0x03ff);
      if (DEFINED(rpte2)) printf(" Page=%d", rpte2&0x0fff);
   printf("\n  UPT[0x%04x] = %04x %04x", (FRAME(rpte1)<<6) + UPTI(va), upte1, upte2);
      if (upte1&BIT_14_MASK) printf(" D");
      if (upte1&BIT_13_MASK) printf(" R");
      if (upte1&BIT_12_MASK) printf(" P");
      printf(" Frame=%d", upte1&0x03ff);
      if (DEFINED(upte2)) printf(" Page=%d", upte2&0x0fff);
   printf("\n  MEM[0x%04x] = %04x", pa, MEMWORD(pa));
	return;
} // end lookVM



// **************************************************************************
// **************************************************************************
// pm <#>  Display page frame
void displayPage(int pn)
{
   short int *buffer;
   int i, ma;
   printf("\nPage %d", pn);
   buffer = (short int*)accessPage(pn, pn, 3);
   for (ma = 0; ma < 64;)
	{
      printf("\n0x%04x:", ma);
		for (i=0; i<8; i++)
		{
		   printf(" %04x", MASKTO16BITS(buffer[ma + i]));
      }
		ma+=8;
   }
   return;
} // end displayPage



// **************************************************************************
// **************************************************************************
// dm <sa> <ea> - dump lc3 memory
void dumpMemory(char *s, int sa, int ea)
{
   int i, ma;
   printf("\n%s", s);
   for (ma = sa; ma < ea;)
	{
		printf("\n0x%04x:", ma);
		for (i=0; i<8; i++)
		{
			printf(" %04x", MEMWORD((ma+i)));
		}
		ma+=8;
	}
   return;
} // end dumpMemory



// **************************************************************************
// **************************************************************************
// dvm <sa> <ea> - dump lc3 virtual memory
void dumpVMemory(char *s, int sa, int ea)
{
   int i, ma;
   printf("\n%s", s);
   for (ma = sa; ma < ea;)
	{
		printf("\n0x%04x:", ma);
		for (i=0; i<8; i++)
		{
			printf(" %04x", getMemoryData(ma+i));
		}
		ma+=8;
	}
   return;
} // end dumpVMemory



// **************************************************************************
// **************************************************************************
// crawler and memtest programs
void loadLC3File(char* string)
{
	char* myArgv[2];
	char buff[32];

	strcpy(buff, string);
	if (strchr(buff, '.')) *(strchr(buff, '.')) = 0;

	myArgv[0] = buff;
	myArgv[1] = string;
	createTask( myArgv[0],				// task name
					lc3Task,					// task
					MED_PRIORITY,			// task priority
					2,							// task argc
				  	myArgv);					// task argv
	return;
} // end loadFile



// **************************************************************************
// **************************************************************************
int P4_crawler(int argc, char* argv[])
{
	loadLC3File("crawler.hex");
	return 0;
} // end P4_crawler



// **************************************************************************
// **************************************************************************
int P4_memtest(int argc, char* argv[])
{
	loadLC3File("memtest.hex");
	return 0;
} // end crawler and memtest programs

// **************************************************************************

