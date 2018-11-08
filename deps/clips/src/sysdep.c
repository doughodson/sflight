   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/05/18             */
   /*                                                     */
   /*               SYSTEM DEPENDENT MODULE               */
   /*******************************************************/

/*************************************************************/
/* Purpose: Isolation of system dependent routines.          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Modified GenOpen to check the file length      */
/*            against the system constant FILENAME_MAX.      */
/*                                                           */
/*      6.24: Support for run-time programs directly passing */
/*            the hash tables for initialization.            */
/*                                                           */
/*            Made gensystem functional for Xcode.           */
/*                                                           */
/*            Added BeforeOpenFunction and AfterOpenFunction */
/*            hooks.                                         */
/*                                                           */
/*            Added environment parameter to GenClose.       */
/*            Added environment parameter to GenOpen.        */
/*                                                           */
/*            Updated UNIX_V gentime functionality.          */
/*                                                           */
/*            Removed GenOpen check against FILENAME_MAX.    */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, IBM_ICB, IBM_TBC, IBM_ZTC, and        */
/*            IBM_SC).                                       */
/*                                                           */
/*            Renamed IBM_MSC and WIN_MVC compiler flags     */
/*            and IBM_GCC to WIN_GCC.                        */
/*                                                           */
/*            Added LINUX and DARWIN compiler flags.         */
/*                                                           */
/*            Removed HELP_FUNCTIONS compilation flag and    */
/*            associated functionality.                      */
/*                                                           */
/*            Removed EMACS_EDITOR compilation flag and      */
/*            associated functionality.                      */
/*                                                           */
/*            Combined BASIC_IO and EXT_IO compilation       */
/*            flags into the single IO_FUNCTIONS flag.       */
/*                                                           */
/*            Changed the EX_MATH compilation flag to        */
/*            EXTENDED_MATH_FUNCTIONS.                       */
/*                                                           */
/*            Support for typed EXTERNAL_ADDRESS_TYPE.       */
/*                                                           */
/*            GenOpen function checks for UTF-8 Byte Order   */
/*            Marker.                                        */
/*                                                           */
/*            Added gengetchar, genungetchar, genprintfile,  */
/*            genstrcpy, genstrncpy, genstrcat, genstrncat,  */
/*            and gensprintf functions.                      */
/*                                                           */
/*            Added SetJmpBuffer function.                   */
/*                                                           */
/*            Added environment argument to genexit.         */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.40: Added genchdir function for changing the       */
/*            current directory.                             */
/*                                                           */
/*            Modified gentime to return "comparable" epoch  */
/*            based values across platforms.                 */
/*                                                           */
/*            Refactored code to reduce header dependencies  */
/*            in sysdep.c.                                   */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            Moved CatchCtrlC to main.c.                    */
/*                                                           */
/*            Removed VAX_VMS support.                       */
/*                                                           */
/*            Removed ContinueEnvFunction, PauseEnvFunction, */
/*            and RedrawScreenFunction callbacks.            */
/*                                                           */
/*            Completion code now returned by gensystem.     */
/*                                                           */
/*            Added flush, rewind, tell, and seek functions. */
/*                                                           */
/*            Removed fflush calls after printing.           */
/*                                                           */
/*            GenOpen, genchdir, genremove, and genrename    */
/*            use wide character functions on Windows to     */
/*            work properly with file and directory names    */
/*            containing unicode characters.                 */
/*                                                           */
/*************************************************************/

#include "setup.h"

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>

#if MAC_XCD
#include <sys/time.h>
#include <unistd.h>
#endif

#if WIN_MVC
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>
#endif

#if   UNIX_7 || WIN_GCC
#include <sys/types.h>
#include <sys/timeb.h>
#include <signal.h>
#endif

#if   UNIX_V || LINUX || DARWIN
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "envrnmnt.h"
#include "memalloc.h"
#include "sysdep.h"

/********************/
/* ENVIRONMENT DATA */
/********************/

#define SYSTEM_DEPENDENT_DATA 58

struct systemDependentData
  {
#if WIN_MVC
   int BinaryFileHandle;
   unsigned char getcBuffer[7];
   int getcLength;
   int getcPosition;
#endif
#if (! WIN_MVC)
   FILE *BinaryFP;
#endif
   int (*BeforeOpenFunction)(Environment *);
   int (*AfterOpenFunction)(Environment *);
   jmp_buf *jmpBuffer;
  };

#define SystemDependentData(theEnv) ((struct systemDependentData *) GetEnvironmentData(theEnv,SYSTEM_DEPENDENT_DATA))

/********************************************************/
/* InitializeSystemDependentData: Allocates environment */
/*    data for system dependent routines.               */
/********************************************************/
void InitializeSystemDependentData(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,SYSTEM_DEPENDENT_DATA,sizeof(struct systemDependentData),NULL);
  }

/*********************************************************/
/* gentime: A function to return a floating point number */
/*   which indicates the present time. Used internally   */
/*   for timing rule firings and debugging.              */
/*********************************************************/
double gentime()
  {
#if MAC_XCD || UNIX_V || DARWIN || LINUX || UNIX_7
   struct timeval now;
   gettimeofday(&now, 0);
   return (now.tv_usec / 1000000.0) + now.tv_sec;
#elif WIN_MVC
    FILETIME ft;
	unsigned long long tt;

    GetSystemTimeAsFileTime(&ft);
	tt = ft.dwHighDateTime;
    tt <<=32;
    tt |= ft.dwLowDateTime;
    tt /=10;
    tt -= 11644473600000000ULL;
	return (double) tt / 1000000.0;
#else
   return((double) time(NULL));
#endif
  }

/*****************************************************/
/* gensystem: Generic routine for passing a string   */
/*   representing a command to the operating system. */
/*****************************************************/
int gensystem(
  Environment *theEnv,
  const char *commandBuffer)
  {
   return system(commandBuffer);
  }

/*******************************************/
/* gengetchar: Generic routine for getting */
/*    a character from stdin.              */
/*******************************************/
int gengetchar(
  Environment *theEnv)
  {
   return(getc(stdin));
  }

/***********************************************/
/* genungetchar: Generic routine for ungetting */
/*    a character from stdin.                  */
/***********************************************/
int genungetchar(
  Environment *theEnv,
  int theChar)
  {
   return(ungetc(theChar,stdin));
  }

/****************************************************/
/* genprintfile: Generic routine for print a single */
/*   character string to a file (including stdout). */
/****************************************************/
void genprintfile(
  Environment *theEnv,
  FILE *fptr,
  const char *str)
  {
   fprintf(fptr,"%s",str);
  }

/***********************************************************/
/* InitializeNonportableFeatures: Initializes non-portable */
/*   features. Currently, the only non-portable feature    */
/*   requiring initialization is the interrupt handler     */
/*   which allows execution to be halted.                  */
/***********************************************************/
void InitializeNonportableFeatures(
  Environment *theEnv)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
  }

/**************************************/
/* genexit:  A generic exit function. */
/**************************************/
void genexit(
  Environment *theEnv,
  int num)
  {
   if (SystemDependentData(theEnv)->jmpBuffer != NULL)
     { longjmp(*SystemDependentData(theEnv)->jmpBuffer,1); }

   exit(num);
  }

/**************************************/
/* SetJmpBuffer: */
/**************************************/
void SetJmpBuffer(
  Environment *theEnv,
  jmp_buf *theJmpBuffer)
  {
   SystemDependentData(theEnv)->jmpBuffer = theJmpBuffer;
  }

/******************************************/
/* genstrcpy: Generic genstrcpy function. */
/******************************************/
char *genstrcpy(
  char *dest,
  const char *src)
  {
   return strcpy(dest,src);
  }

/********************************************/
/* genstrncpy: Generic genstrncpy function. */
/********************************************/
char *genstrncpy(
  char *dest,
  const char *src,
  size_t n)
  {
   return strncpy(dest,src,n);
  }

/******************************************/
/* genstrcat: Generic genstrcat function. */
/******************************************/
char *genstrcat(
  char *dest,
  const char *src)
  {
   return strcat(dest,src);
  }

/********************************************/
/* genstrncat: Generic genstrncat function. */
/********************************************/
char *genstrncat(
  char *dest,
  const char *src,
  size_t n)
  {
   return strncat(dest,src,n);
  }

/*****************************************/
/* gensprintf: Generic sprintf function. */
/*****************************************/
int gensprintf(
  char *buffer,
  const char *restrictStr,
  ...)
  {
   va_list args;
   int rv;

   va_start(args,restrictStr);

   rv = vsprintf(buffer,restrictStr,args);

   va_end(args);

   return rv;
  }

/******************************************************/
/* genrand: Generic random number generator function. */
/******************************************************/
int genrand()
  {
   return(rand());
  }

/**********************************************************************/
/* genseed: Generic function for seeding the random number generator. */
/**********************************************************************/
void genseed(
  unsigned int seed)
  {
   srand(seed);
  }

/*********************************************/
/* gengetcwd: Generic function for returning */
/*   the current directory.                  */
/*********************************************/
char *gengetcwd(
  char *buffer,
  int buflength)
  {
#if MAC_XCD
   return(getcwd(buffer,buflength));
#else
   if (buffer != NULL)
     { buffer[0] = 0; }
   return(buffer);
#endif
  }

/*************************************************/
/* genchdir: Generic function for setting the    */
/*   current directory. Returns 1 if successful, */
/*   0 if unsuccessful, and -1 if unavailable.   */
/*************************************************/
int genchdir(
  Environment *theEnv,
  const char *directory)
  {
   int rv = -1;
   
   /*==========================================================*/
   /* If the directory argument is NULL, then the return value */
   /* indicates whether the chdir functionality is supported.  */
   /*==========================================================*/
   
   if (directory == NULL)
     {
#if MAC_XCD || DARWIN || LINUX || WIN_MVC
      return 1;
#else
      return 0;
#endif
     }
     
   /*========================================*/
   /* Otherwise, try changing the directory. */
   /*========================================*/
   
#if MAC_XCD || DARWIN || LINUX
   rv = chdir(directory) + 1;
#endif
#if WIN_MVC 
   wchar_t *wdirectory;
   int wlength;

   wlength = MultiByteToWideChar(CP_UTF8,0,directory,-1,NULL,0);

   wdirectory = (wchar_t *) genalloc(theEnv,wlength * sizeof(wchar_t)); 

   MultiByteToWideChar(CP_UTF8,0,directory,-1,wdirectory,wlength);

   rv = _wchdir(wdirectory) + 1;

   genfree(theEnv,wdirectory,wlength * sizeof(wchar_t));
#endif

   return rv;
  }

/****************************************************/
/* genremove: Generic function for removing a file. */
/****************************************************/
bool genremove(
  Environment *theEnv,
  const char *fileName)
  {
#if WIN_MVC
   wchar_t *wfileName;
   int wfnlength;
#endif

#if WIN_MVC
   wfnlength = MultiByteToWideChar(CP_UTF8,0,fileName,-1,NULL,0);

   wfileName = (wchar_t *) genalloc(theEnv,wfnlength * sizeof(wchar_t)); 

   MultiByteToWideChar(CP_UTF8,0,fileName,-1,wfileName,wfnlength);

   if (_wremove(wfileName))
     { 
      genfree(theEnv,wfileName,wfnlength * sizeof(wchar_t));
      return false;
     }

   genfree(theEnv,wfileName,wfnlength * sizeof(wchar_t));
#else
   if (remove(fileName)) return false;
#endif

   return true;
  }

/****************************************************/
/* genrename: Generic function for renaming a file. */
/****************************************************/
bool genrename(
  Environment *theEnv,
  const char *oldFileName,
  const char *newFileName)
  {
#if WIN_MVC
   wchar_t *woldFileName, *wnewFileName;
   int wofnlength, wnfnlength;
#endif

#if WIN_MVC
   wofnlength = MultiByteToWideChar(CP_UTF8,0,oldFileName,-1,NULL,0);
   wnfnlength = MultiByteToWideChar(CP_UTF8,0,newFileName,-1,NULL,0);

   woldFileName = (wchar_t *) genalloc(theEnv,wofnlength * sizeof(wchar_t)); 
   wnewFileName = (wchar_t *) genalloc(theEnv,wnfnlength * sizeof(wchar_t)); 

   MultiByteToWideChar(CP_UTF8,0,oldFileName,-1,woldFileName,wofnlength);
   MultiByteToWideChar(CP_UTF8,0,newFileName,-1,wnewFileName,wnfnlength);

   if (_wrename(woldFileName,wnewFileName))
     { 
      genfree(theEnv,woldFileName,wofnlength * sizeof(wchar_t));
      genfree(theEnv,wnewFileName,wnfnlength * sizeof(wchar_t));
      return false;
     }

   genfree(theEnv,woldFileName,wofnlength * sizeof(wchar_t));
   genfree(theEnv,wnewFileName,wnfnlength * sizeof(wchar_t));
#else
   if (rename(oldFileName,newFileName)) return false;
#endif

   return true;
  }

/***********************************/
/* SetBeforeOpenFunction: Sets the */
/*  value of BeforeOpenFunction.   */
/***********************************/
int (*SetBeforeOpenFunction(Environment *theEnv,
                               int (*theFunction)(Environment *)))(Environment *)
  {
   int (*tempFunction)(Environment *);

   tempFunction = SystemDependentData(theEnv)->BeforeOpenFunction;
   SystemDependentData(theEnv)->BeforeOpenFunction = theFunction;
   return(tempFunction);
  }

/**********************************/
/* SetAfterOpenFunction: Sets the */
/*  value of AfterOpenFunction.   */
/**********************************/
int (*SetAfterOpenFunction(Environment *theEnv,
                              int (*theFunction)(Environment *)))(Environment *)
  {
   int (*tempFunction)(Environment *);

   tempFunction = SystemDependentData(theEnv)->AfterOpenFunction;
   SystemDependentData(theEnv)->AfterOpenFunction = theFunction;
   return(tempFunction);
  }

/*********************************************/
/* GenOpen: Trap routine for opening a file. */
/*********************************************/
FILE *GenOpen(
  Environment *theEnv,
  const char *fileName,
  const char *accessType)
  {
   FILE *theFile;
#if WIN_MVC
   wchar_t *wfileName;
   wchar_t *waccessType;
   int wfnlength, watlength;
#endif
   /*==================================*/
   /* Invoke the before open function. */
   /*==================================*/

   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }

   /*================*/
   /* Open the file. */
   /*================*/

#if WIN_MVC
   wfnlength = MultiByteToWideChar(CP_UTF8,0,fileName,-1,NULL,0);
   watlength = MultiByteToWideChar(CP_UTF8,0,accessType,-1,NULL,0);

   wfileName = (wchar_t *) genalloc(theEnv,wfnlength * sizeof(wchar_t)); 
   waccessType = (wchar_t *) genalloc(theEnv,watlength * sizeof(wchar_t)); 

   MultiByteToWideChar(CP_UTF8,0,fileName,-1,wfileName,wfnlength);
   MultiByteToWideChar(CP_UTF8,0,accessType,-1,waccessType,watlength);

   _wfopen_s(&theFile,wfileName,waccessType);

   genfree(theEnv,wfileName,wfnlength * sizeof(wchar_t));
   genfree(theEnv,waccessType,watlength * sizeof(wchar_t));
#else
   theFile = fopen(fileName,accessType);
#endif

   /*=====================================*/
   /* Check for a UTF-8 Byte Order Marker */
   /* (BOM): 0xEF,0xBB,0xBF.              */
   /*=====================================*/

   if ((theFile != NULL) & (strcmp(accessType,"r") == 0))
     {
      int theChar;

      theChar = getc(theFile);
      if (theChar == 0xEF)
       {
        theChar = getc(theFile);
        if (theChar == 0xBB)
          {
           theChar = getc(theFile);
           if (theChar != 0xBF)
             { ungetc(theChar,theFile);}
          }
        else
          { ungetc(theChar,theFile);}
       }
      else
       { ungetc(theChar,theFile); }
     }

   /*=================================*/
   /* Invoke the after open function. */
   /*=================================*/

   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }

   /*===============================*/
   /* Return a pointer to the file. */
   /*===============================*/

   return theFile;
  }

/**********************************************/
/* GenClose: Trap routine for closing a file. */
/**********************************************/
int GenClose(
  Environment *theEnv,
  FILE *theFile)
  {
   int rv;

   rv = fclose(theFile);

   return rv;
  }

/***********************************************/
/* GenFlush: Trap routine for flushing a file. */
/***********************************************/
int GenFlush(
  Environment *theEnv,
  FILE *theFile)
  {
   int rv;

   rv = fflush(theFile);

   return rv;
  }

/*************************************************/
/* GenRewind: Trap routine for rewinding a file. */
/*************************************************/
void GenRewind(
  Environment *theEnv,
  FILE *theFile)
  {
   rewind(theFile);
  }

/*************************************************/
/* GenTell: Trap routine for the ftell function. */
/*************************************************/
long long GenTell(
  Environment *theEnv,
  FILE *theFile)
  {
   long long rv;
   
   rv = ftell(theFile);
   
   if (rv == -1)
     {
      if (errno > 0)
        { return LLONG_MIN; }
     }
   
   return rv;
  }

/*************************************************/
/* GenSeek: Trap routine for the fseek function. */
/*************************************************/
int GenSeek(
  Environment *theEnv,
  FILE *theFile,
  long offset,
  int whereFrom)
  {
   return fseek(theFile,offset,whereFrom);
  }

/************************************************************/
/* GenOpenReadBinary: Generic and machine specific code for */
/*   opening a file for binary access. Only one file may be */
/*   open at a time when using this function since the file */
/*   pointer is stored in a global variable.                */
/************************************************************/
int GenOpenReadBinary(
  Environment *theEnv,
  const char *funcName,
  const char *fileName)
  {
   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }

#if WIN_MVC
   SystemDependentData(theEnv)->BinaryFileHandle = _open(fileName,O_RDONLY | O_BINARY);
   if (SystemDependentData(theEnv)->BinaryFileHandle == -1)
     {
      if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
        { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
      return 0;
     }
#endif

#if (! WIN_MVC)
   if ((SystemDependentData(theEnv)->BinaryFP = fopen(fileName,"rb")) == NULL)
     {
      if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
        { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
      return 0;
     }
#endif

   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }

   return 1;
  }

/***********************************************/
/* GenReadBinary: Generic and machine specific */
/*   code for reading from a file.             */
/***********************************************/
void GenReadBinary(
  Environment *theEnv,
  void *dataPtr,
  size_t size)
  {
#if WIN_MVC
   char *tempPtr;

   tempPtr = (char *) dataPtr;
   while (size > INT_MAX)
     {
      _read(SystemDependentData(theEnv)->BinaryFileHandle,tempPtr,INT_MAX);
      size -= INT_MAX;
      tempPtr = tempPtr + INT_MAX;
     }

   if (size > 0)
     { _read(SystemDependentData(theEnv)->BinaryFileHandle,tempPtr,(unsigned int) size); }
#endif

#if (! WIN_MVC)
   fread(dataPtr,size,1,SystemDependentData(theEnv)->BinaryFP);
#endif
  }

/***************************************************/
/* GetSeekCurBinary:  Generic and machine specific */
/*   code for seeking a position in a file.        */
/***************************************************/
void GetSeekCurBinary(
  Environment *theEnv,
  long offset)
  {
#if WIN_MVC
   _lseek(SystemDependentData(theEnv)->BinaryFileHandle,offset,SEEK_CUR);
#endif

#if (! WIN_MVC)
   fseek(SystemDependentData(theEnv)->BinaryFP,offset,SEEK_CUR);
#endif
  }

/***************************************************/
/* GetSeekSetBinary:  Generic and machine specific */
/*   code for seeking a position in a file.        */
/***************************************************/
void GetSeekSetBinary(
  Environment *theEnv,
  long offset)
  {
#if WIN_MVC
   _lseek(SystemDependentData(theEnv)->BinaryFileHandle,offset,SEEK_SET);
#endif

#if (! WIN_MVC)
   fseek(SystemDependentData(theEnv)->BinaryFP,offset,SEEK_SET);
#endif
  }

/************************************************/
/* GenTellBinary:  Generic and machine specific */
/*   code for telling a position in a file.     */
/************************************************/
void GenTellBinary(
  Environment *theEnv,
  long *offset)
  {
#if WIN_MVC
   *offset = _lseek(SystemDependentData(theEnv)->BinaryFileHandle,0,SEEK_CUR);
#endif

#if (! WIN_MVC)
   *offset = ftell(SystemDependentData(theEnv)->BinaryFP);
#endif
  }

/****************************************/
/* GenCloseBinary:  Generic and machine */
/*   specific code for closing a file.  */
/****************************************/
void GenCloseBinary(
  Environment *theEnv)
  {
   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }

#if WIN_MVC
   _close(SystemDependentData(theEnv)->BinaryFileHandle);
#endif

#if (! WIN_MVC)
   fclose(SystemDependentData(theEnv)->BinaryFP);
#endif

   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
  }

/***********************************************/
/* GenWrite: Generic routine for writing to a  */
/*   file. No machine specific code as of yet. */
/***********************************************/
void GenWrite(
  void *dataPtr,
  size_t size,
  FILE *fp)
  {
   if (size == 0) return;
#if UNIX_7
   fwrite(dataPtr,size,1,fp);
#else
   fwrite(dataPtr,size,1,fp);
#endif
  }
