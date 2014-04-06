// os345mmu.c - LC-3 Memory Management Unit
// **************************************************************************
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
// mmu variables

// LC-3 memory
// essentially, index the memory to an array
unsigned short int memory[LC3_MAX_MEMORY]; //LC-3 memory

// statistics
int memAccess;						// memory accesses
int memHits;						// memory hits
int memPageFaults;					// memory faults
int nextPage;						// swap page size
int pageReads;						// page reads
int pageWrites;						// page writes

int nextupt=0;
int nextrpt=0;
extern int curTask;
extern TCB tcb[MAX_TASKS];

int getFrame(int);
int getAvailableFrame(void);
int getClockFrame(int);


/**
* Gets a page frame.
*/
int getFrame(int unav)
{
	int frame;
	frame = getAvailableFrame();
	if (frame >=0) return frame;

	// run clock
	frame=getClockFrame(unav);

	return frame;
}
// **************************************************************************
// **************************************************************************
// LC3 Memory Management Unit
// Virtual Memory Process
// **************************************************************************
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023) (2^10)
//     / / / /     /                 / _________page defined
//    / / / /     /                 / /       __page # (0-4096) (2^12)
//   / / / /     /                 / /       /
//  / / / /     / 	             / /       /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p


/**
* Gets a a memory address.
* @param int va the virtual address
* @param int rwFlg reading or writing?
* @return short the physical memory address
*/
unsigned short int *getMemAdr(int va, int rwFlg)
{
	int rpta, rpte1, rpte2;                 //root page table (entries)
	int upta, upte1, upte2;                 //user page table (entries)
	int rptFrame, uptFrame;                 //root/user page table frames
    memAccess+=2;
	rpta = TASK_RPT + RPTI(va);
	rpte1 = memory[rpta];
	rpte2 = memory[rpta+1];

	// turn off virtual addressing for system RAM
	if (va < 0x3000) return &memory[va];

	if (DEFINED(rpte1))
	{
		rptFrame=FRAME(rpte1);                                  //
		memHits++;                                              //record the hit
	}
	else
	{
		memPageFaults++;                                        //record the fault
		rptFrame = getFrame(-1);                                //get new frame
		rpte1 = SET_DEFINED(SET_DIRTY(rptFrame));                          //
		if (PAGED(rpte2))
		{
			accessPage(SWAPPAGE(rpte2), rptFrame, PAGE_READ);
		}
		else
		{
		    rpte1=SET_DIRTY(rpte1);                             //Make rpte1 dirty
		    rpte2=0;                                            //set rpte2 to 0

		    //memset(pointer to block of memory to fill,
            //       value to be set,
            //       number of bytes)
			memset(&memory[(rptFrame<<6)], 0, 128);             //clear block of memory (set all to null)
		}
	}


	memory[rpta] = rpte1 = SET_REF(SET_PINNED(rpte1));          //set the reference and pinned bits
	memory[rpta+1] = rpte2;                                     //set

	upta = (FRAME(rpte1)<<6) + UPTI(va);
	upte1 = memory[upta];
	upte2 = memory[upta+1];

	if (DEFINED(upte1))
	{
		uptFrame=FRAME(upte1);
		memHits++;
	}
	else
	{
		// fault
		uptFrame = getFrame(rptFrame);
		upte1 = SET_DEFINED(uptFrame);
		if (PAGED(upte2))
		{
			accessPage(SWAPPAGE(upte2), uptFrame, PAGE_READ);
		}
		else
		{
            upte1=SET_DIRTY(upte1);
            upte2=0;
		}
		memPageFaults++;                                        //record new page fault
	}
	if(rwFlg){
        upte1=SET_DIRTY(upte1);
	}

	memory[upta] = SET_REF(upte1);
	memory[upta+1] = upte2;


	return &memory[(FRAME(upte1)<<6) + FRAMEOFFSET(va)];
} // end getMemAdr


// **************************************************************************
// **************************************************************************
// set frames available from sf to ef
//    flg = 0 -> clear all others
//        = 1 -> just add bits
//
void setFrameTableBits(int flg, int sf, int ef)
{	int i, data;
	int adr = LC3_FBT-1;             // index to frame bit table
	int fmask = 0x0001;              // bit mask

	// 1024 frames in LC-3 memory
	for (i=0; i<LC3_FRAMES; i++)
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;
			adr++;
			data = (flg)?MEMWORD(adr):0;
		}
		else fmask = fmask >> 1;
		// allocate frame if in range
		if ( (i >= sf) && (i < ef)) data = data | fmask;
		MEMWORD(adr) = data;
	}
	return;
} // end setFrameTableBits


/**
* Gets an available frame from the frame bit table.
* If no frame is available, return -1.
*/
int getAvailableFrame()
{
	int i, data;
	int adr = LC3_FBT - 1;				        // index to frame bit table
	int fmask = 0x0001;					        // bit mask

	for (i=0; i<LC3_FRAMES; i++)		        // look thru all frames
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;				        // move to next work
			adr++;
			data = MEMWORD(adr);
		}
		else fmask = fmask >> 1;		        // next frame
        // deallocate frame and return frame #
		if (data & fmask)
		{
		    MEMWORD(adr) = data & ~fmask;
		    accessPage(nextPage, FRAME(i), PAGE_FREE);
			return i;
		}
	}
	return -1;
} // end getAvailableFrame



/**
* Gets a frame from the frame bit table
* @param int unav frame to avoid
* @return the frame if available, or -1 if unavailable
*/
int getClockFrame(int unav){
    int count=0;
    int rpt=0x2400;                //location of root page table
    int upt;
    if(nextrpt) rpt=nextrpt;       //move to next entry in root page table
    for(count=0; count<13; rpt=rpt+2){ //go through root page table

        //Check if we are into main memory
        if(rpt>=0x3000){
            //Reset rpt to top of root page table
            rpt=0x2400;
            count++;
        }

        //check if entry exists
        if(MEMWORD(rpt)){
            //clear the pinned bit of the entry
            memory[rpt]=CLEAR_PINNED(memory[rpt]);

            int pte1=memory[rpt];
            int pte2=memory[rpt+1];

            //front of user page table
            int uptf=FRAME(pte1)<<6;
            //user page table 64 words (entries)
            for(upt=uptf; upt<uptf+64; upt+=2){
                //data is here
                if(memory[upt]){

                    //set entry pinned bit
                    memory[rpt]=SET_PINNED(memory[rpt]);
                    int upt1=memory[upt];
                    int upt2=memory[upt+1];

                    //if frame can be swapped, do it
                    if((!REFERENCED(upt1)) && (FRAME(upt1) != unav))
                    {
                        nextrpt=rpt;
                        nextupt=upt;
                        memory[upt]=0;

                        //Check if paged
                        if(PAGED(upt2)){
                            if(DIRTY(upt1))
                                accessPage(SWAPPAGE(upt2), FRAME(upt1), PAGE_OLD_WRITE);
                        }

                        //Not paged, so page it
                        else{
                            memory[upt+1]=SET_PAGED(nextPage);
                            accessPage(nextPage, FRAME(upt1), PAGE_NEW_WRITE);
                        }

                        accessPage(nextPage, FRAME(upt1), PAGE_FREE);

                        return FRAME(upt1);
                    }

                    if(REFERENCED(memory[upt])){
                        memory[upt]=CLEAR_REF(memory[upt]);
                    }
                }
            }

            if(!PINNED(memory[rpt]) && FRAME(pte1)!=unav){
                nextrpt=rpt+2;
                nextupt=0;
                memory[rpt]=0;

                if(PAGED(pte2)){
                    if(DIRTY(pte1)){
                        accessPage(SWAPPAGE(pte2), FRAME(pte1), PAGE_OLD_WRITE);
                    }
                }
                else{
                    memory[rpt+1]=SET_PAGED(nextPage);
                    accessPage(nextPage, FRAME(pte1), PAGE_NEW_WRITE);
                }
                accessPage(nextPage, FRAME(pte1), PAGE_FREE);
                return FRAME(pte1);
            }
        }
    }

    return -1;
} //end getClockFrame



// **************************************************************************
// read/write to swap space
/**
* Read/write to swap space
* @param pnum the page number
* @param int frame the frame to be written to / read from
* @param int rwnFlg read/write flag
*/
int accessPage(int pnum, int frame, int rwnFlg)
{
    int i;
   static unsigned short int swapMemory[LC3_MAX_SWAP_MEMORY];                       //Array of bytes

    //If nextPage or current page is too high, die
   if ((nextPage >= LC3_MAX_PAGE) || (pnum >= LC3_MAX_PAGE))
   {
      printf("\nVirtual Memory Space Exceeded!  (%d)", LC3_MAX_PAGE);
      exit(-4);
   }
   switch(rwnFlg)
   {
      case PAGE_INIT:                    		// init paging
         nextPage = 0;
         return 0;

      case PAGE_GET_ADR:                    	// return page address
         return (int)(&swapMemory[pnum<<6]);    //get page address from swapMemory

      case PAGE_NEW_WRITE:                      // new write (Drops thru to write old)
         pnum = nextPage++;                     //set pnum to the next page

      case PAGE_OLD_WRITE:                      // write
         //printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", p.PID, frame, frame<<6, pnum);
         memcpy(&swapMemory[pnum<<6], &memory[frame<<6], 1<<7); //Move data from primary to swap
         pageWrites++;                          //Record that page written
         return pnum;                           //Return the page number

      case PAGE_READ:                    // read
         //printf("\n    (%d) Read page %d into frame %d (memory[%04x])", p.PID, pnum, frame, frame<<6);
      	memcpy(&memory[frame<<6], &swapMemory[pnum<<6], 1<<7); //Move data from swap to primary
         pageReads++;                           //Record that page read
         return pnum;                           //Return the page number

      case PAGE_FREE:                   // free page
          for(i=frame<<6; i<(frame+1)<<6; i++){
            memory[i]=0;
          }
         break;
   }
   return pnum;
} // end accessPage
