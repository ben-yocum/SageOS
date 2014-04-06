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
Semaphore* dcChange;
Semaphore* dcMutex;
Semaphore* waitCar;
Semaphore* seatTaken[NUM_CARS];
Semaphore* seatLeave[NUM_CARS];
Semaphore* sleepingDrivers;
Semaphore* needDriverMutex;
Semaphore* holdVisitor[NUM_VISITORS];
Semaphore* ticket;
Semaphore* needTicket;
Semaphore* needDrive;
Semaphore* readyToDrive;
Semaphore* buyTicket;
Semaphore* ticketReady;
Semaphore* museum;
Semaphore* gift;
Semaphore* giftMutex;
Semaphore* park;
Semaphore* pausePark;
Semaphore* finished;
extern d_clock* dc;
extern int superMode;
extern void dc_print(d_clock*);



// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char**);
void CL3_dc(int, char**);
int driverTask();
int carTask(int, char**);
int visitorTask();
int dcount=0;
int currentCar;
int timelock=0;
int displayMessages=0;
char msgBuf[128];


/**
* Allocate memory for the semaphores, delta clock, cars, and visitors.
*/
int init_p3_semaphores()
{
    int i;
    char buf[32];
	char* newArgv[2];
    dc = dc_construct(1024);

	dcChange=createSemaphore("dcChange",0,1);
	dcMutex =createSemaphore("dcMutex", 0,1);
	waitCar =createSemaphore("waitCar", 0,0);

	for(i=0;i<NUM_CARS;i++)
	{
		sprintf(buf,"seatTaken%d",i);
		seatTaken[i]=createSemaphore(buf,0,0);
		sprintf(buf,"seatLeave%d",i);
		seatLeave[i]=createSemaphore(buf,1,0);
	}

	for(i=0;i<NUM_VISITORS;i++)
	{
		sprintf(buf,"holdVisitor%d",i);
		holdVisitor[i]=createSemaphore(buf,0,0);
	}

	pausePark       =createSemaphore("pausePark",      0,0);
	ticket          =createSemaphore("ticket",         1,MAX_TICKETS);
	needTicket      =createSemaphore("needTicket",     0,0);
	ticketReady     =createSemaphore("ticketReady",    0,0);
	buyTicket       =createSemaphore("buyTicket",      0,0);
	museum          =createSemaphore("museum",         1,MAX_IN_MUSEUM);
	gift            =createSemaphore("gift",           1,MAX_IN_GIFTSHOP);
	giftMutex       =createSemaphore("changeGift",     0,1);
	park            =createSemaphore("park",           1,MAX_IN_PARK);
	finished        =createSemaphore("finished",       1,0);
	needDriverMutex =createSemaphore("needDriverMutex",0,1);
	needDrive       =createSemaphore("needDrive",      0,0);
	readyToDrive    =createSemaphore("readyToDrive",   0,0);
	sleepingDrivers =createSemaphore("sleepingDrivers",0,0);

	return 0;
}




/**
* Run project 3.
*/
int P3_project3(int argc, char* argv[])
{
    char buf[32];
	char* newArgv[2];
	init_p3_semaphores();
	if(argc ==2)
        if(!strcmp(argv[1],"1"))
            displayMessages = 1;

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
	for(i=0; i<NUM_CARS; i++){
        sprintf(buf,"carTask%d",i);
		char **temparg=(char**)malloc(sizeof(char**));
		*temparg=(char*)malloc(sizeof(char));
		sprintf(temparg[0],"%d",i);
		createTask(buf,carTask,MED_PRIORITY,1,temparg);
	}

	//Create driver tasks
	for(i=1; i<=NUM_DRIVERS; i++){
        sprintf(buf,"driverTask%d",i);
		char **temparg=(char**)malloc(sizeof(char**));
		*temparg=(char*)malloc(sizeof(char));
		sprintf(temparg[0],"%d",i);
		createTask(buf,driverTask,MED_PRIORITY,1,temparg);
	}

	//Create visitor tasks
	for(i=1; i<=NUM_VISITORS; i++){
        sprintf(buf,"visitorTask%d",i);
		char **temparg=(char**)malloc(sizeof(char*));
		*temparg=(char*)malloc(sizeof(char)+1);
		sprintf(temparg[0],"%d",i);
		createTask(buf,visitorTask,MED_PRIORITY,1,temparg);
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
    int driverId;
	if(argc==1){
		driverId=atoi(argv[0]);
	}
	else{
		return -1;
	}

	while(1){
        printMsg();
		semWait(parkMutex);            SWAP
		myPark.drivers[driverId]=0;    SWAP
		semSignal(parkMutex);          SWAP
		semWait(sleepingDrivers);      SWAP
		if(semTryLock(needTicket)==1)
		{
		    sprintf(msgBuf, "driver%d getting ticket", driverId);
            printMsg();
			semWait(parkMutex);        SWAP
			myPark.drivers[driverId]=-1; SWAP
			semSignal(parkMutex);      SWAP
			semSignal(ticketReady);    SWAP
			semWait(buyTicket);        SWAP
			sprintf(msgBuf, "driver%d got ticket", driverId);
            printMsg;

		}
		else if(semTryLock(needDrive)==1){
            sprintf(msgBuf, "driver%d ready to drive", driverId);
            printMsg;
			semSignal(readyToDrive);   SWAP
            semWait(parkMutex);        SWAP
            myPark.drivers[driverId]=currentCar+1; SWAP
            semSignal(parkMutex);      SWAP
            semWait(seatTaken[currentCar]); SWAP
            sprintf(msgBuf, "driver%d driving", driverId);
            printMsg;
		}
	}



	return 0;
} //end driverTask



/**
* Sets up a car task. This task should never end, so
* it sits on a while(1) loop.
* Car waits for its seats to be filled, gets a driver, does the drive,
* and then drops the visitor off.
*/
int carTask(int argc, char* argv[]){
    int i;
	int carId;
	int visitor[2];
	if(argc==1){
		carId=atoi(argv[0]);
	}
	else{
		return -1;
	}

	while(1){

        SWAP

        for(i=0;i<2;i++)
        {
            semWait(fillSeat[carId]);     SWAP
            currentCar=carId;             SWAP
            semSignal(waitCar);           SWAP
            semSignal(seatFilled[carId]); SWAP
            sprintf(msgBuf, "car%d seat filled", carId);
            printMsg();
        }

        sprintf(msgBuf, "car%d full", carId);
        printMsg();
        semWait(fillSeat[carId]);         SWAP
        currentCar=carId;                 SWAP
        semSignal(waitCar);               SWAP
        sprintf(msgBuf, "car%d waiting driver", carId);
        printMsg;
        semWait(needDriverMutex);         SWAP
        semSignal(needDrive);             SWAP
        semSignal(sleepingDrivers);       SWAP
        currentCar=carId;                 SWAP
        semWait(readyToDrive);            SWAP
        semSignal(seatFilled[carId]);     SWAP
        semSignal(needDriverMutex);       SWAP
        sprintf(msgBuf, "car%d got driver, starting drive", carId);
        printMsg();


        //riding
        semWait(rideOver[carId]);         SWAP
        sprintf(msgBuf, "car%d ride over", carId);
        printMsg();

        //drop off visitor
        for(i=0;i<=NUM_SEATS;i++){
            semSignal(seatTaken[carId]);  SWAP
        }
	}
	return 0;
}



/**
* Visitor task. This task will eventually end (no while(1) loop).
* Visitor arrives outside the park at random time,
*         enters the park,
*         buys a ticket,
*         goes to the museum line,
*         goes into the museum,
*         leaves the museum,
*         enters the car,
*         leaves the car,
*         goes to the gift shop,
*         leaves the gift shop,
*         and leaves the park.
*/
int visitorTask(int argc, char* argv[]){
    int visitorId;
    //Get visitor ID / validate input
	if(argc==1){
		visitorId=atoi(argv[0]);
	}
	//Bad parameters passed in
	else{
        return -1;
	}

    //Set up random values
	int arriveTime=rand()%10*10+1;
	int tickettime=rand()%3*10+1;
 	int museumtime=rand()%3*10+1;
	int gifttime=rand()%3*10+1;

	SWAP

	//arrive outside park
	sprintf(msgBuf, "visitor%d outside park", visitorId);
    printMsg();
	semWait(parkMutex);             SWAP
	myPark.numOutsidePark++;        SWAP
	semSignal(parkMutex);           SWAP
	dc_insert(dc, holdVisitor[visitorId], arriveTime); SWAP;
	semWait(holdVisitor[visitorId]); SWAP


	//enter park.
	sprintf(msgBuf, "visitor%d entered park", visitorId);
    printMsg();
	semWait(park);                SWAP
	semWait(parkMutex);           SWAP
	myPark.numOutsidePark--;      SWAP
	myPark.numInPark++;           SWAP
	myPark.numInTicketLine++;     SWAP
	semSignal(parkMutex);         SWAP
	dc_insert(dc, holdVisitor[visitorId], tickettime); SWAP;
	semWait(holdVisitor[visitorId]); SWAP

	//buy ticket
	sprintf(msgBuf, "visitor%d buying ticket", visitorId);
    printMsg();
	semWait(ticket);              SWAP
	semWait(needDriverMutex);     SWAP
	semSignal(sleepingDrivers);   SWAP
	semSignal(needTicket);        SWAP
	semWait(ticketReady);         SWAP
	semSignal(buyTicket);         SWAP
	semSignal(needDriverMutex);   SWAP
	semWait(parkMutex);           SWAP
	myPark.numTicketsAvailable--; SWAP
	myPark.numInTicketLine--;     SWAP
	myPark.numInMuseumLine++;     SWAP
	semSignal(parkMutex);         SWAP

	//to museum line
	sprintf(msgBuf, "visitor%d to museum line", visitorId);
    printMsg();
	dc_insert(dc, holdVisitor[visitorId], museumtime); SWAP
	semWait(holdVisitor[visitorId]); SWAP

	//into museum
	sprintf(msgBuf, "visitor%d to museum", visitorId);
    printMsg();
	semWait(museum);              SWAP
	semWait(parkMutex);           SWAP
	myPark.numInMuseumLine--;     SWAP
	myPark.numInMuseum++;         SWAP
	semSignal(parkMutex);         SWAP
    dc_insert(dc, holdVisitor[visitorId], museumtime); SWAP
	semWait(holdVisitor[visitorId]); SWAP

	//leave museum
	sprintf(msgBuf, "visitor%d left museum", visitorId);
    printMsg();
	semWait(parkMutex);           SWAP
	semSignal(museum);            SWAP
	myPark.numInMuseum--;         SWAP
	myPark.numInCarLine++;        SWAP
	semSignal(parkMutex);         SWAP
	semWait(waitCar);             SWAP

	//enter car
	sprintf(msgBuf, "visitor%d entered car", visitorId);
    printMsg();
    semWait(parkMutex);           SWAP
	myPark.numInCarLine--;        SWAP
    myPark.numInCars++;           SWAP
    myPark.numTicketsAvailable++; SWAP
    semSignal(parkMutex);         SWAP
    semSignal(ticket);            SWAP
    semSignal(parkMutex);         SWAP
    semWait(seatTaken[currentCar]); SWAP
    semWait(parkMutex);           SWAP
    myPark.numInCars--;           SWAP
    myPark.numInGiftLine++;       SWAP
    semSignal(parkMutex);         SWAP

	//leave car,waiting gift
	sprintf(msgBuf, "visitor%d left car, waiting gift", visitorId);
    printMsg();
	dc_insert(dc, holdVisitor[visitorId], gifttime); SWAP
	semWait(holdVisitor[visitorId]); SWAP
	semWait(gift);                   SWAP

	//inside gift
	sprintf(msgBuf, "visitor%d entered gift shop", visitorId);
    printMsg();
	semWait(parkMutex);              SWAP
	myPark.numInGiftLine--;          SWAP
	myPark.numInGiftShop++;          SWAP
	semSignal(parkMutex);            SWAP
    dc_insert(dc, holdVisitor[visitorId], gifttime); SWAP

	//leave park
	sprintf(msgBuf, "visitor%d leaving park", visitorId);
    printMsg();
	semWait(holdVisitor[visitorId]); SWAP
	semWait(parkMutex);              SWAP
	myPark.numInGiftShop--;          SWAP
 	myPark.numInPark--;              SWAP
	myPark.numExitedPark++;          SWAP
	semSignal(gift);                 SWAP
	semSignal(park);                 SWAP
	semSignal(parkMutex);            SWAP
	semWait(finished);
	return 0;
}



int printMsg(){
    if(displayMessages){
        printf("\n%s", msgBuf);
        return 0;
    }
    return 1;
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

