// os345interrupts.c - pollInterrupts	08/08/2013
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
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
#include <time.h>
#include <assert.h>

#include "os345.h"
#include "os345config.h"
#include "os345signals.h"

#include "logger.h"
#include "bufHist.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static void keyboard_isr(void);
static void timer_isr(void);

// **********************************************************************
// **********************************************************************
// global semaphores

extern Semaphore* keyboard;				// keyboard semaphore
extern Semaphore* charReady;				// character has been entered
extern Semaphore* inBufferReady;			// input buffer ready semaphore

extern Semaphore* tics10sec;            // 10 second semaphore
extern Semaphore* tics1sec;				// 1 second semaphore
extern Semaphore* tics10thsec;				// 1/10 second semaphore

extern char inChar;				// last entered character
extern int charFlag;				// 0 => buffered input
extern int inBufIndx;				// input pointer into input buffer
extern char inBuffer[INBUF_SIZE+1];	// character input buffer

extern time_t oldTime1;					// old 1sec time
extern time_t oldTime10;
extern clock_t myClkTime;
extern clock_t myOldClkTime;

extern int pollClock;				// current clock()
extern int lastPollClock;			// last pollClock

extern int superMode;						// system mode

extern TCB tcb[MAX_TASKS];

extern PQueue* rq;
extern d_clock* dc;
extern d_clock* dct;

extern int test_dc_dec(d_clock* c);
extern int dc_dec(d_clock* c);


// **********************************************************************
// **********************************************************************
// simulate asynchronous interrupts by polling events during idle loop
//
/* This function does three things:
*  1) Check for timeouts. If more than a half second has passed
*     since the function was previously called, fail an assertion.
*  2) Check for keyboard interrupt. If someone has typed something then
*     process the key that was typed.
*  3) Perform the timer interrupt service routine, which just checks to see
*     if a certain amount of time has passed and does something if it has.
*/
void pollInterrupts(void)
{
	// check for task monopoly
	// pollClock is not a measure of time but of clock cycles.
	// It is system specific.
	// See if more than MAX_CYCLES have elapsed since this function was last called.
	// MAX_CYCLES = CLOCKS_PER_SEC / 2
	// So, assert that no more than half a second has passed since last time function was
	// called.
	pollClock = clock();
	//assert("Timeout" && ((pollClock - lastPollClock) < MAX_CYCLES));
	lastPollClock = pollClock;

	// check for keyboard interrupt
	if ((inChar = GET_CHAR) > 0)
	{
	  keyboard_isr();
	}

	// timer interrupt
	timer_isr();

	return;
} // end pollInterrupts


// **********************************************************************
// keyboard interrupt service routine
//
static void keyboard_isr()
{
	// assert system mode
	assert("keyboard_isr Error" && superMode);

	semSignal(charReady);					// SIGNAL(charReady) (No Swap)
	if (charFlag == 0)
	{
		switch (inChar)
		{
            case 72:                        //up
            {
                //clear till beginning of line
                int i; for(i=0; i<strlen(inBuffer); i++) printf("\b \b");

                //Set inBuffer to historical value
                prvBuf(inBuffer);
                inBufIndx=strlen(inBuffer);

                //print inBuffer
                printf(inBuffer);

                break;
            }
            case 80:                        //down
            {
                //clear till beginning of line
                int i; for(i=0; i<strlen(inBuffer); i++) printf("\b \b");

                //Set inBuffer to historical value
                nxtBuf(inBuffer);
                inBufIndx=strlen(inBuffer);

                //print inBuffer
                printf(inBuffer);

                break;
            }
            case 75:                        //left
            {
                break;
            }
            case 77:                        //right
            {
                break;
            }
			case '\r':
			case '\n':
			{
                addBuff(inBuffer);
                resetBuffHist();
				inBufIndx = 0;				// EOL, signal line ready
				semSignal(inBufferReady);	// SIGNAL(inBufferReady)
				break;
			}

			case 0x12:                      //^r
            {
                int tid;
                sigSignal(-1, mySIGCONT);
                for(tid = 0; tid < MAX_TASKS; tid++){
                    sigSignal(tid, mySIGCONT);      //sigSignal SIGCONT to all tasks
                    tcb[tid].signal &= ~mySIGSTOP; //Clear SIGSTOP from all tasks
                    tcb[tid].signal &= ~mySIGTSTP; //Clear SIGTSTP from all tasks
                }
                break;
            }

            case 0x17:                      //^w
            {
                sigSignal(-1, mySIGTSTP);
                break;
            }

			case 0x18:						// ^x
			{
				inBufIndx = 0;
				inBuffer[0] = 0;
				sigSignal(0, mySIGINT);		// interrupt task 0
				semSignal(inBufferReady);	// SEM_SIGNAL(inBufferReady)
				break;
			}

			case '\b':                      //backspace
            {

                if(inBufIndx>0){            //don't go too far back
                    inBufIndx--;            //go back a space
                    printf("\b \b");        //Clear character on screen
                }
                inBuffer[inBufIndx] = 0;      //set character to null

                break;
            }

			default:
			{
				inBuffer[inBufIndx++] = inChar;
				inBuffer[inBufIndx] = 0;
				printf("%c", inChar);		// echo character
			}
		}
	}
	else
	{
		// single character mode
		inBufIndx = 0;
		inBuffer[inBufIndx] = 0;
	}
	return;
} // end keyboard_isr


// **********************************************************************
// timer interrupt service routine
//
static void timer_isr()
{
	time_t currentTime;						// current time

	// assert system mode
	assert("timer_isr Error" && superMode);

	// capture current time
  	time(&currentTime);

  	// ten second timer
  	if ((currentTime - oldTime10) >= 10)
  	{
		// signal 1 second
        semSignal(tics10sec);
		oldTime10 += 10;
  	}

  	// one second timer
  	if ((currentTime - oldTime1) >= 1)
  	{
		// signal 1 second
        semSignal(tics1sec);
		oldTime1 += 1;

  	}

	// sample fine clock
	myClkTime = clock();
	if ((myClkTime - myOldClkTime) >= ONE_TENTH_SEC)
	{
		myOldClkTime = myOldClkTime + ONE_TENTH_SEC;   // update old
		semSignal(tics10thsec);
		if(dct) test_dc_dec(dct);
		if(dc) dc_dec(dc);
	}

	// ?? add other timer sampling/signaling code here for project 2

	return;
} // end timer_isr
