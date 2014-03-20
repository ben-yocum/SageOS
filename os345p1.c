// os345p1.c - Command Line Processor
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
#include <assert.h>
#include <time.h>

#include "os345.h"
#include "os345signals.h"

// The 'reset_context' comes from 'main' in os345.c.  Proper shut-down
// procedure is to long jump to the 'reset_context' passing in the
// power down code from 'os345.h' that indicates the desired behavior.
extern jmp_buf reset_context;
// -----


#define NUM_COMMANDS 56
typedef struct                                // command struct
{
    char* command;
    char* shortcut;
    int (*func)(int, char**);
    char* description;
} Command;

// ***********************************************************************
// project 1 variables
//
extern long swapCount;                    // number of scheduler cycles
extern char inBuffer[];                    // character input buffer
extern Semaphore* inBufferReady;        // input buffer ready semaphore
extern bool diskMounted;                // disk has been mounted
extern char dirPath[];                    // directory path


Command** commands;                        // shell commands


// ***********************************************************************
// project 1 prototypes
Command** P1_init(void);
Command* newCommand(char*, char*, int (*func)(int, char**), char*);
void addBuffHist(char*);
int C_clearBuffHist(int argc, char* argv[]);
int C_clearBuffHist(int argc, char* argv[]);

extern int dc_test(void);
extern void clearBuffHist();



// ***********************************************************************
// myShell - command line interpreter
//
// Project 1 - implement a Shell (CLI) that:
//
// 1. Prompts the user for a command line.
// 2. WAIT's until a user line has been entered.
// 3. Parses the global char array inBuffer.
// 4. Creates new argc, argv variables using malloc.
// 5. Searches a command list for valid OS commands.
// 6. If found, perform a function variable call passing argc/argv variables.
// 7. Supports background execution of non-intrinsic commands.
//
int P1_shellTask(int argc, char* argv[])
{
    int i, found, newArgc;                     // # of arguments
    char** newArgv;                            // pointers to arguments

    // initialize shell commands
    commands = P1_init();                      // init shell commands

    while (1)
    {
        // output prompt
        if (diskMounted) printf("\n%s>>", dirPath);
        else printf("\n%ld>>", swapCount);

        SEM_WAIT(inBufferReady);            // wait for input buffer semaphore
        if (!inBuffer[0]) continue;         // ignore blank lines


        SWAP                                        // do context switch

        {

            static char *sp, *myArgv[MAX_ARGS];

            // init arguments
            newArgc = 1;                              // initialize counter to second member of myArgv
            myArgv[0] = sp = inBuffer;                // set the first member of myArgv to be the string
            int l = 0;
            while(myArgv[0][l] && myArgv[0][l] != ' '){
                myArgv[0][l] = tolower(myArgv[0][l]);
                l++;
            }

            for (i=1; i<MAX_ARGS; i++)                // fill myArgv with 0's
                myArgv[i] = 0;

            // parse input string
            while ((sp = strchr(sp, ' ')))            // sp gets address of ' ' characters
            {
                if(*(sp+1) == '"'){
                   *sp++=0; //set ' ' to null
                   *sp++=0; //set '"' to null
                   myArgv[newArgc++] = sp; //New argument added
                   sp = strchr(sp, '"');
                   *sp++ = 0;
                } else{
                    *sp++ = 0;                            // set the ' ' to NULL
                    myArgv[newArgc] = sp;               // set the next member of myArgv to the first char of next word,
                                                          // increment counter
                    //lower case myArgv[newArgc] here
                    int l = 0;
                    while(myArgv[newArgc][l] && myArgv[newArgc][l] != ' '){
                        myArgv[newArgc][l] = tolower(myArgv[newArgc][l]);
                        l++;
                    }

                    newArgc++;
                }
            }

			int i;
            char** temp = (char **)malloc(sizeof(char*) * MAX_ARGS);
            for(i = 0; i < newArgc; i++){
                temp[i] = (char *)malloc(  ( (strlen(myArgv[i]) +1)  * sizeof(char)  ));
                strcpy(temp[i], myArgv[i]);
            }

            newArgv = temp;                         //save temporary array to the method-scope array

        }    // ?? >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        // look for command
        for (found = i = 0; i < NUM_COMMANDS; i++)
        {
            if (!strcmp(newArgv[0], commands[i]->command) ||
                 !strcmp(newArgv[0], commands[i]->shortcut))
            {
                int retValue;

                // command found
                if(strcmp(newArgv[newArgc-1], "&") == 0)
                    createTask(newArgv[0], *commands[i]->func, MED_PRIORITY, newArgc, newArgv);
                else
                    retValue = (*commands[i]->func)(newArgc, newArgv);

                if (retValue) printf("\nCommand Error %d", retValue);
                found = TRUE;
                break;
            }
        }
        if (!found)    printf("\nInvalid command!");


        // ?? free up any malloc'd argv parameters
		int q = 0;
        for(q = 0; q < newArgc; q++){
            if(newArgv[q] != NULL)
                free(newArgv[q]);
        }
        free(newArgv);

        for (i=0; i<INBUF_SIZE; i++) inBuffer[i] = 0;
    }
    return 0;                        // terminate task
} // end P1_shellTask





// ***********************************************************************
// ***********************************************************************
// P1 Project
//
int P1_project1(int argc, char* argv[])
{
    SWAP                                        // do context switch

    return 0;
} // end P1_project1



// ***********************************************************************
// ***********************************************************************
// quit command
//
int P1_quit(int argc, char* argv[])
{
    int i;

    // free P1 commands
    for (i = 0; i < NUM_COMMANDS; i++)
    {
        free(commands[i]->command);
        free(commands[i]->shortcut);
        free(commands[i]->description);
    }
    free(commands);

    // powerdown OS345
    longjmp(reset_context, POWER_DOWN_QUIT);
    return 0;
} // end P1_quit



// **************************************************************************
// **************************************************************************
// lc3 command
//
int P1_lc3(int argc, char* argv[])
{
    strcpy (argv[0], "0");
    return lc3Task(argc, argv);
} // end P1_lc3



// **************************************************************************
// **************************************************************************
// lc3 command
//
int P1_add(int argc, char* argv[])
{
    char* myErrorMsg = "\nUsage: add <space-separated list of ints, floats, or hex>";
    int i;
    float add = 0;
    if(argc <= 1){
        printf("%s",myErrorMsg);
        return 0;
    }

    for(i = 1; i<argc; i++ ){
        float converted;
        if( strcmp(argv[i], "0")!=0 &&
            strcmp(argv[i], "0.0")!=0 &&
            strcmp(argv[i], "0x0000")!=0){
            converted = strtof(argv[i], NULL);
            if(converted == 0.0F){ //Check for invalid (unconvertable) variables.
                printf("%s",myErrorMsg);
                return 0;
            }
            add += converted;
        }
    }
    printf("\n%f", add);
    return 0;
} // end P1_lc3



// ***********************************************************************
// ***********************************************************************
// help command
//
int P1_help(int argc, char* argv[])
{
    int i;

    // list commands
    for (i = 0; i < NUM_COMMANDS; i++)
    {
        SWAP                                        // do context switch
        if (strstr(commands[i]->description, ":")) printf("\n");
        printf("\n%4s: %s", commands[i]->shortcut, commands[i]->description);
    }

    printf("\nType in a name for additional help: ");
    SEM_WAIT(inBufferReady);            // wait for input buffer semaphore

    for(i = 0; i < NUM_COMMANDS; i++){
        if(strstr(commands[i]->command, inBuffer)){
            printf("\n%s\n%s\n%s",
                   commands[i]->command,
                   commands[i]->shortcut,
                   commands[i]->description);
        }
    }

    return 0;
} // end P1_help





// ***********************************************************************
// ***********************************************************************
// help command
//
int P1_smile(int argc, char* argv[])
{
    printf("=)");
    return 0;
} // end P1_help




// ***********************************************************************
// ***********************************************************************
// help command
//
int P1_args(int argc, char* argv[])
{
    int i;
    for(i=1; i<argc; i++){
        printf("\n%d: %s", i, argv[i]);
    }
    if(argc == 1){
        printf("\nNo args.");
    }

    return 0;
} // end P1_help




// ***********************************************************************
// ***********************************************************************
// help command
//
int P1_time(int argc, char* argv[])
{
    time_t rawtime;
    struct tm * timeinfo;

    time( &rawtime );
    timeinfo = localtime( &rawtime );
    char* ttp = asctime(timeinfo);
    ttp[ strlen(ttp)-1 ] = 0; //remove newline character
    printf("\n%s", ttp);

    return 0;
} // end P1_time


int C_clearBuffHist(int argc, char* argv[])
{
    clearBuffHist();
    return 0;
}


// ***********************************************************************
// ***********************************************************************
// initialize shell commands
//
Command* newCommand(char* command, char* shortcut, int (*func)(int, char**), char* description)
{
    Command* cmd = (Command*)malloc(sizeof(Command));

    // get long command
    cmd->command = (char*)malloc(strlen(command) + 1);
    strcpy(cmd->command, command);

    // get shortcut command
    cmd->shortcut = (char*)malloc(strlen(shortcut) + 1);
    strcpy(cmd->shortcut, shortcut);

    // get function pointer
    cmd->func = func;

    // get description
    cmd->description = (char*)malloc(strlen(description) + 1);
    strcpy(cmd->description, description);

    return cmd;
} // end newCommand


Command** P1_init()
{
    int i  = 0;
    Command** commands = (Command**)malloc(sizeof(Command*) * NUM_COMMANDS);

    // system (3 commands)
    commands[i++] = newCommand("quit", "q", P1_quit, "Quit");
    commands[i++] = newCommand("kill", "kt", P2_killTask, "Kill task");
    commands[i++] = newCommand("reset", "rs", P2_reset, "Reset system");

    // P1: Shell (7 commands)
    commands[i++] = newCommand("project1", "p1", P1_project1, "P1: Shell");
    commands[i++] = newCommand("help", "he", P1_help, "OS345 Help");
    commands[i++] = newCommand("lc3", "lc3", P1_lc3, "Execute LC3 program");

    commands[i++] = newCommand("add", "+", P1_add, "Add numbers in command line");
    commands[i++] = newCommand("args", "al", P1_args, "Print arguments");
    commands[i++] = newCommand("time", "date", P1_time, "Print current date and time");
    commands[i++] = newCommand("smile", "smile", P1_smile, "Make a smiley face");
    commands[i++] = newCommand("clearhist", "cbh", C_clearBuffHist, "Clear the command recall history");

    // P2: Tasking (5 commands)
    commands[i++] = newCommand("project2", "p2", P2_project2, "P2: Tasking");
    commands[i++] = newCommand("semaphores", "sem", P2_listSems, "List semaphores");
    commands[i++] = newCommand("tasks", "lt", P2_listTasks, "List tasks");
    commands[i++] = newCommand("signal1", "s1", P2_signal1, "Signal sem1 semaphore");
    commands[i++] = newCommand("signal2", "s2", P2_signal2, "Signal sem2 semaphore");

    commands[i++] = newCommand("tenSec", "ten", tenSecTask, "Signal every ten seconds");

    // P3: Jurassic Park (2 commands)
    commands[i++] = newCommand("project3", "p3", P3_project3, "P3: Jurassic Park");
    commands[i++] = newCommand("deltaclock", "dc", P3_dc, "List deltaclock entries");
    commands[i++] = newCommand("testdeltaclock", "dct", dc_test, "Run a test on the delta clock");


    // P4: Virtual Memory (13 commands)
    commands[i++] = newCommand("project4", "p4", P4_project4, "P4: Virtual Memory");
    commands[i++] = newCommand("frametable", "dft", P4_dumpFrameTable, "Dump bit frame table");
    commands[i++] = newCommand("initmemory", "im", P4_initMemory, "Initialize virtual memory");
    commands[i++] = newCommand("touch", "vma", P4_vmaccess, "Access LC-3 memory location");
    commands[i++] = newCommand("stats", "vms", P4_virtualMemStats, "Output virtual memory stats");
    commands[i++] = newCommand("crawler", "cra", P4_crawler, "Execute crawler.hex");
    commands[i++] = newCommand("memtest", "mem", P4_memtest, "Execute memtest.hex");

    commands[i++] = newCommand("frame", "dfm", P4_dumpFrame, "Dump LC-3 memory frame");
    commands[i++] = newCommand("memory", "dm", P4_dumpLC3Mem, "Dump LC-3 memory");
    commands[i++] = newCommand("page", "dp", P4_dumpPageMemory, "Dump swap page");
    commands[i++] = newCommand("virtual", "dvm", P4_dumpVirtualMem, "Dump virtual memory page");
    commands[i++] = newCommand("root", "rpt", P4_rootPageTable, "Display root page table");
    commands[i++] = newCommand("user", "upt", P4_userPageTable, "Display user page table");

    // P5: Scheduling (1 commands)
    commands[i++] = newCommand("project5", "p5", P5_project5, "P5: Scheduling");
//    commands[i++] = newCommand("stress1", "t1", P5_stress1, "ATM stress test1");
//    commands[i++] = newCommand("stress2", "t2", P5_stress2, "ATM stress test2");

    // P6: FAT (22 commands)
    commands[i++] = newCommand("project6", "p6", P6_project6, "P6: FAT");
    commands[i++] = newCommand("change", "cd", P6_cd, "Change directory");
    commands[i++] = newCommand("copy", "cf", P6_copy, "Copy file");
    commands[i++] = newCommand("define", "df", P6_define, "Define file");
    commands[i++] = newCommand("delete", "dl", P6_del, "Delete file");
    commands[i++] = newCommand("directory", "dir", P6_dir, "List current directory");
    commands[i++] = newCommand("mount", "md", P6_mount, "Mount disk");
    commands[i++] = newCommand("mkdir", "mk", P6_mkdir, "Create directory");
    commands[i++] = newCommand("run", "run", P6_run, "Execute LC-3 program");
    commands[i++] = newCommand("space", "sp", P6_space, "Space on disk");
    commands[i++] = newCommand("type", "ty", P6_type, "Type file");
    commands[i++] = newCommand("unmount", "um", P6_unmount, "Unmount disk");

    commands[i++] = newCommand("fat", "ft", P6_dfat, "Display fat table");
    commands[i++] = newCommand("fileslots", "fs", P6_fileSlots, "Display current open slots");
    commands[i++] = newCommand("sector", "ds", P6_dumpSector, "Display disk sector");
    commands[i++] = newCommand("chkdsk", "ck", P6_chkdsk, "Check disk");
    commands[i++] = newCommand("final", "ft", P6_finalTest, "Execute file test");

    commands[i++] = newCommand("open", "op", P6_open, "Open file test");
    commands[i++] = newCommand("read", "rd", P6_read, "Read file test");
    commands[i++] = newCommand("write", "wr", P6_write, "Write file test");
    commands[i++] = newCommand("seek", "sk", P6_seek, "Seek file test");
    commands[i++] = newCommand("close", "cl", P6_close, "Close file test");



    assert(i == NUM_COMMANDS);

    return commands;

} // end P1_init
