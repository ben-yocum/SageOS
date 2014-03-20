// os45park.c - Jurassic Park	10/24/2013
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

JPARK myPark;
Semaphore* parkMutex;						// mutex park variable access
Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
Semaphore* seatFilled[NUM_CARS];			// (wait) passenger seated
Semaphore* rideOver[NUM_CARS];			// (signal) ride over

extern TCB tcb[];							// task control block
extern int curTask;
extern Semaphore* tics1sec;				// 1 second semaphore

Semaphore* moveCars;
int makeMove(int car);
void drawPark(JPARK *park);


// ***********************************************************************
// ***********************************************************************
// jurassic task
// ***********************************************************************
int jurassicTask(int argc, char* argv[])
{
	int i, j, k;
	char buf[32];
	int c[NUM_CARS];

	// initial car locations
	for (i=0; i < NUM_CARS; i++)
	{
		myPark.cars[i].location = 33 - i;			SWAP;
		myPark.cars[i].passengers = 0;				SWAP;
	}

	// drivers are all asleep
	for (i=0; i < NUM_DRIVERS; i++)
	{
		myPark.drivers[i] = 0;						SWAP;
	}

	// initialize other park variables
	myPark.numOutsidePark = 0;						SWAP;			// # trying to enter park
	myPark.numTicketsAvailable = MAX_TICKETS;		SWAP;			// T=#
	myPark.numInPark = 0;							SWAP;			// P=#
	myPark.numRidesTaken = 0;						SWAP;			// S=#

	myPark.numInTicketLine = 0;						SWAP;			// Ticket line
	myPark.numInMuseumLine = 0;						SWAP;			// Museum line
	myPark.numInMuseum = 0;							SWAP;			// # in museum
	myPark.numInCarLine = 0;						SWAP;			// Ride line
	myPark.numInCars = 0;							SWAP;			// # in cars
	myPark.numInGiftLine = 0;						SWAP;			// Gift shop line
	myPark.numInGiftShop = 0;						SWAP;			// # in gift shop
	myPark.numExitedPark = 0;						SWAP;			// # exited park

	// create move car signal semaphore
	moveCars = createSemaphore("moveCar", BINARY, 0);		SWAP;

	// initialize communication semaphores
	for (i=0; i < NUM_CARS; i++)
	{
		sprintf(buf, "fillSeat%d", i);						SWAP;
		fillSeat[i] = createSemaphore(buf, BINARY, 0);		SWAP;
		sprintf(buf, "seatFilled%d", i);					SWAP;
		seatFilled[i] = createSemaphore(buf, BINARY, 0);	SWAP;
		sprintf(buf, "rideOver%d", i);						SWAP;
		rideOver[i] = createSemaphore(buf, BINARY, 0);		SWAP;
	}

	// start display park task
	createTask("displayPark",	// task name
		jurassicDisplayTask,		// task
		MED_PRIORITY,				// task priority
		1,								// task count
		argv);						// task argument

	// OK, let's get the park going...
	parkMutex = createSemaphore("parkMutex", BINARY, 1);	SWAP;

	// start lost visitor task
	createTask("lostVisitor",	// task name
		lostVisitorTask,			// task
		MED_PRIORITY,				// task priority
		1,								// task count
		argv);						// task argument

	// wait to move cars
	do
	{
   	// wait for signal to move cars
		ParkDebug("SEM_WAIT(moveCars)");
		SEM_WAIT(moveCars);									SWAP;

		// order car locations
		for (i=0; i<NUM_CARS; i++)
		{
			c[i] = i;										SWAP;
		}

		// sort car positions
		for (i=0; i<(NUM_CARS-1); i++)
		{
			for (j=i+1; j<NUM_CARS; j++)
			{
				if (myPark.cars[c[i]].location < myPark.cars[c[j]].location)
				{
					k = c[i];								SWAP;
					c[i] = c[j];							SWAP;
					c[j] = k;								SWAP;
				}
			}
		}

		// move each car if possible (or load or unload)
		for (i=0; i < NUM_CARS; i++)
		{
			// move car (-1=location occupied, 0=moved, 1=no move)
			if (makeMove(c[i]))
			{
				// check if car is loading
				if ((myPark.cars[c[i]].location == 33) &&
					 (myPark.cars[c[i]].passengers < NUM_SEATS) &&
				    (myPark.numInCarLine) )
				{
					// need a passenger
					sprintf(buf, "SEM_SIGNAL(fillSeat[%d])", c[i]);
					ParkDebug(buf);
					SEM_SIGNAL(fillSeat[c[i]]);					SWAP;	// seat open

					sprintf(buf, "SEM_WAIT(seatFilled[%d])", c[i]);
					ParkDebug(buf);
					SEM_WAIT(seatFilled[c[i]]);					SWAP;	// passenger in seat
					myPark.cars[c[i]].passengers++;				SWAP;
				}

				// check if car is unloading
				if ((myPark.cars[c[i]].location == 30) &&
					 (myPark.cars[c[i]].passengers > 0))
				{
					// empty each seat until car is empty
					myPark.numRidesTaken++;						SWAP;

					// if car empty, signal ride over
					if (--myPark.cars[c[i]].passengers == 0)
					{
						sprintf(buf, "SEM_SIGNAL(rideOver[%d])", c[i]);
						ParkDebug(buf);
						SEM_SIGNAL(rideOver[c[i]]);				SWAP;
					}
				}

				// sanity check
				if (	(myPark.cars[0].location == myPark.cars[1].location) ||
						(myPark.cars[0].location == myPark.cars[2].location) ||
						(myPark.cars[0].location == myPark.cars[3].location) ||
						(myPark.cars[1].location == myPark.cars[2].location) ||
						(myPark.cars[1].location == myPark.cars[3].location) ||
						(myPark.cars[2].location == myPark.cars[3].location) )
				{
					printf("\nProblem:");
					for (k=0; k<NUM_CARS; k++) printf(" %d", myPark.cars[k].location);
					k=getchar();
				}
			}
		}
	} while (myPark.numExitedPark < NUM_VISITORS);

	// park done
	printf("\nJurassic Park is shutting down for the evening!!");	SWAP;
	killTask(-1);

	// should never get here!!
	return 0;
} // end



//***********************************************************************
// Move car in Jurassic Park (if possible)
//
//	return -1 if location occupied
//			  0 if move ok
//         1 if no move
//
int makeMove(int car)
{
	int i, j;
	int moved;

	switch(j = myPark.cars[car].location)
	{
		// at cross roads 1
		case 7:
		{
			j = (rand()%2) ? 24 : 8;								SWAP;
			break;
		}

		// at cross roads 2
		case 20:
		{
			// take shorter route if no one in line
			if (myPark.numInCarLine == 0)
			{
				j = 28;												SWAP;
			}
			else
			{
				j = (rand()%3) ? 28 : 21;							SWAP;
			}
			break;
		}

		// bridge loop
		case 23:
		{
			j = 1;													SWAP;
			break;
		}

		// bridge short route
		case 27:
		{
			j = 20;													SWAP;
			break;
		}

		// exit car
		case 30:
		{
			if (myPark.cars[car].passengers == 0)
			{
				j++;												SWAP;
			}
			break;
		}

		// loading car
		case 33:
		{
			// if there is someone in car and noone is in line, proceed
			//if ((myPark.cars[car].passengers == NUM_SEATS) ||
			//	 ((myPark.numInCarLine == 0) && myPark.cars[car].passengers))

			// if car is full, proceed into park
			if (myPark.cars[car].passengers == NUM_SEATS)
			{
				j = 0;												SWAP;
			}
			break;
		}

		// all other moves
		default:
		{
			j++;													SWAP;
			break;
		}
	}
	// check to see if position is taken
	for (i=0; i<NUM_CARS; i++)
	{
		if (i != car)
		{
			SWAP;
			if (myPark.cars[i].location == j)
			{
				return -1;
			}
		}
	}

	// return 0 if car moved
	moved = (myPark.cars[car].location == j);					SWAP;

	// make move
	myPark.cars[car].location = j;								SWAP;
	return moved;
}



// ***********************************************************************
// ***********************************************************************
// jurassic task
// ***********************************************************************
int jurassicDisplayTask(int argc, char* argv[])
{
	// wait for park to get initialized...
	while (!parkMutex) SWAP;

	// display park every second
	do
	{
		JPARK currentPark;

   	// update every second
		SEM_WAIT(tics1sec);											SWAP;

		// take snapshot of park
		SEM_WAIT(parkMutex);										SWAP;
		currentPark = myPark;										SWAP;
		SEM_SIGNAL(parkMutex);										SWAP;

		// draw current park
		drawPark(&currentPark);										SWAP;

		// signal for cars to move
		SEM_SIGNAL(moveCars);										SWAP;

	} while (myPark.numExitedPark < NUM_VISITORS);

	// park done
	printf("\nThank you for visiting Jurassic Park!!");
	return 0;
} // end



//***********************************************************************
// Draw Jurassic Park
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890
//                _______________________________________________________	0
//   Entrance    /            ++++++++++++++++++++++++++++++++++++++++++|	1
//              -             +o0o-o1o-o2o-o3o-o4o-o5o-o6o             +|	2
//          ## -             /+   /                       \            +|	3
//            /             o +  o     ************        o           +|	4
//   |********         ##>>33 + 23     *Cntrl Room*        7           +|	5
//   |*Ticket*              o +  o     * A=# D1=# *        o           +|	6
//   |*Booth *             32 +  |     * B=# D2=# *       / \          +|	7
//   |* T=#  * ##           o +  o     * C=# D3=# *      o   o8o-o9o   +|	8
//   |* P=## *             31 + 22     * D=# D4=# *    24           \  +|	9
//   |* S=#  *              o +  o     ************    o             o +|	10
//   |********         ##<<30 +   \                   /             10 +|	11
//   |                      o +    o21-o20-o27-o26-o25               o +|	12
//   |                       \+       /   \                          | +|	13
//   |                        +o29-o28     o                         o +|	14
//   |                        +++++++++++ 19                        11 +|	15
//   |                                  +  o                         o +|	16
//   |              ##     ##           +   \                       /  +|	17
//   |        ******\ /****\ /********  +    o  O\                 o   +|	18
//   |        *         *            *  +   18    \/|||\___       12   +|	19
//    \       *  Gifts  *   Museum   *  +    o     x   x           o   +|	20
//     -      *    #    *     ##     *  +     \                   /    +|	21
//   ## -     *         *            *  +      o17-o16-o15-o14-o13     +|	22
//       \    ************************  ++++++++++++++++++++++++++++++++|	23
//
#define D1Upper	13
#define D1Left	45
#define D1Lower	18
#define D1Right	53

#define D2Upper	4
#define D2Left	53
#define D2Lower	13
#define D2Right	57

void drawPark(JPARK *park)
{
	static int direction1 = D1Left;
	static int dy1 = D1Lower;

	static int direction2 = D2Left;
	static int dy2 = D1Lower-9;

	int i, j;
	char svtime[64];						// ascii current time
	char driver[] = {'T', 'z', 'A', 'B', 'C', 'D' };
	char buf[32];
	char pk[25][80];
	char cp[34][3] = {	{2, 29, 0}, {2, 33, 0}, {2, 37, 0}, {2, 41, 0},				// 0-6
								{2, 45, 0}, {2, 49, 0}, {2, 53, 0},
								{4, 57, 1},											// 7
								{8, 59, 0}, {8, 63, 0},								// 8-9
								{10, 67, 1}, {14, 67, 1}, {18, 65, 1},				// 10-12
								{22, 61, 0}, {22, 57, 0}, {22, 53, 0},				// 13-17
								{22, 49, 0}, {22, 45, 0},
							  	{18, 43, 1}, {14, 41, 1},							// 18-19
								{12, 37, 0}, {12, 33, 0}, {8, 31, 1}, {4, 31, 1},	// 20-23
								{8, 55, 3},											// 24
							  	{12, 49, 0}, {12, 45, 0}, {12, 41, 0},				// 25-27
								{14, 33, 0}, {14, 29, 0},							// 28-29
								{10, 26, 1}, {8, 26, 1}, {6, 26, 1}, {4, 26, 1},	// 30-33
							};

	strcpy(&pk[0][0],  "                ___Jurassic Park_______________________________________");
	strcpy(&pk[1][0],  "   Entrance    /            ++++++++++++++++++++++++++++++++++++++++++|");
	strcpy(&pk[2][0],  "              -             +---------------------------             +|");
	strcpy(&pk[3][0],  "           # -             /+   /                       \\            +|");
	strcpy(&pk[4][0],  "            /             | +  |     ************        |           +|");
	strcpy(&pk[5][0],  "   |********          # >>| +  |     *Cntrl Room*        |           +|");
	strcpy(&pk[6][0],  "   |*Ticket*              | +  |     * A=# D1=# *        |           +|");
	strcpy(&pk[7][0],  "   |*Booth *              | +  |     * B=# D2=# *       / \\          +|");
	strcpy(&pk[8][0],  "   |* T=#  * #            | +  |     * C=# D3=# *      /   -------   +|");
	strcpy(&pk[9][0],  "   |* P=#  *              | +  |     * D=# D4=# *     /           \\  +|");
	strcpy(&pk[10][0], "   |* S=#  *              | +  |     ************    /             | +|");
	strcpy(&pk[11][0], "   |********            <<| +   \\                   /              | +|");
	strcpy(&pk[12][0], "   |                      | +    -------------------               | +|");
	strcpy(&pk[13][0], "   |                       \\+       /   \\                          | +|");
	strcpy(&pk[14][0], "   |                        +-------     |                         | +|");
	strcpy(&pk[15][0], "   |                        +++++++++++  |                         | +|");
	strcpy(&pk[16][0], "   |                                  +  |                         | +|");
	strcpy(&pk[17][0], "   |                #      #          +   \\                       /  +|");
	strcpy(&pk[18][0], "   |        ******\\ /****\\ /********  +    |                     |   +|");
	strcpy(&pk[19][0], "   |        *         *            *  +    |                     |   +|");
	strcpy(&pk[20][0], "    \\       *  Gifts  *   Museum   *  +    |                     |   +|");
	strcpy(&pk[21][0], "     -      *         *            *  +     \\                   /    +|");
	strcpy(&pk[22][0], "    # -     *         *            *  +      -------------------     +|");
	strcpy(&pk[23][0], "       \\    ************************  ++++++++++++++++++++++++++++++++|");
	strcpy(&pk[24][0], "   Exit \\_____________________________________________________________|");

	// output time
	sprintf(buf, "%s", myTime(svtime));									SWAP;
	memcpy(&pk[0][strlen(pk[0]) - strlen(buf)], buf, strlen(buf));		SWAP;

	// out number waiting to get into park
	sprintf(buf, "%d", park->numOutsidePark);							SWAP;
	memcpy(&pk[3][12 - strlen(buf)], buf, strlen(buf));					SWAP;

	// T=#, out number of tickets available
	sprintf(buf, "%d ", park->numTicketsAvailable);						SWAP;
	memcpy(&pk[8][8], buf, strlen(buf));								SWAP;

	// P=#, out number in park (not to exceed OSHA requirements)
	sprintf(buf, "%d ", park->numInPark);								SWAP;
	memcpy(&pk[9][8], buf, strlen(buf));								SWAP;

	// S=#, output guests completing ride
	sprintf(buf, "%d", park->numRidesTaken);							SWAP;
	memcpy(&pk[10][8], buf, strlen(buf));								SWAP;

	// out number in ticket line
	sprintf(buf, "%d ", park->numInTicketLine);							SWAP;
	memcpy(&pk[8][13], buf, strlen(buf));								SWAP;

	// out number in gift shop line
	sprintf(buf, "%d", park->numInGiftLine);							SWAP;
	memcpy(&pk[17][21 - strlen(buf)], buf, strlen(buf));				SWAP;

	// out number in museum line
	sprintf(buf, "%d", park->numInMuseumLine);							SWAP;
	memcpy(&pk[17][28 - strlen(buf)], buf, strlen(buf));				SWAP;

	// out number in car line
	sprintf(buf, "%d", park->numInCarLine);								SWAP;
	memcpy(&pk[5][23 - strlen(buf)], buf, strlen(buf));					SWAP;

	// out number in gift shop
	sprintf(buf, "%d ", park->numInGiftShop);							SWAP;
	memcpy(&pk[21][17], buf, strlen(buf));								SWAP;

	// out number in museum
	sprintf(buf, "%d ", park->numInMuseum);								SWAP;
	memcpy(&pk[21][29], buf, strlen(buf));								SWAP;

	// out number exited park
	sprintf(buf, "%d", park->numExitedPark);							SWAP;
	memcpy(&pk[22][5 - strlen(buf)], buf, strlen(buf));					SWAP;

	// cars
	for (i=0; i<NUM_CARS; i++)
	{
		sprintf(buf, "%d", park->cars[i].passengers);					SWAP;
		memcpy(&pk[6+i][42 - strlen(buf)], buf, strlen(buf));			SWAP;
	}

	// drivers
	for (i=0; i<NUM_DRIVERS; i++)
	{
		pk[6+i][46] = driver[park->drivers[i] + 1];						SWAP;
	}

	// output cars
	for (i=0; i<NUM_CARS; i++)
	{
		// draw car
		j = park->cars[i].location;												SWAP;
		switch (cp[j][2])
		{
			// horizontal
			case 0:
			{
				pk[cp[j][0]][cp[j][1]+0] = 'o';									SWAP;
				pk[cp[j][0]][cp[j][1]+1] = 'A'+i;								SWAP;
				pk[cp[j][0]][cp[j][1]+2] = 'o';									SWAP;
				break;
			}
			// vertical
			case 1:
			{
				pk[cp[j][0]+0][cp[j][1]] = 'o';									SWAP;

				//pk[cp[j][0]+1][cp[j][1]] = 'A'+i;
				//if ((park->cars[i].passengers > 0) && (park->cars[i].passengers < NUM_SEATS))
				if ((park->cars[i].passengers > 0) &&
					((j == 30) || (j == 33)))
				{
					pk[cp[j][0]+1][cp[j][1]] = '0'+park->cars[i].passengers;	SWAP;
				}
				else pk[cp[j][0]+1][cp[j][1]] = 'A'+i;							SWAP;

				pk[cp[j][0]+2][cp[j][1]] = 'o';									SWAP;
				break;
			}
			case 2:
			{
				pk[cp[j][0]+0][cp[j][1]+0] = 'o';								SWAP;
				pk[cp[j][0]+1][cp[j][1]+1] = 'A'+i;								SWAP;
				pk[cp[j][0]+2][cp[j][1]+2] = 'o';								SWAP;
				break;
			}
			case 3:
			{
				pk[cp[j][0]+0][cp[j][1]-0] = 'o';								SWAP;
				pk[cp[j][0]+1][cp[j][1]-1] = 'A'+i;								SWAP;
				pk[cp[j][0]+2][cp[j][1]-2] = 'o';								SWAP;
				break;
			}
		}
	}

	// move dinosaur #1
	dy1 = dy1 + (rand()%3) - 1;													SWAP;
	if (dy1 < D1Upper) dy1 = D1Upper;											SWAP;
	if (dy1 > D1Lower) dy1 = D1Lower;											SWAP;

	if (direction1 > 0)
	{
		memcpy(&pk[dy1+0][direction1+4], "...  /O", 7);						SWAP;
		memcpy(&pk[dy1+1][direction1+0], "___/|||\\/", 9);					SWAP;
		memcpy(&pk[dy1+2][direction1+3], "x   x", 5);						SWAP;
		if (++direction1 > D1Right) direction1 = -direction1;				SWAP;
	}
	else
	{
		if ((rand()%3) == 1)
		{
			memcpy(&pk[dy1+0][-direction1+4], "...", 3);						SWAP;
			memcpy(&pk[dy1+1][-direction1+1], "__/|||\\___", 10);			SWAP;
			memcpy(&pk[dy1+2][-direction1+0], "O  x   x", 8);				SWAP;
		}
		else
		{
			memcpy(&pk[dy1+0][-direction1+0], "O\\  ...", 7);				SWAP;
			memcpy(&pk[dy1+1][-direction1+2], "\\/|||\\___", 9);			SWAP;
			memcpy(&pk[dy1+2][-direction1+3], "x   x", 5);					SWAP;
		}
		if (++direction1 > -D1Left) direction1 = -direction1;				SWAP;
	}

	// move dinosaur #2

	dy2 = dy2 + (rand()%3) - 1;
	if (dy2 < D2Upper) dy2 = D2Upper;											SWAP;
	if (dy2 > D2Lower) dy2 = D2Lower;											SWAP;
	dy2 = (dy2+9) >= dy1 ? dy1-9 : dy2;											SWAP;

	if (direction2 > 0)
	{
		memcpy(&pk[dy2+0][direction2+7], "_", 1);								SWAP;
		memcpy(&pk[dy2+1][direction2+6], "/o\\", 3);							SWAP;
		memcpy(&pk[dy2+2][direction2+4], "</ _<", 5);						SWAP;
		memcpy(&pk[dy2+3][direction2+3], "</ /", 4);							SWAP;
		memcpy(&pk[dy2+4][direction2+2], "</ ==x", 6);						SWAP;
		memcpy(&pk[dy2+5][direction2+3], "/  \\", 4);						SWAP;
		memcpy(&pk[dy2+6][direction2+2], "//)__)", 6);						SWAP;
		memcpy(&pk[dy2+7][direction2+0], "<<< \\_ \\_", 9);				SWAP;
		if (++direction2 > D2Right) direction2 = -direction2;				SWAP;
	}
	else
	{
		memcpy(&pk[dy2+0][-direction2+1], "_", 1);							SWAP;
		memcpy(&pk[dy2+1][-direction2+0], "/o\\", 3);						SWAP;
		memcpy(&pk[dy2+2][-direction2+0], ">_ \\>", 5);						SWAP;
		memcpy(&pk[dy2+3][-direction2+2], "\\ \\>", 4);						SWAP;
		memcpy(&pk[dy2+4][-direction2+1], "x== \\>", 6);					SWAP;
		memcpy(&pk[dy2+5][-direction2+2], "/  \\", 4);						SWAP;
		memcpy(&pk[dy2+6][-direction2+1], "(__(\\\\", 6);					SWAP;
		memcpy(&pk[dy2+7][-direction2+0], "_/ _/ >>>", 9);					SWAP;
		if (++direction2 > -D2Left) direction2 = -direction2;				SWAP;
	}

    d_park(pk);

	// driver in only one place at a time
	for (i=0; i<(NUM_DRIVERS-1); i++)
	{
		if (park->drivers[i] != 0)
		{
			for (j=i+1; j<NUM_DRIVERS; j++)
			{
				assert("Driver Error" && (park->drivers[i] != park->drivers[j]));
			}
		}
	}

	return;
} // end drawPark



// ***********************************************************************
// ***********************************************************************
// lostVisitor task
// ***********************************************************************
int lostVisitorTask(int argc, char* argv[])
{
	int inPark;

	while (myPark.numExitedPark < NUM_VISITORS)
	{
		// number in park
		SEM_WAIT(parkMutex);												SWAP;
		inPark = myPark.numInTicketLine +
					myPark.numInMuseumLine +
					myPark.numInMuseum +
					myPark.numInCarLine +
					myPark.numInCars +
					myPark.numInGiftLine +
					myPark.numInGiftShop;								SWAP;

		if (inPark != myPark.numInPark)
		{
			printf("\nSomeone is lost!!!");
			printf("\nThere are %d visitors in the park,", myPark.numInPark);
			printf("\nbut I can only find %d of them!\n", inPark);

			printf("\n      numInPark=%d", myPark.numInPark);
			printf("\n         inPark=%d", inPark);
			printf("\nnumInTicketLine=%d", myPark.numInTicketLine);
			printf("\nnumInMuseumLine=%d", myPark.numInMuseumLine);
			printf("\n    numInMuseum=%d", myPark.numInMuseum);
			printf("\n   numInCarLine=%d", myPark.numInCarLine);
			printf("\n      numInCars=%d", myPark.numInCars);
			printf("\n  numInGiftLine=%d", myPark.numInGiftLine);
			printf("\n  numInGiftShop=%d", myPark.numInGiftShop);
			printf("\n");

			assert("Too few in Park!" && (inPark == myPark.numInPark));
		}
		SEM_SIGNAL(parkMutex);											SWAP;
	}
	return 0;
} // end lostVisitorTask

