// os345p3.c - Jurassic Park
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
#include <time.h>
#include <assert.h>
#include "os345.h"
#include "os345park.h"

#define DEFAULT_NUM_VISITORS 45

/*
typedef struct jpark
{
	int numOutsidePark;			// # outside of park
	int numInPark;				// # in park (P=#)
	int numTicketsAvailable;	// # left to sell (T=#)
	int numRidesTaken;			// # of tour rides taken (S=#)
	int numExitedPark;			// # who have exited the park
	int numInTicketLine;		// # in ticket line
	int numInMuseumLine;		// # in museum line
	int numInMuseum;			// # in museum
	int numInCarLine;			// # in tour car line
	int numInCars;				// # in tour cars
	int numInGiftLine;			// # in gift shop line
	int numInGiftShop;			// # in gift shop
	int drivers[NUM_DRIVERS];	// driver state (-1=T, 0=z, 1=A, 2=B, etc.)
	CAR cars[NUM_CARS];			// cars in park
} JPARK;
*/

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];			// (signal) ride over

extern d_clock* dc;
extern void dc_print(d_clock*);


// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char**);
void CL3_dc(int, char**);
int driverTask();
int carTask(int, char**);
int visitorTask();


// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	char buf[32];
	char* newArgv[2];

	// start park
	sprintf(buf, "jurassicPark");
	newArgv[0] = buf;
	createTask( buf,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,							// task count
		newArgv);					// task argument

	// wait for park to get initialized...
	while (!parkMutex) SWAP;
	printf("\nStart Jurassic Park...");

	int i;

	//Create car tasks
	for(i=1; i<=NUM_CARS; i++){
        char cn[6]; //supports up to 9999 cars
        sprintf(cn, "car%d", i);
        char* temp[1];
        //temp[0] = itoa(i);
        createTask(cn, carTask, MED_PRIORITY, 0, NULL);
	}

	//Create driver tasks
	for(i=1; i<=NUM_DRIVERS; i++){
        char dn[9]; //supports up to 9999 drivers
        sprintf(dn, "driver%d", i);
        createTask(dn, driverTask, MED_PRIORITY, 0, NULL);
	}

	//Create visitor tasks
	for(i=1; i<=NUM_VISITORS; i++){
        char vn[10]; //supports up to 9999 visitors
        sprintf(vn, "visitor%d", i);
        createTask(vn, visitorTask, MED_PRIORITY, 0, NULL);
	}

	return 0;
} // end project3



// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	dc_print(dc);

	return 0;
} // end CL3_dc



//Need to do: Updating variables that are to be printed
int driverTask(int argc, char* argv[])
{
    /*char buf[32];
    Semaphore* driverDone;
    int myID = atoi(argv[1])-1;                     SWAP;
    printf("Starting driverTask%d", myID+1);        SWAP;
    sprintf(buf, "driverDone%d", myID + 1);         SWAP;
    driverDone = createSemaphore(buf, BINARY, 0);   SWAP;

    while(1)
    {
        SEM_WAIT(wakeupDriver);                     SWAP;
        if(SEM_TRYLOCK(needDriver))                 SWAP;
        {

            driverDoneSemaphore = driverDone;       SWAP;
            SEM_SIGNAL(driverReady);                SWAP;
            SEM_WAIT(carReady);                     SWAP;
            SEM_WAIT(driverDone);                   SWAP;
        }
        else if (mySEM_TRYLOCK(needTicket))
        {
            SEM_WAIT(tickets);                      SWAP;
            SEM_SIGNAL(takeTicket);                 SWAP;
        }
        else break;
    }*/
    printf("\nStarting driverTask");
    return 0;
} //end driverTask




int carTask(int argc, char* argv[]){

    /*int carID = atoi(argv[0]);

    //fill all seats
    int i; for(i=0; i<NUM_SEATS; i++){
        SEM_WAIT(fillSeat[carID]);          SWAP;
        //SEM_SIGNAL(getPassenger);           SWAP;
        //SEM_WAIT(seatTaken);                SWAP;
        SEM_SIGNAL(seatFilled);	            SWAP;

    }

    //once seats full, get driver
    SEM_WAIT(needDriverMutex);          SWAP;
    SEM_SIGNAL(wakeupDriver);           SWAP;
    SEM_SIGNAL(needDriverMutex);        SWAP;

    //if car full, wait until ride over
    SEM_WAIT(rideOver[carID]);               SWAP;
    SEM_SIGNAL(driverDone);                 SWAP;
    SEM_SIGNAL(rideDone[i]);                SWAP;
    printf("\nStarting carTask");*/
    return 0;
}


int visitorTask(int argc, char* argv[]){
    printf("\nStarting visitorTask");
    return 0;
}


/*
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	// ?? Implement a routine to display the current delta clock contents
	//printf("\nTo Be Implemented!");
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
	}
	return 0;
} // end CL3_dc


// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
	}
	return;
}


// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char* argv[])
{
	createTask( "DC Test",			// task name
		dcMonitorTask,		// task
		10,					// task priority
		argc,					// task arguments
		argv);

	timeTaskID = createTask( "Time",		// task name
		timeTask,	// task
		10,			// task priority
		argc,			// task arguments
		argv);
	return 0;
} // end P3_tdc



// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char* argv[])
{
	int i, flg;
	char buf[32];
	// create some test times for event[0-9]
	int ttime[10] = {
		90, 300, 50, 170, 340, 300, 50, 300, 40, 110	};

	for (i=0; i<10; i++)
	{
		sprintf(buf, "event[%d]", i);
		event[i] = createSemaphore(buf, BINARY, 0);
		insertDeltaClock(ttime[i], event[i]);
	}
	printDeltaClock();

	while (numDeltaClock > 0)
	{
		SEM_WAIT(dcChange)
		flg = 0;
		for (i=0; i<10; i++)
		{
			if (event[i]->state ==1)			{
					printf("\n  event[%d] signaled", i);
					event[i]->state = 0;
					flg = 1;
				}
		}
		if (flg) printDeltaClock();
	}
	printf("\nNo more events in Delta Clock");

	// kill dcMonitorTask
	tcb[timeTaskID].state = S_EXIT;
	return 0;
} // end dcMonitorTask


extern Semaphore* tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
	char svtime[64];						// ascii current time
	while (1)
	{
		SEM_WAIT(tics1sec)
		printf("\nTime = %s", myTime(svtime));
	}
	return 0;
} // end timeTask
*/

