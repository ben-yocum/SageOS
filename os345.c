// os345.c - OS Kernel	09/12/2013
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
#include "os345signals.h"
#include "os345config.h"
#include "os345lc3.h"
#include "os345fat.h"
#include "logger.h"
#include "queue.h"

#include "bufHist.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static int scheduler(void);
static int dispatcher(void);

//static void keyboard_isr(void);
//static void timer_isr(void);

int sysKillTask(int taskId);
static int initOS(void);

// **********************************************************************
// **********************************************************************
// global semaphores

Semaphore* semaphoreList;			// linked list of active semaphores

Semaphore* keyboard;				// keyboard semaphore
Semaphore* charReady;				// character has been entered
Semaphore* inBufferReady;			// input buffer ready semaphore

Semaphore* tics10sec;               // 10 second semaphore
Semaphore* tics1sec;				// 1 second semaphore
Semaphore* tics10thsec;				// 1/10 second semaphore

Semaphore* sysMutex;


// **********************************************************************
// **********************************************************************
// global system variables

TCB tcb[MAX_TASKS];					// task control block
Semaphore* taskSems[MAX_TASKS];		// task semaphore
jmp_buf k_context;					// context of kernel stack
jmp_buf reset_context;				// context of kernel stack
volatile void* temp;				// temp pointer used in dispatcher

int scheduler_mode;					// scheduler mode (0=prioritized round-robin, 1=fair scheduler
int superMode;						// system mode
int curTask;						// current task #
long swapCount;						// number of re-schedule cycles
char inChar;						// last entered character
int charFlag;						// 0 => buffered input
int inBufIndx;						// input pointer into input buffer
char inBuffer[INBUF_SIZE+1];		// character input buffer
//Message messages[NUM_MESSAGES];		// process message buffers

int pollClock;						// current clock()
int lastPollClock;					// last pollClock
bool diskMounted;					// disk has been mounted

time_t oldTime10;
time_t oldTime1;					// old 1sec time

clock_t myClkTime;
clock_t myOldClkTime;
PQueue* rq;							// ready priority queue

//delta clocks
d_clock* dc;
d_clock* dct;                       //test delta clock

extern d_clock* dc_construct(int size);
extern void dc_destruct(d_clock** c);



// **********************************************************************
// **********************************************************************
// OS startup
//
// 1. Init OS
// 2. Define reset longjmp vector
// 3. Define global system semaphores
// 4. Create CLI task
// 5. Enter scheduling/idle loop
//
int main(int argc, char* argv[])
{


	// save context for restart (a system reset would return here...)
	int resetCode = setjmp(reset_context);  // when longjmp(reset_context, SOME_CODE) is called, returns here
                                            // on first run through, set to 0 (POWER_UP)


	superMode = TRUE;						// supervisor mode

	switch (resetCode)
	{
		case POWER_DOWN_QUIT: //-2				// quit
			powerDown(0);
		    logg("Power down.\n");

			printf("\nGoodbye!!");
			return 0;

		case POWER_DOWN_RESTART: //-1			// restart
			powerDown(resetCode); //power down, then continue with
                                  //rest of switch (no break; till POWER_UP)
            logg("Restart.");
			printf("\nRestarting system...\n");

		case POWER_UP: //0						// startup
		    logg("Power up.");
			break; //case does nothing. Function continues normally.

		default: //error
			printf("\nShutting down due to error %d", resetCode);
			logg("Shutting down due to an error.");
			powerDown(resetCode);
			return resetCode;
	}

	// output header message
	printf("%s", STARTUP_MSG);

	// initalize OS
	if ( (resetCode = initOS()) ) return resetCode;         //if an error, it returns 99, which is not
                                                            //a recognized reset code.



    //Semaphore* createSemaphore(char* name, int type, int state);
	charReady     = createSemaphore( "charReady",     BINARY,   0 );
	inBufferReady = createSemaphore( "inBufferReady", BINARY,   0 );
	keyboard      = createSemaphore( "keyboard",      BINARY,   1 );
	tics10sec     = createSemaphore( "tics10sec",     COUNTING, 0 );
	tics1sec      = createSemaphore( "tics1sec",      BINARY,   0 );
	tics10thsec   = createSemaphore( "tics10thsec",   BINARY,   0 );
	sysMutex      = createSemaphore( "sysMutex",      BINARY,   1 );

	//Create delta clock
    dc = dc_construct(1000);

	// schedule CLI task
	createTask("myShell",			// task name
					P1_shellTask,	// task
					MED_PRIORITY,	// task priority
					argc,			// task arg count
					argv);			// task argument pointers


    logg("Beginning to run SageOS");


	// HERE WE GO................

	// Scheduling loop
	// 1. Check for asynchronous events (character inputs, timers, etc.)
	// 2. Choose a ready task to schedule
	// 3. Dispatch task
	// 4. Loop (forever!)

	while(1)									// scheduling loop
	{
		// check for character / timer interrupts
		pollInterrupts();

		// schedule highest priority ready task
		if ((curTask = scheduler()) < 0) continue;

		// dispatch curTask, quit OS if negative return
		if (dispatcher() < 0) break;
	}											// end of scheduling loop

	// exit os
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end main




/**
* This function finds (schedules) the next task that
* should be performed by popping and pushing onto the ready
* queue. Once a task has been popped, it is immediately
* pushed back onto the queue.
*
* This function has two side effects:
* 1) Change the value of curTask
* 2) Pop and push the top element on the ready queue
*
* @return int the id of the next task that should be performed
*/
static int scheduler()
{
    //Round robin
    if(scheduler_mode == 0){
        //Get nextTask, store in the correct places
        int nextTask = peek(rq);

        if (tcb[nextTask].signal & mySIGSTOP) return -1;

        nextTask = dequeue(rq, -1);
        curTask = nextTask;

        //put task right back on the queue
        enqueue(rq, nextTask, tcb[curTask].priority);

        //Return -1 if task has been stopped
        if (tcb[nextTask].signal & mySIGTERM) killTask(nextTask);


        return nextTask;

    }

    //Fair scheduler
    else{
        //printf("\nBeginning fair scheduler cycle");

        int firstTask = dequeue(rq, -1);
        //printf("\nDequeueing %d", firstTask);
        enqueue(rq, firstTask, tcb[firstTask].priority);
        //printf("\nEnqueueing %d", firstTask);

        int nextTask = firstTask;

        //Find next task with time != 0
        while(tcb[nextTask].time == 0){
           // printf("\nTesting %d, time = %d", nextTask, tcb[nextTask].time);
            nextTask = dequeue(rq, -1);
            enqueue(rq, nextTask, tcb[nextTask].priority);

            //printf("\nAttempting to find next task with time != 0, nextTask = %d", nextTask);

            //If all tasks have 0 time, redistribute time
            if(nextTask == firstTask){
                //printf("\nGot to firstTask %d, distributing time", firstTask);
                int total_time = 10000; //100,000

                fair_dist_time(rq, total_time);
            }
        }

        //printf("\nPerforming task %d", nextTask);
        //Now, perform task
        tcb[nextTask].time--;
        if (tcb[nextTask].signal & mySIGSTOP) return -1;

        curTask = nextTask;

        if (tcb[nextTask].signal & mySIGTERM) killTask(nextTask);

        return nextTask;
    }
} // end scheduler



// **********************************************************************
// **********************************************************************
// dispatch curTask
//
static int dispatcher()
{
	int result;

	// schedule task
	switch(tcb[curTask].state)
	{
		case S_NEW:
		{
			// new task
			tcb[curTask].state = S_RUNNING;	// set task to run state

			// save kernel context for task SWAP's
			if (setjmp(k_context))
			{
				superMode = TRUE;					// supervisor mode
				break;								// context switch to next task
			}

			// move to new task stack (leave room for return value/address)
			temp = (int*)tcb[curTask].stack + (STACK_SIZE-8);
			SET_STACK(temp);
			superMode = FALSE;						// user mode

			// begin execution of new task, pass argc, argv
			result = (*tcb[curTask].task)(tcb[curTask].argc, tcb[curTask].argv);

			// task has completed
			if (result) printf("\nTask[%d] returned %d", curTask, result);
			else printf("\nTask[%d] returned %d", curTask, result);
			tcb[curTask].state = S_EXIT;			// set task to exit state

			// return to kernal mode
			longjmp(k_context, 1);					// return to kernel
		}

		case S_READY:
		{
			tcb[curTask].state = S_RUNNING;			// set task to run
		}

		case S_RUNNING:
		{
			if (setjmp(k_context))
			{
				// SWAP executed in task
				superMode = TRUE;					// supervisor mode
				break;								// return from task
			}
			if (signals()) break;
			longjmp(tcb[curTask].context, 3); 		// restore task context
		}

		case S_BLOCKED:
		{
			// ?? Could check here to unblock task
			break;
		}

		case S_EXIT:
		{
			if (curTask == 0) return -1;			// if CLI, then quit scheduler
			// release resources and kill task
			sysKillTask(curTask);					// kill current task
			break;
		}

		default:
		{
			printf("Unknown Task[%d] State", curTask);
			longjmp(reset_context, POWER_DOWN_ERROR);
		}
	}
	return 0;
} // end dispatcher



// **********************************************************************
// **********************************************************************
// Do a context switch to next task.

// 1. If scheduling task, return (setjmp returns non-zero value)
// 2. Else, save current task context (setjmp returns zero value)
// 3. Set current task state to READY
// 4. Enter kernel mode (longjmp to k_context)

void swapTask()
{
	assert("SWAP Error" && !superMode);		// assert user mode

	// increment swap cycle counter
	swapCount++;

	// either save current task context or schedule task (return)
	if (setjmp(tcb[curTask].context))
	{
		superMode = FALSE;					// user mode
		return;
	}

	// context switch - move task state to ready
	if (tcb[curTask].state == S_RUNNING) tcb[curTask].state = S_READY;

	// move to kernel mode (reschedule)
	longjmp(k_context, 2);
} // end swapTask



// **********************************************************************
// **********************************************************************
// system utility functions
// **********************************************************************
// **********************************************************************

// **********************************************************************
// **********************************************************************
// initialize operating system
/*
* Essentially, sets a bunch of values to 0, including a lot in the task
* control block.
* Also gets the ready queue ready.
* Malloc space for buffHist.
*/
static int initOS()
{


	int i;

	// make any system adjustments (for unblocking keyboard inputs)
	//system("stty -echo -icanon");fcntl(1,F_SETFL,O_NONBLOCK);
	//Apparently, in DOS and .NET, does nothing.
	INIT_OS

	// reset system variables
	curTask = 0;						// current task #
	swapCount = 0;						// number of scheduler cycles
	scheduler_mode = 0;					// default scheduler
	inChar = 0;							// last entered character
	charFlag = 0;						// 0 => buffered input
	inBufIndx = 0;						// input pointer into input buffer
	semaphoreList = 0;					// linked list of active semaphores
	diskMounted = 0;					// disk has been mounted

	// malloc ready queue
	rq = pq_construct(MAX_TASKS);
	if (rq == NULL) return 99;

	// capture current time
	lastPollClock = clock();			// last pollClock
	time(&oldTime1);                    //set oldTime1 to the current UNIX timestamp in seconds
	time(&oldTime10);

	// init system tcb's
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;				// tcb
		taskSems[i] = NULL;				// task semaphore
	}

	// init tcb
	for (i=0; i<MAX_TASKS; i++)         //Not sure what this is for.
	{                                   //It looks to repeat what already happened in the previous
		tcb[i].name = NULL;             //for loop.
	}



	// initialize lc-3 memory
	initLC3Memory(LC3_MEM_FRAME, 0xF800>>6);    //I'm not going to get into this bad boy yet

    initDB();
	// ?? initialize all execution queues

	return 0;
} // end initOS



// **********************************************************************
// **********************************************************************
// Causes the system to shut down. Use this for critical errors
/*
* This function shuts the program down.
* @param code Valid codes: 0 (shut down) or -1 (POWER_DOWN_RESTART)
* Note that no error checking is performed on this code,
* and it has little functional importance.
*/
void powerDown(int code)
{

    superMode = 0;
    SEM_WAIT(sysMutex);
    superMode = 1;

	int i;                                      //for iterating through tcb
	printf("\nPowerDown Code %d", code);

	// release all system resources.
	printf("\nRecovering Task Resources...");

	// kill all tasks
	for (i = MAX_TASKS-1; i >= 0; i--)          //for each task, starting from end of array
		if(tcb[i].name) sysKillTask(i);         //if it has been assigned a name (in other words, if
                                                //the task has actually been created, not just a placeholder)
                                                //then call sysKillTask to free the resources in it.

	// delete all semaphores
	while (semaphoreList)
		deleteSemaphore(&semaphoreList);

    //free delta clock
    dc_destruct(&dc);

	// free ready queue
	pq_destruct(rq);

	//release db
	killDB();

	// ?? release any other system resources
	// ?? deltaclock (project 3)

	RESTORE_OS
	return;
} // end powerDown

