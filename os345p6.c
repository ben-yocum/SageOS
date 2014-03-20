// os345p6.c - FAT File Management System
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
#include <time.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345fat.h"

// ***********************************************************************
// ***********************************************************************
// project 6 variables

// RAM disk
extern unsigned char RAMDisk[SECTORS_PER_DISK * BYTES_PER_SECTOR];
extern unsigned char FAT1[];			// current fat table
extern unsigned char FAT2[];			// secondary fat table
extern FDEntry OFTable[];	          	// open files
extern char dirPath[128];				// directory path

extern TCB tcb[];						// task control block
extern int curTask;						// current task #
extern bool diskMounted;				// disk has been mounted

FMSERROR FMSErrors[NUM_ERRORS]   = {
                              {ERR50, ERR50_MSG},  // Invalid File Name
                              {ERR51, ERR51_MSG},  // Invalid File Type
                              {ERR52, ERR52_MSG},  // Invalid File Descriptor
                              {ERR53, ERR53_MSG},  // Invalid Sector Number
                              {ERR54, ERR54_MSG},  // Invalid FAT Chain
                              {ERR55, ERR55_MSG},  // Invalid Directory

                              {ERR60, ERR60_MSG},  // File Already Defined
                              {ERR61, ERR61_MSG},  // File Not Defined
                              {ERR62, ERR62_MSG},  // File Already Open
                              {ERR63, ERR63_MSG},  // File Not Open
                              {ERR64, ERR64_MSG},  // File Directory Full
                              {ERR65, ERR65_MSG},  // File Space Full
                              {ERR66, ERR66_MSG},  // End-Of-File
                              {ERR67, ERR67_MSG},  // End-Of-Directory
                              {ERR68, ERR68_MSG},  // Directory Not Found
                              {ERR69, ERR69_MSG},  // Can Not Delete

                              {ERR70, ERR70_MSG},  // Too Many Files Open
                              {ERR71, ERR71_MSG},  // Not Enough Contiguous Space
                              {ERR72, ERR72_MSG},  // Disk Not Mounted

                              {ERR80, ERR80_MSG},  // File Seek Error
                              {ERR81, ERR81_MSG},  // File Locked
                              {ERR82, ERR82_MSG},  // File Delete Protected
                              {ERR83, ERR83_MSG},  // File Write Protected
                              {ERR84, ERR84_MSG},  // Read Only File
                              {ERR85, ERR85_MSG}   // Illegal Access
                   };

int sectorReads;
int sectorWrites;

// ***********************************************************************
// project 6 functions and tasks


// ***********************************************************************
// ***********************************************************************


// ***********************************************************************
// ***********************************************************************
// p6 - runs finalTest
//
int P6_project6(int argc, char* argv[])
{
	char* newArgv[] = { "finalTest", "All" };

	if (sizeof(DirEntry) != 32)
	{
		printf("\n***Incompatible long data type in DirEntry***");
		return 0;
	}

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}

	P6_chkdsk(0, (char**)0);			// check RAM disk
	P6_finalTest(2, newArgv);			// all final tests
	return 0;
} // end P6_project6



// ***********************************************************************
// ***********************************************************************
// cd <fileName>
int P6_cd(int argc, char* argv[])			// change directory
{
	int error;

	if (argc < 2)
	{
		printf("\n  CD <fileName>");
		return 0;
	}

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (!(error = fmsChangeDir(argv[1]))) return 0;
	if (error == ERR67) error = ERR68;
	fmsError(error);
	return 0;
} // end P6_cd



// ***********************************************************************
// ***********************************************************************
// dir <mask>
int P6_dir(int argc, char* argv[])		// list directory
{
	int index = 0;
	DirEntry dirEntry;
	char mask[20];
	int error = 0;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2) strcpy(mask, "*.*");
		else strcpy(mask, argv[1]);

	//dumpRAMDisk("Root Directory", 19*512, 19*512+256);
	printf("\nName:ext                time      date    cluster  size");
	while (1)
	{
		error = fmsGetNextDirEntry(&index, mask, &dirEntry, CDIR);
		if (error)
		{
			if (error != ERR67) fmsError(error);
			break;
		}
		printDirectoryEntry(&dirEntry);
		SWAP;
	}
	//dumpRAMDisk("Root Directory", 19*512, 20*512);
	return 0;
} // end P6_dir



// ***********************************************************************
// ***********************************************************************
// fat {<TAB#>{,<LOC#>{,<END>}}}
//
//		1	fat					output fat 1 table
//		2	fat <1 to 2>		output fat <#> table
//		2	fat <# gt 2>		output fat 1 table entry <#>
//		3	fat <#>,<s>			output fat table <#> starting with entry <s>
//		4	fat <#>,<s>,<e>	output fat table <#> from <s> to <e>
//
void outFatEntry(int index)
{
	int fatvalue;

	printf("\n  FAT1[%d] = ", index);
	fatvalue = getFatEntry(index, FAT1);
	// Special formatting cases...
	if (index < 2) printf("RSRV");			// A reserved cluster
	else if (fatvalue == FAT_EOC) printf("EOC"); // The EOC marker
	else if (fatvalue == FAT_BAD) printf("BAD"); // The BAD cluster marker
	else printf("%d", fatvalue);
	return;
}

int P6_dfat(int argc, char* argv[])		// list FAT table
{
	int index;
	int start = 0;
	int end = 64;

	//		1	fat					output fat 1 table
	//		2	fat <1 to 2>		output fat <#> table
	//		2	fat <# gt 2>		output fat 1 table entry <#>
	//		3	fat <#>,<s>			output fat table <#> starting with entry <s>
	//		4	fat <#>,<s>,<e>	output fat table <#> from <s> to <e>

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	switch (argc)
	{
		case 1:						//	1	fat					output fat 1 table
		{
			printFatEntries("FAT1", start, end);
		}

		case 2:						//	2	fat <1 to 2>		output fat <#> table
		{
			char buf[32];
			index = INTEGER(argv[1]);
			if ((index == 1) || (index ==2))
			{
				sprintf(buf, "Disk Image: FAT %d", index);
				dumpRAMDisk(buf, (1+index*8)*512, (1+index*8)*512+64);
			}
			else
			{
				outFatEntry(index);
			}
			break;
		}

		case 3:						//	3	fat <#>,<s>			output fat table <#> starting with entry <s>
		{
			char* FATtb = (INTEGER(argv[1]) == 2) ? FAT2 : FAT1;
			start = INTEGER(argv[2]);
			end = start + 1;

			for (index=start; start<end; index++) outFatEntry(index);
			break;
		}

		case 4:						//	4	fat <#>,<s>,<e>	output fat table <#> from <s> to <e>
		{
			char* FATtb = (INTEGER(argv[1]) == 2) ? FAT2 : FAT1;
			start = INTEGER(argv[2]);
			end = INTEGER(argv[3]);

			for (index=start; start<end; index++) outFatEntry(index);
			break;
		}

	}
	return 0;
} // end P6_dfat



// ***********************************************************************
// ***********************************************************************
// mount <disk name>
//
int P6_mount(int argc, char* argv[])		// mount RAM disk
{
	int error;
	BSStruct bootSector;
	char temp[128] = {""};

	assert("64-bit" && (sizeof(DirEntry) == 32));

	if (argc < 2) strcat(temp, "c:/lcc/projects/disk4");
		else strcat(temp, argv[1]);
	printf("\nMount Disk \"%s\"", temp);

	error = fmsMount(temp, &RAMDisk);
	if (error)
	{
		printf("\nDisk Mount Error %d", error);
		return 0;
	}
	//dumpRAMDisk("Boot Sector", 0, 512-450);
	//printf("\nBoot size = %d", sizeof(BSStruct));
	fmsReadSector(&bootSector, 0);
	strncpy(temp, (char*)&bootSector.BS_OEMName, 8);
	temp[8] = 0;
	printf("\n                System: %s", temp);
	printf("\n          Bytes/Sector: %d", bootSector.BPB_BytsPerSec);
	printf("\n       Sectors/Cluster: %d", bootSector.BPB_SecPerClus);
	printf("\n      Reserved sectors: %d", bootSector.BPB_RsvdSecCnt);
	printf("\n            FAT tables: %d", bootSector.BPB_NumFATs);
	printf("\n  Max root dir entries: %d", bootSector.BPB_RootEntCnt);
	printf("\n        FAT-12 sectors: %d", bootSector.BPB_TotSec16);
	printf("\n           FAT sectors: %d", bootSector.BPB_FATSz16);		// FAT sectors (should be 9)
	printf("\n         Sectors/track: %d", bootSector.BPB_SecPerTrk);		// Sectors per cylindrical track
	printf("\n          Heads/volume: %d", bootSector.BPB_NumHeads);		// Heads per volume (2 for 1.4Mb 3.5" floppy)
	printf("\n        FAT-32 sectors: %ld", bootSector.BPB_HiddSec);		// Hidden sectors (0 for non-partitioned media)
	return 0;
} // end P6_mount



// ***********************************************************************
// ***********************************************************************
// run <fileName>
int P6_run(int argc, char* argv[])		// run lc3 program from RAM disk
{
	char fileName[128];
	char* myArgv[] = {"1", ""};

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\nRUN <fileName>");
		return 0;
	}
	strcpy(fileName, argv[1]);
	if (!strstr(fileName, ".hex")) strcat(fileName, ".hex");
	myArgv[1] = (char*)&fileName;
	createTask( myArgv[0],				// task name
					lc3Task,					// task
					MED_PRIORITY,			// task priority
					2,							// task argc
					myArgv);					// task arguments
	return 0;
} // end P6_run



// ***********************************************************************
// ***********************************************************************
// space
int P6_space(int argc, char* argv[])
{
	DiskSize dskSize;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	fmsDiskStats(&dskSize);
	printf("\nFree: %d", dskSize.free);
	printf("\nUsed: %d", dskSize.used);
	printf("\n Bad: %d", dskSize.bad);
	printf("\nSize: %d", dskSize.size);
	return 0;
} // end P6_space


// ***********************************************************************
// ***********************************************************************
// type <fileName>
int P6_type(int argc, char* argv[])		// display file
{
	int error, nBytes, FDs;
	char buffer[4];

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\n  type <file>");
		return 0;
	}
	printf("\nType File \"%s\"\n", argv[1]);	// ?? debug
	// open source file
	if ((FDs = fmsOpenFile(argv[1], 0)) < 0)
	{
		fmsError(FDs);
		return 0;
	}
	printf("\n FDs = %d\n", FDs);					// ?? debug
	while ((nBytes = fmsReadFile(FDs, buffer, 1)) == 1)
	{
		putchar(buffer[0]);
		SWAP;
	}
	if (nBytes != ERR66) fmsError(nBytes);
	if (error = fmsCloseFile(FDs)) fmsError(error);
	return 0;
} // end P6_type



// ***********************************************************************
// ***********************************************************************
//	copy <file1>,<file2>
int P6_copy(int argc, char* argv[])		 	// copy file
{
	int error, nBytes, FDs, FDd;
	char buffer[BYTES_PER_SECTOR];

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 3)
	{
		printf("\n  copy <file1> <file2>");
		return 0;
	}
	// open source file
	if ((FDs = fmsOpenFile(argv[1], 0)) < 0)
	{	fmsError(FDs);
		return 0;
	}
	// open destination file
	if ((FDd = fmsOpenFile(argv[2], 1)) < 0)
	{	fmsCloseFile(FDs);
		fmsError(FDd);
		return 0;
	}
	//printf("\n FDs = %d\n FDd = %d\n", FDs, FDd);

	nBytes = 1;
	while (nBytes > 0)
	{
		error = 0;
		nBytes = fmsReadFile(FDs, buffer, BYTES_PER_SECTOR);
		SWAP;
		if (nBytes < 0) break;
		error = fmsWriteFile(FDd, buffer, nBytes);
		if (error < 0) break;
		//for (error=0; error<nBytes; error++) putchar(buffer[error]);
	}
	if (nBytes != ERR66) fmsError(nBytes);
	if (error) fmsError(error);

	error = fmsCloseFile(FDs);
	if (error) fmsError(error);

	error = fmsCloseFile(FDd);
	if (error) fmsError(error);
	return 0;
} // end P6_copy



// ***********************************************************************
// ***********************************************************************
// df <fileName>
int P6_define(int argc, char* argv[])			// define file
{
	int error;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\n  DF <fileName>");
		return 0;
	}
	if ((error = fmsDefineFile(argv[1], ARCHIVE)) < 0)
	{
		fmsError(error);
	}
	return 0;
} // end P6_define



// ***********************************************************************
// ***********************************************************************
// del <fileName>
int P6_del(int argc, char* argv[])				// delete file
{
	int error;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\n  DL <fileName>");
		return 0;
	}
	if ((error = fmsDeleteFile(argv[1])) < 0)
	{
		fmsError(error);
	}
	return 0;
} // end P6_del



// ***********************************************************************
// ***********************************************************************
// mkdir <fileName>
int P6_mkdir(int argc, char* argv[])				// create directory file
{
	int error;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\n  MK <fileName>");
		return 0;
	}
	if ((error = fmsDefineFile(argv[1], DIRECTORY)) < 0)
	{
		fmsError(error);
	}
	return 0;
} // P6_mkdir



// ***********************************************************************
// ***********************************************************************
// unmount <diskname>
int P6_unmount(int argc, char* argv[])			// save ram disk
{
	int error;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\n  UM <fileName>");
		return 0;
	}
	if ((error = fmsUnMount(argv[1], RAMDisk)) < 0)
	{
		fmsError(error);
	}
	return 0;
} // end P6_unmount



// ***********************************************************************
// ***********************************************************************
// ds <sector>
int P6_dumpSector(int argc, char* argv[])	// dump RAM disk sector
{
	char temp[32];
	int sector;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	printf("\nValidate arguments...");	// ?? validate arguments
	sector = INTEGER(argv[1]);

	sprintf(temp, "Sector %d:", sector);
	dumpRAMDisk(temp, sector*512, sector*512 + 512);
	return 0;
} // end P6_dumpSector



// ***********************************************************************
// ***********************************************************************
// fs
int P6_fileSlots(int argc, char* argv[])	// list open file slots
{
	int i, fd;
	FDEntry* fdEntry;

	printf("\nSlot  Name    Ext  Atr  Size  Strt  Curr  cDir  cPID  Mode  Flag  Indx");
	for (fd = 0; fd < NFILES; fd++)
	{
		fdEntry = &OFTable[fd];
		if (fdEntry->name[0] == 0) continue;      // open slot
		printf("\n %2d   ", fd);
		for (i=0; i<12; i++) printf("%c", fdEntry->name[i]);
		printf("  %02x%6ld%6d%6d%6d%6d%6d%6d%6ld",
			fdEntry->attributes,
			fdEntry->fileSize,
			fdEntry->startCluster,
			fdEntry->currentCluster,
			fdEntry->directoryCluster,
			fdEntry->pid,
			fdEntry->mode,
			fdEntry->flags,
			fdEntry->fileIndex);
	}
	return 0;
} // end P6_fileSlots



// ***********************************************************************
// ***********************************************************************
// *	Support functions
// ***********************************************************************
// Print directory entry
void printDirectoryEntry(DirEntry* dirent)
{
	int i = 7;
	char p_string[64] = "              ------  00:00:00 03/01/2004";
   FATDate date;											// The Date bit field structure
   FATTime time;											// The Time bit field structure

   strncpy(p_string, (char*)&(dirent->name), 8);	// Copies 8 bytes from the name
	while (p_string[i] == ' ') i--;
	p_string[i+1] = '.';									// Add extension
	strncpy(&p_string[i+2], (char*)&(dirent->extension), 3);
	while (p_string[i+4] == ' ') i--;
	if (p_string[i+4] == '.') p_string[i+4] = ' ';

   // Generate the attributes
   if(dirent->attributes & 0x01) p_string[14] = 'R';
   if(dirent->attributes & 0x02) p_string[15] = 'H';
   if(dirent->attributes & 0x04) p_string[16] = 'S';
   if(dirent->attributes & 0x08) p_string[17] = 'V';
   if(dirent->attributes & 0x10) p_string[18] = 'D';
   if(dirent->attributes & 0x20) p_string[19] = 'A';

   // Extract the time and format it into the string...
   memcpy(&time, &(dirent->time), sizeof(FATTime));
   sprintf(&p_string[22], "%02d:%02d:%02d  ", time.hour, time.min, time.sec*2);

   // Extract the date and format it as well, inserting it into the string...
   memcpy(&date, &(dirent->date), sizeof(FATDate));
   sprintf(&p_string[31], "%02d/%02d/%04d %5d %5lu",
  				date.month+1, date.day, date.year+1980,
			   dirent->startCluster, dirent->fileSize);

   // p_string is now ready!
	printf("\n%s", p_string);
	return;
} // end PrintDirectoryEntry


// ***********************************************************************
// print FAT table
void printFatEntries(unsigned char* FAT, int start, int end)
{
	char tbuf[16];
	char fbuf[100];
	unsigned short fatvalue;
	int i, nn, counter = 0;;

	nn = (512 * 9) / 1.5;						// The number of fat entries in the FAT table
	if (end < nn) nn = end;

	sprintf(fbuf, "\n    %6d:", start);
	for (i=start; i<nn; i++)
	{
		if (!(counter%10) && counter)
		{
			printf("%s", fbuf);
			sprintf(fbuf, "\n    %6d:", i);
		}
      	fatvalue = getFatEntry(i, FAT);
      	// Special formatting cases...
      	if (i < 2) sprintf(tbuf, " RSRV"); 							// A reserved cluster
      	else if (fatvalue == FAT_EOC) sprintf(tbuf, "  EOC"); // The EOC marker
      	else if (fatvalue == FAT_BAD) sprintf(tbuf, "  BAD"); // The BAD cluster marker
      	else sprintf(tbuf, "%5d", fatvalue);
		strcat(fbuf, tbuf);
		counter++;
	}
	if (counter%10) printf("%s", fbuf);
	return;
} // End PrintFatTable



// ***********************************************************************
// dm <sa> <ea> - dump dumpRAMDisk memory
void dumpRAMDisk(char *s, int sa, int ea)
{
   int i, ma;
   unsigned char j;

   printf("\n%s", s);
   for (ma = sa; ma < ea; ma+=16)
	{
		printf("\n0x%08x:", ma);
		for (i=0; i<16; i++)
		{
			printf(" %02x", (unsigned char)RAMDisk[ma+i]);
		}
      printf("  ");
      for (i=0; i<16; i++)
      {
         j = RAMDisk[ma+i];
         if ((j<20) || (j>127)) j='.';
         printf("%c", j);
      }
	}
   return;
} // end dumpRAMDisk


// ***********************************************************************
// ***********************************************************************
//	*																							 *
//	*	    PPPPPP     AA     SSSSS   SSSSS    OOOO   FFFFFF FFFFFF 		 *
//	*	    PP   PP   AAAA   SS   SS SS   SS  OO  OO  FF     FF  	  		 *
//	*	    PP   PP  AA  AA  SSSS    SSSS    OO    OO FF     FF    			 *
//	*	    PPPPPP  AAAAAAAA    SSS     SSS  OO    OO FFFF   FFFF  			 *
//	*	    PP      AA    AA SS   SS SS   SS  OO  OO  FF     FF    			 *
//	*	    PP      AA    AA  SSSSS   SSSSS    OOOO   FF     FF    			 *
//	*																							 *
// ***********************************************************************
// ***********************************************************************

// ***********************************************************************
// ***********************************************************************
// passoff functions and tasks
#define ckFAT1	FAT1
#define ckFAT2	FAT2

int isValidDirEntry(DirEntry* dirEntry);
void getFileName(char* fileName, DirEntry* dirEntry);
void checkDirectory(char* dirName, unsigned char fat[], int dir);

// ***********************************************************************
// ***********************************************************************
//	check disk for errors
//
int P6_chkdsk(int argc, char* argv[])		// check RAM disk
{
	int i, j;
	unsigned char fat[CLUSTERS_PER_DISK];

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	printf("\nChecking disk...");
	// set fat if cluster allocated
	for (i=2; i<CLUSTERS_PER_DISK; i++)
	{
		fat[i] = getFatEntry(i,ckFAT1) ? 1 : 0;
	}
	// process all directory entries
	checkDirectory("Root", fat, 0);
	// check to see if there are any zombie clusters
	j = 0;
	for (i=2; i<CLUSTERS_PER_DISK; i++)
	{
		if (fat[i])
		{
			SWAP;
			if (j)
			{
				if (++j == i) continue;
				printf("-%d", --j);
			}
			else
			{
				printf("\n  Orphaned Clusters: ");
			}
			printf(",  %d", i);
			j = i;
		}
	}
	if (j) printf("-%d", j);
	// reconcile fat tables
	for (i=0; i<CLUSTERS_PER_DISK; i++)
	{
		if (getFatEntry(i,ckFAT1) != getFatEntry(i,ckFAT2))
		{
			printf("\n  FAT1[%d] != FAT2[%d] (%d/%d)",
		  		i, i, getFatEntry(i,ckFAT1), getFatEntry(i,ckFAT2));
		}
	}
	return 0;
} // end P6_chkdsk


// ***********************************************************************
// chkdsk utilities
// ***********************************************************************
//
//		getFileName
//
//		IN:	*fileName	pointer to buffer of at least 16 bytes
//				*dirEntry	32-byte directory entry
//
void getFileName(char* fileName, DirEntry* dirEntry)
{
	int i = 7;

	memset(fileName, 0, 16);
   strncpy(fileName, (char*)&(dirEntry->name), 8);	// Copies 8 bytes from the name
	i = strlen(fileName) - 1;
	while (fileName[i] == ' ') i--;
	fileName[++i] = '.';									// Add extension
	strncpy(&fileName[++i], (char*)&(dirEntry->extension), 3);
	i = strlen(fileName) - 1;
	while (fileName[i] == ' ') i--;
	if (fileName[i] == '.') fileName[i] = 0;
	return;
} // end getFileName



// ***********************************************************************
//	check for valid directory entry
int isValidDirEntry(DirEntry* dirEntry)
{
	char name[16];
	char fileName[16];
	int error = 0;

	getFileName(fileName, dirEntry);
	strncpy(name, (char*)dirEntry->name, 11);
	name[11] = 0;
	if (strlen(name) != 11)
	{
		printf("\n  Illegal NULL in file name \"%s\"", fileName);
		error = 1;
	}
	// check for invalid characters
	if (strpbrk(name, ".\"\\/:*<>|?"))
	{
		printf("\n  Illegal character in file name \"%s\"", fileName);
		error = 1;
	}
	return error;
} // end isValidDirEntry


// ***********************************************************************
//	check directory (recursively)
void checkDirectory(char* dirName, unsigned char fat[], int dir)
{
	int error, index = 0;
	DirEntry dirEntry;
	char fileName[32];
	unsigned long maxSize;

	//	check for dot/dotdot
	if (dir)
	{
		index = 0;
		if (fmsGetNextDirEntry(&index, ".", &dirEntry, dir) < 0)
			printf("\n  \".\" missing from \"%s\" directory", dirName);
		index = 0;
		if (fmsGetNextDirEntry(&index, "..", &dirEntry, dir) < 0)
			printf("\n  \"..\" missing from \"%s\" directory", dirName);
	}
	index = 0;
	while (1)
	{
		if (fmsGetNextDirEntry(&index, "*.*", &dirEntry, dir)) break;
		// process file name
		getFileName(fileName, &dirEntry);
		if (dirEntry.attributes & (VOLUME || DIRECTORY))
			printf("\n  Attribute 0x%02x is illegal in file \"%s\"", dirEntry.attributes, fileName);
		if (dirEntry.name[0] == '.') continue;
		else
		{	// follow clusters and clear fat
			unsigned char chain[CLUSTERS_PER_DISK];
			unsigned int i = dirEntry.startCluster;
			// check for valid file name
			isValidDirEntry(&dirEntry);
			memset(chain, 0, CLUSTERS_PER_DISK);
			error = 0;
			maxSize = 0;
			while (i)
			{
				maxSize += BYTES_PER_SECTOR;
				// check for illegal cluster
				if ((i < 2) || (i > CLUSTERS_PER_DISK))
				{
					printf("\n  Illegal reference to cluster %d in file \"%s\"", i, fileName);
					error = 1;
					break;
				}
				// check for loop
				if (chain[i]++)
				{
					printf("\n  Loop detected at cluster %d in file \"%s\"", i, fileName);
					error = 2;
					break;
				}
				// check for cross chains
				if (!fat[i])
				{
					printf("\n  Cross chain detected at cluster %d in file \"%s\"", i, fileName);
					error = 3;
					break;
				}
				// mark cluster as used, get next cluster
				fat[i] = 0;
				i = getFatEntry(i,ckFAT1);
	         if (i == FAT_EOC) break;	   // EOD
	         if (i == FAT_BAD) break;
			}
		}
		// if no loop and directory, then recurse
		if ((error != 2) && (dirEntry.attributes & DIRECTORY))
		{
			// file size of directory should be 0
			if (dirEntry.fileSize)
				printf("\n  Non-zero file size of %lu in directory \"%s\"", dirEntry.fileSize, fileName);
			checkDirectory(fileName, fat, dirEntry.startCluster);
		}
		else
		{	// check file size
			if (dirEntry.fileSize > maxSize)
				printf("\n  File size of %lu in file \"%s\" exceeds %lu", dirEntry.fileSize, fileName, maxSize);
		}
	}
	return;
} // end checkDirectory


// ***********************************************************************
// final pass-off routine
#define FERROR(s1,s2,e) { printf(s1,s2); fmsError(e); return e; }
#define numWords	16
#define numFiles	64
#define numDirs	128
#define numTests	6

#define try(s1) if((error=s1)<0){printf("\nFailed \"%s\"",#s1);fmsError(error);return error;}

int fmsTests(int test, int debug);
extern int P6_fileSlots(int, char**);	// list open file slots
extern int P6_dir(int, char**);			// list current directory

int P6_finalTest(int argc, char* argv[])
{
	int i, flags = 0;
	int finalDebug = 0;

	if (!diskMounted)
	{
		fmsError(ERR72);
		return 0;
	}
	if (argc < 2)
	{
		printf("\n  final 1   Define 128 directories, 64 files");
		printf("\n  final 2   Open 32 files, write, rewind, print, close");
		printf("\n  final 3   Open and append words");
		printf("\n  final 4   Random access on sector boundaries");
		printf("\n  final 5   Creating and deleting directories");
		printf("\n  final 6   Delete all test 1 files/directories");
		printf("\n  final all Run all tests");
		return 0;
	}
	if (toupper(argv[1][0]) == 'A') flags = 0x3f;
	else
	{
		flags = 0;
		for (i=1; i<argc; i++)
		{
			flags |= 1 << (INTEGER(argv[i]) - 1);
			printf("\n  Run test #%d", INTEGER(argv[i]));
		}
		printf("\n  flags = %02x", flags);
	}

	sectorReads = 0;
	sectorWrites = 0;
	for (i=1; i<=numTests; i++)
	{
		if (flags & (1 << (i-1)))
		{
			if (fmsTests(i, finalDebug))
			{
				printf("\nFAILED TEST %d!", i);
				return 0;
			}
		}
	}
	// list system
	P6_fileSlots(0, 0);		// list open file slots
	P6_dir(0, 0);				// list directory

	printf("\nSector reads = %d", sectorReads);
	printf("\nSector writes = %d", sectorWrites);
	printf("\nCONGRATULATIONS!  YOU PASS!");
	return 0;
} // end P6_finalTest


// ***********************************************************************
int fmsTests(int test, bool debug)
{
	char buf[32], buf1[32], buf2[32];
	char rBuf[128];
	char result[] = "Now is the time for all good men to come to the aid of their country.";
	int i, error;
	int tFID[numFiles];
	char* text[numWords] = {"Now ", "is ", "the ", "time ", "for ",
									"all ", "good ", "men ", "to ", "come ",
									"to ", "the ", "aid ", "of ", "their ",
									"country." };
	switch (test)
	{
		case 1:					// file creation
		{
			printf("\nRunning Test 1...");
			// create numDirs directories in current directory
			printf("\n  Define %d directories...", numDirs);
			for (i=0; i<numDirs; i++)
			{	sprintf(buf, "dfile%d", i);
				if (debug) printf("\n   fmsDefineFile(\"%s\")", buf);
				try(fmsDefineFile(buf, DIRECTORY));
			}

			// create numFiles files in current directory
			printf("\n  Define %d files...", numFiles);
			for (i=0; i<numFiles; i++)
			{	sprintf(buf, "file%d.txt", i);
				if (debug) printf("\n   fmsDefineFile(\"%s\")", buf);
				try(fmsDefineFile(buf, ARCHIVE));
			}
			break;
		}

		case 2:					// open files
		{
			printf("\nRunning Test 2...");
			// try to open numFiles files for read/write access
			// note: the number of open files is limited to the number of file slots
			printf("\n  Open %d files...", numFiles);
			for (i=0; i<numFiles; i++)
			{
				sprintf(buf, "file%d.txt", i);
				if ((tFID[i] = fmsOpenFile(buf, OPEN_RDWR)) < 0)
				{	if (i == NFILES) break;
					FERROR("\nFailed fmsOpenFile(\"%s\") with R/W",buf,tFID[i]);
				}
			}
			// write a word to each file, rewind the files (seek to 0), read and print word
			for (i=0; i<numWords; i++)
			{
				if (debug) printf("\n   fmsWriteFile(tFID[%d], \"%s\", %d)", i, text[i], strlen(text[i]));
				try(fmsWriteFile(tFID[i],text[i],strlen(text[i])));
			}
			// seek to beginning of files
			for (i=0; i<numWords; i++)
			{
				try(fmsSeekFile(tFID[i],0));
				if (debug) printf("\n   fmsSeekFile(tFID[%d], 0) = %d", i, error);
			}
			// read from numWords files
			rBuf[0] = 0;
			for (i=0; i<numWords; i++)
			{
				try(fmsReadFile(tFID[i], buf, strlen(text[i])));
				strncat(rBuf, buf, strlen(text[i]));
			}
			printf("\n  %s", rBuf);

			// close files
			for (i=0; i<NFILES; i++)
			{
				if (debug) printf("\n  fmsCloseFile(%d)", tFID[i]);
				try(fmsCloseFile(tFID[i]));
			}
			return strcmp(rBuf, result);
		}

		case 3:
		{
			// close/open word file and append to test2 file
			// then print test2 file
			int test2 = numFiles < NFILES ? numFiles-1 : NFILES-1;
			printf("\nRunning Test 3...");
			sprintf(buf2, "file%d.txt", test2);
			for (i=0; i<numWords; i++)
			{
				sprintf(buf, "file%d.txt", i);
				// open word file
				if (debug) printf("\n  fmsOpenFile(%s, OPEN_READ)", buf);
				try(tFID[i] = fmsOpenFile(buf, OPEN_READ));
				// open test2 file for appending
				if (debug) printf("\n  fmsOpenFile(%s, OPEN_APPEND)", buf2);
				try(tFID[test2] = fmsOpenFile(buf2, OPEN_APPEND));
				// read word from word file
				try(fmsReadFile(tFID[i], buf, strlen(text[i])));
				// write to buffer
				try(fmsWriteFile(tFID[test2], buf, strlen(text[i])));
				// close word file
				if (debug) printf("\n  fmsCloseFile(%d)", tFID[i]);
				try(fmsCloseFile(tFID[i]));
				// close test2 file
				if (debug) printf("\n  fmsCloseFile(%d)", test2);
				try(fmsCloseFile(tFID[test2]));
			}
			// read and print test2 file
			try(tFID[test2] = fmsOpenFile(buf2, OPEN_READ));
			rBuf[0] = 0;
			while ((error = fmsReadFile(tFID[test2], buf, 1)) > 0) strncat(rBuf, buf, 1);
			if ((error < 0) & (error != ERR66))
				FERROR("\nFailed fmsReadFile(%d)",tFID[test2],error);
			printf("\n  %s", rBuf);

			if (debug) printf("\n  fmsCloseFile(%d)", test2);
			try(fmsCloseFile(tFID[test2]));
			return strcmp(rBuf, result);
		}

		case 4:
		{
			int test3 = numFiles < NFILES ? numFiles-2 : NFILES-2;
			int t3FID;
			int fileSize = 10*512;
			int index[numWords] =  {510, 20, 5120, 1024, 0, 4095, 4000, 5000,
											100, 5090, 3000, 2000, 1535, 3100, 1900, 4500 };
			// write X's to test3 file
			// seek and write test words throughout test3 file
			// seek/read/print test words from test3 file
			printf("\nRunning Test 4...");
			sprintf(buf2, "file%d.txt", test3);

			if (debug) printf("\n  fmsOpenFile(\"%s\", %d)", buf2, OPEN_RDWR);
			try(t3FID = fmsOpenFile(buf2, OPEN_RDWR));

			// write file with X's
			if (debug) printf("\n  Write %d X's to \"%s\"", fileSize, buf2);
			for (i=0; i<fileSize; i++)
			{
				try(fmsWriteFile(t3FID, "X", 1));
			}

			// seek to write text
			for (i=0; i<numWords; i++)
			{
				if (debug) printf("\n  Write \"%s\" to position %d", text[i], index[i]);
				try(fmsSeekFile(t3FID, index[i]));
				try(fmsWriteFile(t3FID, text[i], strlen(text[i])));
			}

			// seek to read file
			rBuf[0] = 0;
			for (i=0; i<numWords; i++)
			{	//memset(buf, 0, sizeof(buf));
				try(fmsSeekFile(t3FID, index[i]));
				if ((error = fmsReadFile(t3FID, buf, strlen(text[i]))) > 0)
					strncat(rBuf, buf, strlen(text[i]));
				if (error <= 0) FERROR("\nFailed fmsReadFile(%d)",t3FID,error);
			}
			printf("\n  %s", rBuf);

			if (debug) printf("\n  fmsCloseFile(%d)", t3FID);
			try(fmsCloseFile(t3FID));
			return strcmp(rBuf, result);
		}

		case 5:
		{
			printf("\nRunning Test 5...");
			// define test directory
			strcpy(buf, "TESTDIR");
			printf("\n  fmsMakeDirectory(\"%s\")", buf);
			try(fmsDefineFile(buf, DIRECTORY));
			// change directories
			printf("\n  fmsChangeDir(\"%s\")", buf);
			try(fmsChangeDir(buf));
			// create numFiles files in current directory
			printf("\n  Define %d files...", numFiles);
			for (i=0; i<numFiles; i++)
			{	sprintf(buf1, "file%d.txt", i);
				if (debug) printf("\n  fmsDefineFile(\"%s\")", buf1);
				try(fmsDefineFile(buf1, ARCHIVE));
			}
			// go up one directory
			strcpy(buf2, "..");
			printf("\n  fmsChangeDir(\"%s\")", buf2);
			try(fmsChangeDir(buf2));
			// try to delete directory
			printf("\n  fmsDeleteFile(\"%s\")", buf);
			if ((error = fmsDeleteFile(buf)) != ERR69)
				FERROR("\nFailed fmsDeleteFile(\"%s\")",buf,error);
			printf(" Can Not Delete... Good!");
			// go back into directory
			printf("\n  fmsChangeDir(\"%s\")", buf);
			try(fmsChangeDir(buf));
			// delete all the files
			printf("\n  Delete %d files...", numFiles);
			for (i=0; i<numFiles; i++)
			{	sprintf(buf1, "file%d.txt", i);
				if (debug) printf("\n  fmsDeleteFile(\"%s\")", buf1);
				try(fmsDeleteFile(buf1));
			}
			// go up one directory again
			strcpy(buf2, "..");
			printf("\n  fmsChangeDir(\"%s\")", buf2);
			try(fmsChangeDir(buf2));
			// try to delete directory again
			printf("\n  fmsDeleteFile(\"%s\")", buf);
			try(fmsDeleteFile(buf));
			break;
		}

		case 6:
		{
			// delete numFiles files
			printf("\nRunning Test 6...");
			printf("\n  Delete %d files...", numFiles);
			for (i=0; i<numFiles; i++)
			{	sprintf(buf, "file%d.txt", i);
				if (debug) printf("\n  fmsDeleteFile(\"%s\")", buf);
				try(fmsDeleteFile(buf));
			}

			// delete numDirs directories
			printf("\n  Delete %d directories...", numDirs);
			for (i=0; i<numDirs; i++)
			{	sprintf(buf, "dfile%d", i);
				if (debug) printf("\n  fmsDeleteFile(\"%s\")", buf);
				try(fmsDeleteFile(buf));
			}
			break;
		}

		default:
			printf("\nInvalid test!");
	}
	return 0;
} // end finalTest



// ***********************************************************************
// ***********************************************************************
//	Unit test routines for open, read, write, seek, and close FMS functions
//
//	1. Add the following prototypes to os345.h:
//
//		int P6_open(int, char**);
//		int P6_read(int, char**);
//		int P6_write(int, char**);
//		int P6_seek(int, char**);
//		int P6_close(int, char**);
//
//	2. Add the following shell commands to os345p1.c:
//
//		{	"open",			"op",		P6_open,					"Open file test"	},
//		{	"read",			"rd",		P6_read,					"Read file test"	},
//		{	"write",			"wr",		P6_write,				"Write file test"	},
//		{	"seek",			"sk",		P6_seek,					"Seek file test"	},
//		{	"close",			"cl",		P6_close,				"Close file test"	},
//
//	3. Mount disk4:
//
//		CS345 F2008
//		0>>mount
//		Mount Disk "c:/lcc/projects/disk4"
//		                System: IBM  3.3
//		          Bytes/Sector: 512
//		       Sectors/Cluster: 1
//		      Reserved sectors: 1
//		            FAT tables: 2
//		  Max root dir entries: 224
//		        FAT-12 sectors: 2880
//		           FAT sectors: 9
//		         Sectors/track: 18
//		          Heads/volume: 2
//		        FAT-32 sectors: 0
//		c:/lcc/projects/disk4:\>>
//
//	4. List root director (validates fmsGetNextDirEntry for root):
//
//		c:/lcc/projects/disk4:\>>dir
//		Name:ext                time      date    cluster  size
//		BIGDIR        ----D-  11:31:40 03/31/2004     3     0
//		BYU           ----D-  11:34:54 03/31/2004   171     0
//		JOKES         ----D-  11:37:06 03/31/2004   800     0
//		LONGFI~1      ----D-  11:37:14 03/31/2004   875     0
//		PERSONAL      ----D-  11:37:18 03/31/2004   937     0
//		TEMP          ----D-  11:37:36 03/31/2004  1355     0
//		H2O.C         -----A  19:00:02 02/12/2004  1380  3425
//		MAKE.TXT      -----A  16:26:58 02/27/2004  1387 18584
//		c:/lcc/projects/disk4:\>>
//
//	5. Open MAKE.TXT and H2O.C:
//
//		c:/lcc/projects/disk4:\>>open make.txt
//		Open File "make.txt",read
//		Slot  Name    Ext  Atr  Size  Strt  Curr  cDir  cPID  Mode  Flag  Indx
//		  0   MAKE    TXT   20 18584  1387     0     0     0     0     0     0
//		FileID = 0
//		c:/lcc/projects/disk4:\>>open h2o.c
//		Open File "h2o.c",read
//		Slot  Name    Ext  Atr  Size  Strt  Curr  cDir  cPID  Mode  Flag  Indx
//		  0   MAKE    TXT   20 18584  1387     0     0     0     0     0     0
//		  1   H2O     C     20  3425  1380     0     0     0     0     0     0
//		FileID = 1
//
//	6. Read from last opened file (H2O.c):
//
//		c:/lcc/projects/disk4:\>>read 20
//		Buffer[0-20] = // Hex to Object 01/
//		Slot  Name    Ext  Atr  Size  Strt  Curr  cDir  cPID  Mode  Flag  Indx
//		  0   MAKE    TXT   20 18584  1387     0     0     0     0     0     0
//		  1   H2O     C     20  3425  1380  1380     0     0     0     0    20
//		c:/lcc/projects/disk4:\>>read 500
//		Buffer[0-500] = 24/2004
//		#include <stdio.h>
//		...500 bytes...
//
//		Slot  Name    Ext  Atr  Size  Strt  Curr  cDir  cPID  Mode  Flag  Indx
//		  0   MAKE    TXT   20 18584  1387     0     0     0     0     0     0
//		  1   H2O     C     20  3425  1380  1381     0     0     0     0   520
//		c:/lcc/projects/disk4:\>>
//
//	7. Use the commands to test you file manager:
//
//		>>open <fileName>{,<mode>}
// 	>>read {<# of bytes>}
// 	>>write {<# of bytes>}
// 	>>seek {<seekIndex>}
// 	>>close {<fileId>}
//
//
// ***********************************************************************
// ***********************************************************************
//	Open file
//
//		>>open <fileName>{,<mode>}
//
//		where <fileName> = current directory file name
//				<mode> = 0=OPEN_READ
//							1=OPEN_WRITE
//							2=OPEN_APPEND
//							3=OPEN_RDWR
//
int lastFD;										// fileid of last opened file

int P6_open(int argc, char* argv[])		// open file
{
	int error, mode;
	char* omode[] = {"read", "write", "append", "r/w"};

	if (argc < 3) mode = OPEN_READ;
	else mode = INTEGER(argv[2]);
	mode %= 4;

	printf("\nOpen File \"%s\",%s", argv[1], omode[mode]);

	// open source file
	if ((lastFD = fmsOpenFile(argv[1], mode)) < 0)
	{
		fmsError(lastFD);
		return 0;
	}
	printf("\nFileID = %d", lastFD);

	SWAP;
//	P6_fileSlots(0, (char**)0);
	return 0;
} // end P6_type



// ***********************************************************************
// ***********************************************************************
//	Read and print bytes from file
//
// 	>>read {<# of bytes>}
//
//		where <# of bytes> = # of bytes to read from file
//									(default = 1, max = 512)
//
int P6_read(int argc, char* argv[])		// read file
{
	int error, nBytes;
	char buffer[520];

	if (argc < 2) nBytes = 1;
	else nBytes = INTEGER(argv[1]);
	nBytes %= 512;

	if ((error = fmsReadFile(lastFD, buffer, nBytes)) < 0)
	{
		fmsError(error);
		return 0;
	}
	// terminate buffer
	buffer[error] = 0;
	printf("\nBuffer[0-%d] = %s", error, buffer);

	SWAP;
	P6_fileSlots(0, (char**)0);
	return 0;
} // end P6_read



// ***********************************************************************
// ***********************************************************************
//	Write and print alphabet to file
//
// 	>>write {<# of bytes>}
//
//		where <# of bytes> = # of bytes to write to file
//									(default = 1, max = 52)
//
int P6_write(int argc, char* argv[])		// write file
{
	int error, nBytes;
	char buffer[520];
	strcpy(buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

	if (argc < 2) nBytes = 1;
	else nBytes = INTEGER(argv[1]);
	nBytes %= strlen(buffer);

	if ((error = fmsWriteFile(lastFD, buffer, nBytes)) < 0)
	{
		fmsError(error);
		return 0;
	}

	buffer[nBytes] = 0;
	printf("\nWrite = %s", buffer);

	SWAP;
	P6_fileSlots(0, (char**)0);
	return 0;
} // end P6_write


// ***********************************************************************
// ***********************************************************************
//	Seek within last opened file file
//
// 	>>seek {<seekIndex>}
//
//		where <seekIndex> = byte index into last opened file
//								  (default = 0)
//
int P6_seek(int argc, char* argv[])		// seek file
{
	int error, fileID, seekIndex;

	// default to last accessed file or argument
	if (argc < 2) seekIndex = 0;
	else seekIndex = INTEGER(argv[1]);

	if ((error = fmsSeekFile(lastFD, seekIndex)) < 0)
	{
		fmsError(error);
		return 0;
	}

	SWAP;
	P6_fileSlots(0, (char**)0);
	return 0;
} // end P6_seek




// ***********************************************************************
// ***********************************************************************
//	Close file
//
// 	>>close {<fileId>}
//
//		where <fileId> = file ID as returned from open
//							  (default = last opened file)
//
int P6_close(int argc, char* argv[])		// close file
{
	int error, fileID;

	// default to last accessed file or argument
	if (argc < 2) fileID = lastFD;
	else fileID = INTEGER(argv[1]);

	if ((error = fmsCloseFile(fileID)) < 0)
	{
		fmsError(error);
		return 0;
	}

	SWAP;
	P6_fileSlots(0, (char**)0);
	return 0;
} // end P6_close





// ***************************************************************************************
// ***************************************************************************************
// Support functions
// ***************************************************************************************
// ***************************************************************************************

// ***************************************************************************************
// ***************************************************************************************
//	setDirTimeDate
//
//		struct tm
//		{
//			int tm_sec;     // 0 to 60
//			int tm_min;     // 0 to 59
//			int tm_hour;    // 0 to 23
//			int tm_mday;    // 1 to 31
//			int tm_mon;     // 0 to 11
//			int tm_year;    // year - 1900
//			int tm_wday;    // Sunday = 0
//			int tm_yday;    // 0 to 365
//			int tm_isdst;   // >0 if Daylight Savings Time,
//			                //  0 if Standard,
//			                // <0 if unknown
//			char *tm_zone;  // time zone name
//			long tm_gmtoff; // offset from GMT
//		};
//
void setDirTimeDate(DirEntry* dir)
{
   time_t a;
   struct tm *b;

   time(&a);
   b = localtime(&a);
   dir->date.year = b->tm_year + 1900 - 1980;
   dir->date.month = b->tm_mon;
   dir->date.day = b->tm_mday;

   dir->time.hour = b->tm_hour;
   dir->time.min = b->tm_min;
   dir->time.sec = b->tm_sec;
   return;
} // end setDirTimeDate



// ***************************************************************************************
// ***************************************************************************************
// Error processor
void fmsError(int error)
{
   int i;

   for (i=0; i<NUM_ERRORS; i++)
   {
      if (FMSErrors[i].error == error)
      {
         printf("\n%s", FMSErrors[i].error_msg);
         return;
      }
   }
   printf("\n%s %d", UNDEFINED_MSG, error);
   return;
} // end fmsError


// ***************************************************************************************
// ***************************************************************************************
//	check for valid DOS 8.3 file name
//
//	return 	 1 = valid 8.3 name
//				 0 = invalid name
//				-1 = valid long file name
//
//	Valid characters: ` ~ ! @ # $ % ^ & ( ) _ - { } '
// Cannot start with .
//	Invalid Characters: "  / : * < > | ?
//
//		while((string_ptr=strpbrk(string," "))!=NULL) *string_ptr='-';
//
int isValidFileName(char* fileName)
{
	char* s;
	// cannot start with '.'
	if (fileName[0] == '.') return 0;
	// check for invalid characters
	if (strpbrk(fileName, "\"/:*<>|?")) return 0;
	// check for double period
	if (s = strchr(fileName, '.'))
	{
		if (strchr(s+1, '.')) return 0;			// more than 1 '.'
		if (strlen(s+1) > 3) return 0;			// too long of extension
		if ((strlen(fileName) - strlen(s)) <= 8) return 1;
		else return -1;
	}
	if (strlen(fileName) <= 8) return 1;
	return -1;
} // end isValidFileName



// ***************************************************************************************
// ***************************************************************************************
int fmsMask(char* mask, char* name, char* ext)
{
   int i,j;
   // check for ..
   if (!strcmp(mask, ".."))
      if (!strncmp(name, "..", 2)) return 1;
      else return 0;

   // look thru name
   for (i=0,j=0; j<8; i++,j++)
   {
      if (mask[i] == '*') {i++; break;}
      if ((mask[i] == '.') && (name[j] == ' ')) {i++; break;}
      if ((mask[i] == '?') && (name[j] != ' ')) continue;
      if (!mask[i] && (name[j] == ' ') && (ext[0] == ' ')) return 1;
      if ((mask[i] != toupper(name[j])) && (mask[i] != tolower(name[j]))) return 0;
   }
   while (mask[i] == '.') i++;
   // check extension
   for (j=0; j<3; i++,j++)
   {
      if (mask[i] == '*') return 1;
      if (!mask[i] && (ext[j] == ' ')) return 1;
      if ((mask[i] == '?') && (ext[i] != ' ')) continue;
      if ((mask[i] != toupper(ext[j])) && (mask[i] != tolower(ext[j]))) return 0;
   }
   return 1;
} // end fmsMask



// ***************************************************************************************
// ***************************************************************************************
int fmsGetDirEntry(char* fileName, DirEntry* dirEntry)
// This function returns the directory entry from the current directory as
//    specified by fileName.
// Return 0 for success; otherwise, return an error number.
//
//    ERR61 = File Not Defined
{
   int error, index = 0;
	//if (isValidFileName(fileName) < 1) return ERR50;
   error = fmsGetNextDirEntry(&index, fileName, dirEntry, CDIR);
	return (error ? ((error == ERR67) ? ERR61 : error) : 0);
} // end fmsGetDirEntry



// ***************************************************************************************
// ***************************************************************************************
int fmsGetNextDirEntry(int *dirNum, char* mask, DirEntry* dirEntry, int dir)
//	Called by dir or ls command.
// This function returns the next directory entry of the current directory.
//	The dirNum parameter is set to 0 for the first entry and is subsequently
//    updated for each additional call.
//	The next directory entry is returned in the 32 byte directory structure dirEntry:
//	The parameter mask is a selection string.  If null, return next directory entry.
//    Otherwise, use the mask string to select the next directory entry.
//    A '*' is a wild card for any length string.
//    A '?' is a wild card for any single character.
//    Any other character must match exactly.
// NOTE:
//		*.*		all files
//		*			all files w/o extension
//		a*.txt	all files beginning with the character 'a' and with a .txt extension
//	Return 0 for success, otherwise, return the error number.
//
//    ERR54 = Invalid FAT Chain
//    ERR67 = End of Directory
{
	char buffer[BYTES_PER_SECTOR];
	int dirIndex, dirSector, error;
	int loop = *dirNum / ENTRIES_PER_SECTOR;
	int dirCluster = dir;

	while(1)
	{	// load directory sector
		if (dir)
		{	// sub directory
			while(loop--)
			{
				dirCluster = getFatEntry(dirCluster, FAT1);
				if (dirCluster == FAT_EOC) return ERR67;
				if (dirCluster == FAT_BAD) return ERR54;
				if (dirCluster < 2) return ERR54;
			}
			dirSector = C_2_S(dirCluster);
		}
		else
		{	// root directory
			dirSector = (*dirNum / ENTRIES_PER_SECTOR) + BEG_ROOT_SECTOR;
			if (dirSector >= BEG_DATA_SECTOR) return ERR67;
		}

		// read sector into directory buffer
		if (error = fmsReadSector(buffer, dirSector)) return error;

		// find next matching directory entry
		while(1)
		{	// read directory entry
			dirIndex = *dirNum % ENTRIES_PER_SECTOR;
			memcpy(dirEntry, &buffer[dirIndex * sizeof(DirEntry)], sizeof(DirEntry));
			if (dirEntry->name[0] == 0) return ERR67;	// EOD
			(*dirNum)++;                        		// prepare for next read
			if (dirEntry->name[0] == 0xe5);     		// Deleted entry, go on...
			else if (dirEntry->attributes == LONGNAME);
			else if (fmsMask(mask, dirEntry->name, dirEntry->extension)) return 0;   // return if valid
			// break if sector boundary
			if ((*dirNum % ENTRIES_PER_SECTOR) == 0) break;
		}
		// next directory sector/cluster
		loop = 1;
   }
	return 0;
} // end fmsGetNextDirEntry



// ***************************************************************************************
// ***************************************************************************************
int fmsChangeDir(char* dirName)
//	Called by cd command.
// This function changes the current directory to the subdirectory specified by
//    the argument dirname.
// You will only need to handle moving up a directory or moving down a subdirectory.
//	Verify that dirname is a valid directory name in the current directory.
//	Return 0 for success, otherwise, return the error number.
{
   int i, error;
	DirEntry dirEntry;

	// need to allow for . and ..
	//if (isValidFileName(dirName) < 1) return ERR50;
   if ((error = fmsGetDirEntry(dirName, &dirEntry))) return error;
   if (dirEntry.attributes != DIRECTORY) return ERR55;
   CDIR = dirEntry.startCluster;

	// keep track of path name
	if (!strcmp(dirName, ".")) return 0;
	if (!strcmp(dirName, ".."))
	{	// drop last directory
		for (i=strlen(dirPath)-1; i>0; i--) if (dirPath[i] == '\\') break;
		if (dirPath[i-1] == ':') i++;
		dirPath[i] = '\0';
		return 0;
	}
	// add new path
	if (dirPath[strlen(dirPath)-1] != '\\') strcat(dirPath, "\\");
	strcat(dirPath, dirName);
   return 0;
} // end fmsChangeDir



// ***************************************************************************************
// ***************************************************************************************
// size disk
void sizeDisk(DiskSize* dskSize, char* mask, int dir)
{
	int index = 0;
	DirEntry dirEntry;

	while (1)
	{
		if (fmsGetNextDirEntry(&index, mask, &dirEntry, dir)) break;
		if (dirEntry.name[0] == '.') continue;
		dskSize->used += ((dirEntry.fileSize + BYTES_PER_SECTOR) / BYTES_PER_SECTOR);	         // # of sectors used
		if ((dirEntry.attributes == DIRECTORY) && (dirEntry.name[0] != '.'))
		{
			sizeDisk(dskSize, mask, dirEntry.startCluster);		// get space of subdirectory
			dskSize->used += ((dirEntry.fileSize + BYTES_PER_SECTOR) / BYTES_PER_SECTOR);	         // # of sectors used
		}
	}
	return;
} // end sizeDisk


int fmsDiskStats(DiskSize* dskSize)
//	Called by sp command.
// This function returns the number of free, used, bad, and total sectors
//    on your RAM disk in the structure dskSize.
//	Return 0 for success; otherwise, return an error number.
{
	sizeDisk(dskSize, "*.*", 0);
	dskSize->free = CLUSTERS_PER_DISK - dskSize->used;	// # of sectors free
	dskSize->bad = 0;  	                        // # of bad sectors
	dskSize->size = CLUSTERS_PER_DISK;	         // Total # of sectors in RAM disk
	return 0;
} // end fmsDiskStats



// ***************************************************************************************
// ***************************************************************************************
int fmsMount(char* fileName, void* ramDisk)
//	Called by mount command.
// This function loads a RAM disk image from a file.
//	The parameter fileName is the file path name of the disk image.
//	The parameter ramDisk is a pointer to a character array whose
//    size is equal to a 1.4 mb floppy disk (2849 ´ 512 bytes).
//	Return 0 for success, otherwise, return the error number
{
   FILE* fp;
   fp = fopen(fileName, "rb");
   if (fp)
   {
      fread(ramDisk, sizeof(char), SECTORS_PER_DISK * BYTES_PER_SECTOR, fp);
   }
   else return -1;
   fclose(fp);
	// copy FAT table to memory
	memcpy(FAT1, &RAMDisk[1 * BYTES_PER_SECTOR], NUM_FAT_SECTORS * BYTES_PER_SECTOR);
	memcpy(FAT2, &RAMDisk[10 * BYTES_PER_SECTOR], NUM_FAT_SECTORS * BYTES_PER_SECTOR);
	diskMounted = 1;				// disk has been mounted
 	strcpy(dirPath, fileName);
	strcat(dirPath, ":\\");
	return 0;
} // end fmsMount



// ***************************************************************************************
// ***************************************************************************************
int fmsUnMount(char* fileName, void* ramDisk)
// Called by the unmount command.
// This function unloads your Project 5 RAM disk image to file computer file.
// The parameter fileName is the file path name of the disk image.
// The pointer parameter ramDisk points to a character array whose size is equal to a 1.4
// mb floppy disk (2849 ´ 512 bytes).
// Return 0 for success; otherwise, return the error number.
{
	diskMounted = 0;							// unmount disk
	// ?? add code here
	printf("\nfmsUnMount Not Implemented");

	return -1;
} // end fmsUnMount



// ***************************************************************************************
// ***************************************************************************************
int fmsReadSector(void* buffer, int sectorNumber)
//	Read into buffer RAM disk sector number sectorNumber.
// Sectors are 512 bytes.
//	Return 0 for success; otherwise, return an error number.
{
   memcpy(buffer, &RAMDisk[sectorNumber * BYTES_PER_SECTOR], BYTES_PER_SECTOR);
   ++sectorReads;
   return 0;
} // end fmsReadSector



// ***************************************************************************************
// ***************************************************************************************
int fmsWriteSector(void* buffer, int sectorNumber)
// Write 512 bytes from memory pointed to by buffer to RAM disk sector sectorNumber.
// Return 0 for success; otherwise, return an error number.
{
	memcpy(&RAMDisk[sectorNumber * BYTES_PER_SECTOR], buffer, BYTES_PER_SECTOR);
	++sectorWrites;
	return 0;
} // end fmsWriteSector



// ***************************************************************************************
// Take a FAT table index and return an unsigned short containing the 12-bit FAT entry code
// ***************************************************************************************
void setFatEntry(int FATindex, unsigned short FAT12ClusEntryVal, unsigned char* FAT)
{
	int FATOffset = ((FATindex * 3) / 2);		// Calculate the offset
	int FATData = *((unsigned short*)&FAT[FATOffset]);
	FATData = BigEndian(FATData);
	if (FATindex % 2 == 0)						// If the index is even
	{	FAT12ClusEntryVal &= 0x0FFF;			// mask to 12 bits
	FATData &= 0xF000;							// mask complement
	}
	else												// Index is odd
	{	FAT12ClusEntryVal <<= 4; 				// move 12-bits high
		FATData &= 0x000F;						// mask complement
	}
	// Update FAT entry value in the FAT table
	FATData = BigEndian(FATData);
	*((unsigned short *)&FAT[FATOffset]) = FATData | FAT12ClusEntryVal;
	return;
} // End SetFatEntry



// ***************************************************************************************
// ***************************************************************************************
// Replace the 12-bit FAT entry code in the unsigned char FAT table at index
unsigned short getFatEntry(int FATindex, unsigned char* FATtable)
{
	unsigned short FATEntryCode;    			// The return value
	int FatOffset = ((FATindex * 3) / 2);	// Calculate the offset of the unsigned short to get
	if ((FATindex % 2) == 1)    				// If the index is odd
	{
		// Pull out a unsigned short from a unsigned char array
		FATEntryCode = *((unsigned short *)&FATtable[FatOffset]);
      FATEntryCode = BigEndian(FATEntryCode);
		FATEntryCode >>= 4;   					// Extract the high-order 12 bits
	}
	else												// If the index is even
	{
		// Pull out a unsigned short from a unsigned char array
		FATEntryCode = *((unsigned short *)&FATtable[FatOffset]);
      FATEntryCode = BigEndian(FATEntryCode);
      FATEntryCode &= 0x0fff;    			// Extract the low-order 12 bits
	}
	return FATEntryCode;
} // end GetFatEntry

// ***************************************************************************************
// ***************************************************************************************

