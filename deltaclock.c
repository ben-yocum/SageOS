#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "deltaclock.h"
#include "logger.h"
#include "os345.h"

#define MIN_INT -32767

/*
My delta clock will have the following structure
elem #      tics
5           1      <- TOP (next to be performed'd)
4           20
3           30     <- BOTTOM (last to be performed'd)
2                  <- empty (no accessible data)
1
0

*/




extern TCB* tcb;




/**
* Malloc space for the new delta clock.
* @param int size the max number of elements in the array
* @return d_clock* the newly allocated delta clock
*/
d_clock* dc_construct(int size)
{
    assert(size > 0);
    int i;

    d_clock* dc;
    dc = (d_clock*)malloc(sizeof(d_clock));
    dc->elem = malloc( sizeof(pq_elem*) * size );
    dc->size = size;
    dc->bot = size-1;

    for(i=0; i<size; i++){
        dc->elem[i] = malloc( sizeof(dc_elem) );
        dc->elem[i]->tic = -1;
        dc->elem[i]->tid = -1;
    }

    return dc;
}

void dc_destruct(d_clock* dc){
    int i;
    for(i = 0; i < dc->size; i++){
        free( dc->elem[i] );
    }
    free(dc->elem);
    free(dc);
}


/**
* Push an element onto the indicated delta clock.
* @param d_clock* dc a pointer to the delta clock in question
* @param int tid the task id to be pushed
* @param int tic the number of tics
* @return 1 if successful, 0 if unsuccessful
*/
int enqueue(PQueue *q, int tid, int priority)
{
    int i=q->bot; //where to insert

    //q is not null, tid is nonnegative
    assert(q->elem && tid>=0);

    //Return false if queue is full
    if(q->bot==-1){
        return 0; //queue full
    }

    //starting from the bottom of the queue, find the first instance of an element
    //with a greater-than-or-equal-to priority
    for(; i < q->size; i++){
        if(q->elem[i]->priority >= priority){
            break;
        }
    }
    i--;


    //Now we know which element has >= priority. Move all elements below it
    //down and push.

    int c;
    for(c=q->bot; c<=i; c++){
        if(c!=0){
            q->elem[c-1]->priority = q->elem[c]->priority;
            q->elem[c-1]->tid = q->elem[c]->tid;
        }

    }
    c--;

    q->elem[c]->priority = priority;
    q->elem[c]->tid = tid;

    q->bot--;

    return 1;
}

/**
* Remove and return the specified element from the queue.
* @param PQueue the queue from which to get the data
* @param tid the task id to be removed. If negative, return
*        top item.
* @return the task id of the next task to be performed, or 0
*         if no elements exist or tid is not found.
*/
int dequeue(PQueue *q, int tid)
{
    //q is not null
    assert(q->elem);

    //Return false if queue is empty
    if(q->bot==q->size-1){
        return 0; //queue empty
    }

    //Store position of element to be popped in i
    int i=-1;
    if(tid == -1){ //retrieving top element
        i = q->size-1;
    } else { //retrieving specified element
        //find the element to retrieve
        int j = 0;
        for(j=0; j < q->size; j++){
            if(q->elem[j]->tid == tid) i = j;
        }
        j--;

        if(i == -1) //element not found
            return 0;
    }

    //Save value to be returned
    int myTID = q->elem[i]->tid;

    //Roll all lower values up to i
    int c;
    for(c=i; c>q->bot; c--){
        if(c!=0){
            q->elem[c]->priority = q->elem[c-1]->priority;
            q->elem[c]->tid = q->elem[c-1]->tid;
        }
    }

    c++;
    q->elem[c]->priority = MIN_INT;
    q->elem[c]->tid = -1;

    q->bot++;


    return myTID;
}


/**
* View top element in queue without removing it
* @param PQueue q the queue in question
* @return int the task id of the top element (next task to be performed).
          Returns -1 if stack is empty (because 0 is a valid task id)
*/
int peek(PQueue* q)
{
    int size = q->size;
    return q->elem[size-1]->tid;
}



void pq_print(PQueue* q)
{
    int id;
    for(id=q->size-1; id>=q->bot; id--){
        int i = q->elem[id]->tid;
        if(i>=0)
            printTask(i);
    }
}



void pq_test(void){
    printf("\nTesting Priority Queue");
    logln();
    logg("******Testing PQueue******");

    //Test memory allocation
    logg("Allocating memory for queue of size 3");
    PQueue* q = pq_construct(3);

    int t = 1;

    //Test adding a valid element
    if(t) t=   pq_assert("Adding valid element with id 1, priority 1", q, enqueue(q, 1, 1));
    if(t) t=   pq_assert("Peek should be 1", q, peek(q)==1);
    if(t) t=   pq_assert("Peek should NOT be 2", q, peek(q)!=2);

    if(t) t=   pq_assert("Adding valid element with id 2, priority 10", q, enqueue(q, 2, 10));
    if(t) t=   pq_assert("Peek should be 2", q, peek(q)==2);
    if(t) t=   pq_assert("Peek should NOT be 1", q, peek(q)!=1);

    if(t) t=   pq_assert("Adding valid element with id 3, priority 5", q, enqueue(q, 3, 5));
    if(t) t=   pq_assert("Peek should be 2", q, peek(q));

    if(t) t=   pq_assert("Popping top of queue", q, dequeue(q, -1));
    if(t) t=   pq_assert("Peek should be 3", q, peek(q));

    if(t) t=   pq_assert("Popping tid 1", q, dequeue(q, 1));
    if(t) t=   pq_assert("Peek should be 3", q, peek(q)==3);

    if(t) t=   pq_assert("Attempting to pop element that doesn't exist", q, !dequeue(q, 5));
    if(t) t=   pq_assert("Peek should be 3", q, peek(q)==3);

    if(t) t=   pq_assert("Popping last element", q, dequeue(q, 3));
    if(t) t=   pq_assert("Peek should be -1", q, peek(q)==-1);

    if(t) t=   pq_assert("Testing pop on empty stack", q, !dequeue(q, 1));
    if(t) t=   pq_assert("Testing peek on empty stack", q, peek(q)==-1);

    if(t) printf("\nAll tests passed.");
    else printf("\nTests failed. Please check log for information.");


    logln();
    logg("Destructing queue");
    pq_destruct(q);
    logg("PQueue test complete");
}

int pq_assert(char* message, PQueue* q, int success){
    logln();
    logg(message);
    if(success) {
        logg("SUCCESS");
        return 1;
    }
    else{
        logg("FAILED");
        logg("QUITTING");
        return 0;
    }
}

