/**
* Delta clock implementation
* @author Benjamin Yocum
*
* This is based heavily on my implementation of a queue
*
* My dc will have the following structure
* elem #      tics
* 5           9      <- TOP (next to be removed)
* 4           2
* 3           8       <- BOTTOM (last to be removed)
* 2                   <- empty (no accessible data)
* 1
* 0
*
* Here is the implementation of the dc_elem and d_clock structs:
*
* typedef struct
* {
*    Semaphore* sem;
*    int tic;
* } dc_elem;

*typedef struct
* {
*    dc_elem** elem;
*    int size;
*    int bot;
* } d_clock;
*
*
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "logger.h"
#include <string.h>

#define MIN_INT -32767
#define DCTIC ONE_TENTH_SEC

extern d_clock* dct;

int dct_removed = 0;




/**
* Malloc space for the new delta clock.
* @param int size the max number of elements in the array
* @return d_clock the newly allocated delta clock
*/
d_clock* dc_construct(int size)
{
    assert(size > 0);
    int i;

    d_clock* c;
    c = (d_clock*)malloc(sizeof(d_clock));
    c->elem = malloc( sizeof(dc_elem*) * size );
    c->size = size;
    c->bot = size-1;

    //Set default values for each element
    for(i=0; i<size; i++){
        c->elem[i] = malloc( sizeof(pq_element) );
        c->elem[i]->tic = -1;
        c->elem[i]->sem = NULL;
    }

    return c;
}

void dc_destruct(d_clock** cp){
    d_clock* c = *cp;
    int i;
    for(i = 0; i < c->size; i++){
        free( c->elem[i] );
    }
    free(c->elem);
    free(c);

    *cp = NULL;
    return;
}


/**
* Insert an element onto the indicated delta clock.
* @param d_clock* c a pointer to the delta clock in question
* @param Semaphore* sem the semaphore to be inserted
* @param int tic the number of 1/10 second tics
* @return 1 if successful, 0 if unsuccessful
*/
int dc_insert(d_clock *c, Semaphore* sem, int tic)
{
    int i=c->size-1; //where to insert

    //tic is positive
    assert( tic>0);

    //Return false if dc is full
    if(c->bot==-1){
        return 0; //dc full
    }

    //starting from the top of the dc, find the place
    //where the sum(prevtics) > tic
    int sum=0;
    for(i=c->size-1; i >= c->bot; i--){
        //Perform insert here
        if(sum+c->elem[i]->tic > tic || c->bot == c->size-1 || i == c->bot){
            //Move elements down
            int m;
            for(m=c->bot; m<=i; m++){
                if(m!=0){
                    c->elem[m-1]->sem = c->elem[m]->sem;
                    c->elem[m-1]->tic = c->elem[m]->tic;
                }
            }
            m--;

            //insert
            c->elem[m]->sem = sem;
            c->elem[m]->tic = tic;

            //decrement tic of current element
            c->elem[m]->tic = c->elem[m]->tic - sum;

            //decrement tic of elem directly below
            if(m != c->bot){
                if(c->elem[m-1]->tic != -1){
                    c->elem[m-1]->tic = (c->elem[m-1]->tic) - c->elem[m]->tic;
                }
            }

            c->bot--;

            break;
        }
        else sum = sum+c->elem[i]->tic;
    }

    return 1;
}

/**
* Removes an element.
* This function should generally not be called by anything
* other than the dc_dec() function.
* @param c the d_clock from which to remove
* @return 1 if successful, 0 if failed
*/
int dc_remove(d_clock* c)
{
    //top element has tic of 0
    assert(c->elem[c->size-1]->tic == 0);

    //delta clock is empty
    if(c->bot == c->size-1) return 0;

    c->bot++;


    //roll up from top
    int m;
    for(m=c->size-1; m>c->bot; m--){
        c->elem[m]->sem = c->elem[m-1]->sem;
        c->elem[m]->tic = c->elem[m-1]->tic;
    }
    m++;


    //remove bottom element (which is left over from the roll-up)
    c->elem[c->bot]->tic = -1;
    c->elem[c->bot]->sem = NULL;


    return 1;
}


int dc_dec(d_clock* c)
{
    c->elem[c->size-1]->tic--;
    while(c->elem[c->size-1]->tic == 0){
        SEM_SIGNAL(c->elem[c->size-1]->sem);
        dc_remove(c);
    }
    return 0;
}

int test_dc_dec(d_clock* c){
    printf(".");
    c->elem[c->size-1]->tic--;
    while(c->elem[c->size-1]->tic == 0){
        SEM_SIGNAL(c->elem[c->size-1]->sem);
        printf("\nSignaling and removing semaphore %s", c->elem[c->size-1] ->sem->name);
        dc_remove(c);
        printf("\n");
        dct_removed++;
    }
    if(dct_removed==8){
        printf("Test finished. Cleaning up.");
        dc_destruct(&dct);
    }
    return 0;
}




void dc_print(d_clock* c)
{
    int i;
    for(i=c->size-1; i>c->bot; i--){
        printf("\n%d    %s", c->elem[i]->tic, c->elem[i]->sem->name);
    }
}



int dc_test(void){

    //Setup
    printf("\nAllocating memory for delta clock of size 8");
    dct = dc_construct(8);
    Semaphore* sems[dct->size];
    int tics[dct->size];
    printf("\nTesting Delta Clock");

    //Test insertions
    int tic = 20; Semaphore* tSem1 = createSemaphore("tSem1", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem1->name);
    dc_insert(dct, tSem1, tic);
    sems[7] = tSem1; tics[7] = tic;
    dc_test_state(sems, tics);

    tic = 5; Semaphore* tSem2 = createSemaphore("tSem2", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem2->name);
    dc_insert(dct, tSem2, tic);
    sems[7] = tSem2; tics[7] = 5;
    sems[6] = tSem1; tics[6] = 15;
    dc_test_state(sems, tics);

    tic = 35; Semaphore* tSem3 = createSemaphore("tSem3", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem3->name);
    dc_insert(dct, tSem3, tic);
    sems[5] = tSem3; tics[5] = 15;
    dc_test_state(sems, tics);

    tic = 27; Semaphore* tSem4 = createSemaphore("tSem4", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem4->name);
    dc_insert(dct, tSem4, tic);
    sems[5] = tSem4; tics[5] = 7;
    sems[4] = tSem3; tics[4] = 8;
    dc_test_state(sems, tics);

    tic = 27; Semaphore* tSem5 = createSemaphore("tSem5", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem5->name);
    dc_insert(dct, tSem5, tic);
    sems[4] = tSem5; tics[4] = 0;
    sems[3] = tSem3; tics[3] = 8;
    dc_test_state(sems, tics);

    tic = 22; Semaphore* tSem6 = createSemaphore("tSem6", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem6->name);
    dc_insert(dct, tSem6, tic);
    sems[5] = tSem6; tics[5] = 2;
    sems[4] = tSem4; tics[4] = 5;
    sems[3] = tSem5; tics[3] = 0;
    sems[2] = tSem3; tics[2] = 8;
    dc_test_state(sems, tics);

    tic = 17; Semaphore* tSem7 = createSemaphore("tSem7", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem7->name);
    dc_insert(dct, tSem7, tic);
    sems[6] = tSem7; tics[6] = 12;
    sems[5] = tSem1; tics[5] = 3;
    sems[4] = tSem6; tics[4] = 2;
    sems[3] = tSem4; tics[3] = 5;
    sems[2] = tSem5; tics[2] = 0;
    sems[1] = tSem3; tics[1] = 8;
    dc_test_state(sems, tics);

    tic = 31; Semaphore* tSem8 = createSemaphore("tSem8", 0, 0);
    printf("\nInserting tics %d and sem %s", tic, tSem8->name);
    dc_insert(dct, tSem8, tic);
    sems[1] = tSem8; tics[1] = 4;
    sems[0] = tSem3; tics[0] = 4;
    dc_test_state(sems, tics);




    printf("\nFinished inserting. Waiting for removals in order 27164583.");

    //Removal tests and final cleanup happen in dc_dec()

    return 0;
}


int dc_test_state(Semaphore** sems, int* tics)
{
    printf("\nChecking values... ");
    int i; for(i = dct->size-1; i > dct->bot; i--){
        if(strcmp(dct->elem[i]->sem->name, sems[i]->name) || dct->elem[i]->tic != tics[i]){
            printf("Problem found.");
            printf("\nExpected:");
            int j; for(j = dct->size-1; j > dct->bot; j--){
                printf("\n%d    %s", tics[j], sems[j]->name);
            }
            printf("\nFound:");
            dc_print(dct);
            return 0;
        }
    }

    printf("Correct.");
    return 1;
}
