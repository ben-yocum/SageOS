// os345config.h		01/08/2014
#ifndef __os345config_h__
#define __os345config_h__
// ***********************************************************************
//
#define STARTUP_MSG	"SageOS"

// ***********************************************************************
// Select development system environment here:
#define DOS	1						// DOS
#define GCC	0						// UNIX/Linux
#define PPC	0						// Power PC
#define MAC	0						// Mac
#define NET	0						// .NET

// ***********************************************************************
#if DOS == 1
// FOR LCC AND COMPATIBLE COMPILERS
#include <conio.h>
#define INIT_OS

//if(key was pressed)
//  wait till a key is pressed to do something
//else
//  return false
#define GET_CHAR		(kbhit()?getch():0)
#define SET_STACK(s)	 __asm__("movl _temp,%esp");
#define RESTORE_OS
#define LITTLE	1
#define CLEAR_SCREEN	system("cls");
#endif

#if GCC == 1
// FOR GCC AND COMPATIBLE COMPILERS
#include <fcntl.h>
#define INIT_OS		system("stty -echo -icanon");fcntl(1,F_SETFL,O_NONBLOCK);
#define GET_CHAR		getchar()
//#define SET_STACK __asm__ __volatile__("movl %0,%%esp"::"r"(temp):%esp);
#define SET_STACK(s)	asm("movl temp,%esp");
#define RESTORE_OS	system("stty icanon echo");	// enable canonical mode and echo
#define LITTLE	1
#define CLEAR_SCREEN	system("clear");
#endif

#if NET == 1
// FOR .NET AND COMPATIBLE COMPILERS
#include <conio.h>
#define INIT_OS
#define GET_CHAR		(kbhit()?getch():0)
#define SET_STACK(s) __asm mov ESP,s;
#define RESTORE_OS
#define LITTLE	1
#define CLEAR_SCREEN	system("cls");
#endif

// FOR POWER PC COMPATIBLE COMPILERS
#if PPC == 1
#include <fcntl.h>
#define INIT_OS		system("stty -echo -icanon");fcntl(1,F_SETFL,O_NONBLOCK);
#define GET_CHAR		getchar()
#define SET_STACK(s)	__asm("addis r2,0,ha16(_temp)");\
							__asm("lwz r1,lo16(_temp)(r2)");
#define RESTORE_OS	system("stty icanon echo");	// enable canonical mode and echo
#define LITTLE	0
#define CLEAR_SCREEN	system("cls");
#endif

// FOR 64-BIT MAC COMPATIBLE COMPILERS (OSX Lion 10.7)
#if MAC == 1
#include <fcntl.h>
#define INIT_OS		system("stty -echo -icanon");fcntl(1,F_SETFL,O_NONBLOCK);
#define GET_CHAR		getchar()
#define SET_STACK(s)	__asm("movq _temp(%rip),%rsp");
#define RESTORE_OS	system("stty icanon echo");	// enable canonical mode and echo
#define LITTLE	1
#define CLEAR_SCREEN	system("cls");
#endif

#define SWAP_BYTES(v) 1?v:((((v)>>8)&0x00ff))|((v)<<8)
#define SWAP_WORDS(v) LITTLE?v:((SWAP_BYTES(v)<<16))|(SWAP_BYTES((v)>>16))

#endif // __os345config_h__
