// os345semaphores.c - OS Semaphores
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
#include "queue.h"

extern TCB tcb[];							// task control block
extern int curTask;							// current task #

extern int superMode;						// system mode
extern Semaphore* semaphoreList;			// linked list of active semaphores

extern PQueue* rq;


// **********************************************************************
// **********************************************************************
// signal semaphore
//
//	if task blocked by semaphore, then clear semaphore and wakeup task
//	else signal semaphore
//  Essentially, the semaphore is a stop light. This function could be called
//  "green light."
//
void semSignal(Semaphore* s)
{
	int i;
	// assert there is a semaphore and it is a legal type
	assert("semSignal Error" && s && ((s->type == BINARY) || (s->type == COUNTING)));

	// check semaphore type
	if (s->type == 0)
	{
		// binary semaphore
		// look through tasks for one suspended on this semaphore

		for (i=0; i<MAX_TASKS; i++)	// look for suspended task
		                            // look through tasks for tasks with the indicated semaphore
		{
			if (tcb[i].event == s)
			{
				s->state = 0;				// clear semaphore
				tcb[i].event = 0;			// clear event pointer
				tcb[i].state = S_READY;	    // unblock task

				// ?? move task from blocked to ready queue
                enqueue(rq, dequeue(s->q, i), tcb[i].priority);

				if (!superMode) swapTask();
				return;
			}
		}
		// nothing waiting on semaphore, go ahead and just signal
		s->state = 1;						// nothing waiting, signal
		if (!superMode) swapTask();
		return;
	}
	else
	{
		// counting semaphore
		if(++s->state>0) return;

		int tid = dequeue(s->q, -1);

		tcb[tid].state=S_READY;
		tcb[tid].event=0;
        enqueue(rq, tid, tcb[tid].priority);
        return;

	}
} // end semSignal



// **********************************************************************
// **********************************************************************
// wait on semaphore
//
//	if semaphore is signaled, return immediately
//	else block task
//
int semWait(Semaphore* s)
{
	assert("semWait Error" && s);										// assert semaphore
	assert("semWait Error" && ((s->type == 0) || (s->type == 1)));	    // assert legal type
	assert("semWait Error" && !superMode);								// assert user mode

	// check semaphore type
	if (s->type == 0)
	{
		// binary semaphore
		// if state is zero, then block task

		if (s->state == 0)
		{
			tcb[curTask].event = s;		// block task
			tcb[curTask].state = S_BLOCKED;

            enqueue(s->q, dequeue(rq, curTask), tcb[curTask].priority);

			swapTask();						// reschedule the tasks
			return 1;
		}
		// state is non-zero (semaphore already signaled)
		s->state = 0;						// reset state, and don't block
		return 0;
	}
	else // counting semaphore
	{

   		if(--s->state >= 0) return 0;

        //remove from ready queue, put on semaphore queue, set state to blocked
        int tid = dequeue(rq, curTask);
        tcb[tid].state = S_BLOCKED;

        tcb[tid].event = s;
        enqueue(s->q, tid, tcb[tid].priority); //put task on semaphore queue
        swapTask();

        return 1;
	}
} // end semWait



// **********************************************************************
// **********************************************************************
// try to wait on semaphore
//
//	if semaphore is signaled, return 1
//	else return 0
//
int semTryLock(Semaphore* s)
{
	assert("semTryLock Error" && s);												// assert semaphore
	assert("semTryLock Error" && ((s->type == 0) || (s->type == 1)));	// assert legal type
	assert("semTryLock Error" && !superMode);									// assert user mode

	// check semaphore type
	if (s->type == 0)
	{
		// binary semaphore
		// if state is zero, then block task

		if (s->state == 0)
		{
			return 0;
		}
		// state is non-zero (semaphore already signaled)
		s->state = 0;						// reset state, and don't block
		return 1;
	}
	else
	{
	    //Point is to test if we can take the semaphore or not
	    //Is the semaphore available or not
		// counting semaphore
		// ?? implement counting semaphore

		//Check to see if it can take it
		//If it can, same logic as SEM_WAIT
		//trylock says "is the resource on the semaphore available? If it's not, don't block me."
	}
} // end semTryLock


// **********************************************************************
// **********************************************************************
// Create a new semaphore.
// Use heap memory (malloc) and link into semaphore list (Semaphores)
// 	name = semaphore name
//	type = binary (0), counting (1)
//  state = initial semaphore state
// Note: memory must be released when the OS exits.
//
Semaphore* createSemaphore(char* name, int type, int state)
{
	Semaphore* sem = semaphoreList;              //first element of semaphoreList
	Semaphore** semLink = &semaphoreList;        //I don't think this actually gets used

    // assert semaphore is binary (0) or counting (1)
	assert("createSemaphore Error" && ((type == 0) || (type == 1)));	// assert type is validate

	// look for duplicate name
	while (sem)
	{
		if (!strcmp(sem->name, name))       //compare all semaphore names to name parameter
		{                                   // !strcmp means strcmp returned equal (0 or NULL)
			printf("\nSemaphore %s already defined", sem->name);

			// ?? What should be done about duplicate semaphores ??
			// semaphore found - change to new state
			// In other words, change the values of the already-existing semaphore.
			sem->type = type;				// 0=binary, 1=counting
			sem->state = state;				// initial semaphore state
			sem->taskNum = curTask;			// set parent task #
			return sem;
		}
		// move to next semaphore
		semLink = (Semaphore**)&sem->semLink;   //address of next step in linked list
		sem = (Semaphore*)sem->semLink;         //next step in linked list
	}

	// allocate memory for new semaphore
	sem = (Semaphore*)malloc(sizeof(Semaphore));
	sem->q = pq_construct(P2_SEMS);

	// set semaphore values
	sem->name = (char*)malloc(strlen(name)+1);
	strcpy(sem->name, name);				    // semaphore name
	sem->type = type;							// 0=binary, 1=counting
	sem->state = state;						    // initial semaphore state
	sem->taskNum = curTask;					    // set parent task #

	// prepend to semaphore list
	sem->semLink = (struct semaphore*)semaphoreList;
	semaphoreList = sem;						// link into semaphore list
	return sem;									// return semaphore pointer
} // end createSemaphore



// **********************************************************************
// **********************************************************************
// Delete semaphore and free its resources
//
bool deleteSemaphore(Semaphore** semaphore)
{
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;

	// assert there is a semaphore
	assert("deleteSemaphore Error" && *semaphore);

	// look for semaphore
	while(sem)
	{
		if (sem == *semaphore)
		{
			// semaphore found, delete from list, release memory
			*semLink = (Semaphore*)sem->semLink;

			// free the name array before freeing semaphore
			//printf("\ndeleteSemaphore(%s)", sem->name);

			// ?? free all semaphore memory
			free(sem->name);
			pq_destruct(sem->q);
			free(sem);

			return TRUE;
		}
		// move to next semaphore
		semLink = (Semaphore**)&sem->semLink;
		sem = (Semaphore*)sem->semLink;
	}

	// could not delete
	return FALSE;
} // end deleteSemaphore
