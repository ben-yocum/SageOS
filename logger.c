#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "os345.h"
#include "logger.h"

#define FILE_NAME "log.txt"
#define TIME_BUFFER_SIZE 20

char* getTime(void);
char buffer[TIME_BUFFER_SIZE];
extern Semaphore* parkMutex;
extern Semaphore* sysMutex;

int printed = 0;

void logg(char* toLog){

    FILE *f = fopen(FILE_NAME, "a+");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "\n%s >> %s", getTime(), toLog);

    fclose(f);
    return;

}

void logln(void){

    FILE *f = fopen(FILE_NAME, "a+");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "\n");

    fclose(f);
    return;
}

char* getTime(void){
      time_t rawtime;
      struct tm * timeinfo;

      time (&rawtime);
      timeinfo = localtime (&rawtime);

      strftime (buffer,TIME_BUFFER_SIZE,"%d-%m-%y %H:%M:%S",timeinfo);

      return buffer;
}

/**
* @param pk a pointer to an array of c-strings
*/
void d_park(char pk[25][80]){
    SEM_WAIT(parkMutex);
    SEM_WAIT(sysMutex);
    FILE *f;

    if(printed++ < 100)
        {f = fopen("pk.txt", "a+");          SWAP;}
    else{
        f = fopen("pk.txt", "w");         SWAP;
        printed = 0;
    }


    int i;                                  SWAP;

    //clear screen
    for(i=0; i<30; i++){
        fprintf(f, "\n");                   SWAP;
    }

    //draw park
	fprintf(f, "\n");						SWAP;
	for (i=0; i<25; i++)
	    fprintf(f, "\n%s", pk[i]);			SWAP;
	fprintf(f, "\n");						SWAP;

	fclose(f);                              SWAP;
	SEM_SIGNAL(sysMutex);
	SEM_SIGNAL(parkMutex);
}
