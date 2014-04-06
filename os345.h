// os345.h	08/08/2013

#include <setjmp.h>


#ifndef __os345_h__
#define __os345_h__
// ***********************************************************************
// ***********************************************************************
// Context switching directives
#define SWAP swapTask();

// ***********************************************************************
// Thread-safe extensions: delete _s if desired
//#define SPRINTF sprintf_s
//#define STRCAT strcat_s

// ***********************************************************************
// Semaphore directives
#define SEM_WAIT(s)			semWait(s);
#define SEM_SIGNAL(s)		semSignal(s);
#define SEM_TRYLOCK(s)		semTryLock(s);

// ***********************************************************************
// Miscellaneous directives
#define INTEGER(s)	((s)?(isdigit(*s)||(*s=='-')?(int)strtol(s,0,0):0):0)

// ***********************************************************************
// Miscellaneous equates
#define MAX_TASKS 			127
#define MAX_STRING_SIZE		127
#define MAX_ARGS			50
#define STACK_SIZE			(64*1024/sizeof(int))
#define MAX_CYCLES			CLOCKS_PER_SEC/2
#define NUM_MESSAGES		500
#define INBUF_SIZE			256
#define ONE_TENTH_SEC		(CLOCKS_PER_SEC/10)

// Default priorities
#define LOW_PRIORITY		1
#define MED_PRIORITY		5
#define HIGH_PRIORITY		10
#define VERY_HIGH_PRIORITY	20
#define HIGHEST_PRIORITY	99

// Task ID's
#define SHELL_TID			0
#define ALL_TID				-1

// Task state equates
#define S_NEW				0
#define S_READY				1
#define S_RUNNING			2
#define S_BLOCKED			3
#define S_EXIT				4

#define FALSE				0
#define TRUE				1

#define BINARY				0
#define COUNTING			1

// Swap space
#define PAGE_FREE      		4
#define PAGE_GET_ADR   		3
#define PAGE_NEW_WRITE 		2
#define PAGE_OLD_WRITE 		1
#define PAGE_READ      		0
#define PAGE_INIT     		-1

#define P2_SEMS            128


// ***********************************************************************
// system structs
typedef int bool;						// boolean value
typedef int TID;						// task id







//Priority queue element
typedef struct
{
    int tid;
    int priority;
} pq_element;


//PQueue
typedef struct
{
    pq_element** elem;
    int size;
    int bot;      //bottom of the queue
} PQueue;

// semaphore
// If I'm not mistaken, this is a one-way linked list.
typedef struct semaphore			// semaphore
{
	struct semaphore* semLink;		// semaphore link. address of next semaphore.
	char* name;						// semaphore name
	int state;						// semaphore state
	int type;						// semaphore type. can be binary (0) or counting (1)
	int taskNum;					// semaphore creator task #
	PQueue* q;                      // Priority queue for counting semaphore
} Semaphore;


typedef struct
{
    Semaphore* sem;
    int tic;
} dc_elem;

typedef struct
{
    dc_elem** elem;
    int size;
    int bot;
} d_clock;



// task control block
typedef struct							// task control block
{
	char* name;							// task name
	int (*task)(int,char**);		    // task address
	int state;							// task state NEW READY RUNNING BLOCKED EXIT
	int priority;						// task priority (project 2)
	int argc;							// task argument count (project 1)
	char** argv;						// task argument pointers (project 1)
	int signal;							// task signals (project 1)
	void (*sigContHandler)(void);	    // task mySIGCONT handler
	void (*sigKillHandler)(void);	    // task mySIGKILL handler
	void (*sigIntHandler)(void);	    // task mySIGINT handler
	void (*sigTermHandler)(void);	    // task mySIGTERM handler
	void (*sigTstpHandler)(void);	    // task mySIGTSTP handler
	TID parent;							// task parent
	int RPT;							// task root page table (project 5)
	int cdir;							// task directory (project 6)
	Semaphore *event;					// blocked task semaphore. Red light / green light.
	void* stack;						// task stack
	jmp_buf context;					// task context pointer
	int time;                           // task time
} TCB;

// Task specific variables
#define CDIR		tcb[curTask].cdir
#define TASK_RPT	tcb[curTask].RPT

// intertask message
typedef struct
{
	int from;                  // source
   int to;                    // destination
   char* msg;						// msg
} Message;
#define MAX_MESSAGE_SIZE		64

// ***********************************************************************
// system prototypes
int createTask(char*, int (*)(int, char**), int, int, char**);
int killTask(int taskId);
void powerDown(int code);
void swapTask(void);

int getMessage(int from, int to, Message* msg);
int postMessage(int from, int to, char* msg);
char* myTime(char*);

Semaphore* createSemaphore(char* name, int type, int state);
bool deleteSemaphore(Semaphore** semaphore);
void semSignal(Semaphore*);
int semWait(Semaphore*);
int semTryLock(Semaphore*);




// ***********************************************************************
#define POWER_UP					0

// The POWER_DOWN_ERROR is to catch error conditions in the simple OS
// that are unrecoverable.  Again, code must set context back to the
// 'main' function and let 'main' handle the 'powerDown' sequence.
#define POWER_DOWN_ERROR      1

// POWER_DOWN_QUIT is used to indicate that the CLI has made a long
// jump into the main context of os345 with the intent to 'quit' the
// simple OS and power down.  The main function in os345.c will catch
// the 'quit' request and 'powerDown'.
#define POWER_DOWN_QUIT       -2

// The POWER_DOWN_RESTART indicates the user issued the 'restart'
// command in the CLI.  Again, the 'main' function issues the
// 'powerDown' sequence.
#define POWER_DOWN_RESTART    -1

// ***********************************************************************
// project prototypes
int P1_project1(int, char**);
int P1_shellTask(int, char**);
int P1_help(int, char**);
int P1_quit(int, char**);
int P1_lc3(int, char**);

int P2_project2(int, char**);
int P2_killTask(int, char**);
int P2_listSems(int, char**);
int P2_listTasks(int, char**);
int P2_reset(int, char**);
int P2_signal1(int, char**);
int P2_signal2(int, char**);
int tenSecTask(int, char**);

int P3_project3(int, char**);
int P3_dc(int, char**);
int dc_test(void);

int P4_project4(int, char**);
int P4_dumpFrame(int, char**);
int P4_dumpFrameTable(int, char**);
int P4_dumpLC3Mem(int, char**);
int P4_dumpPageMemory(int, char**);
int P4_dumpVirtualMem(int, char**);
int P4_initMemory(int, char**);
int P4_rootPageTable(int, char**);
int P4_userPageTable(int, char**);
int P4_vmaccess(int, char**);
int P4_virtualMemStats(int, char**);
int P4_crawler(int, char**);
int P4_memtest(int, char**);

int P5_project5(int, char**);
int P5_stress1(int, char**);
int P5_stress2(int, char**);
/*
int P5_atm(int, char**);
int P5_listMessages(int, char**);
*/

int P6_project6(int, char**);
int P6_cd(int, char**);
int P6_dir(int, char**);
int P6_dfat(int, char**);
int P6_mount(int, char**);
int P6_run(int, char**);
int P6_space(int, char**);
int P6_type(int, char**);
int P6_dumpSector(int, char**);
int P6_fileSlots(int, char**);

int P6_copy(int, char**);
int P6_define(int, char**);
int P6_del(int, char**);
int P6_mkdir(int, char**);
int P6_unmount(int, char**);
int P6_chkdsk(int, char**);
int P6_finalTest(int, char**);

int P6_open(int, char**);
int P6_read(int, char**);
int P6_write(int, char**);
int P6_seek(int, char**);
int P6_close(int, char**);

void initLC3Memory(int startFrame, int endFrame);

int atmTask(int, char**);
int bankTask(int, char**);
int dmeTask(int, char**);
int lc3Task(int, char**);

unsigned short int *getMemAdr(int va, int rwFlg);
void outPTE(char* s, int pte);
int accessPage(int pnum, int frame, int rwnFlg);
void initLC3Memory(int startFrame, int endFrame);

#endif // __os345_h__
