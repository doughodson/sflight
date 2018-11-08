   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/11/17             */
   /*                                                     */
   /*                    MEMORY MODULE                    */
   /*******************************************************/

/*************************************************************/
/* Purpose: Memory allocation routines.                      */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed HaltExecution check from the           */
/*            EnvReleaseMem function. DR0863                 */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Corrected code to remove compiler warnings.    */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems.                   */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Removed genlongalloc/genlongfree functions.    */
/*                                                           */
/*            Added get_mem and rtn_mem macros.              */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Removed deallocating message parameter from    */
/*            EnvReleaseMem.                                 */
/*                                                           */
/*            Removed support for BLOCK_MEMORY.              */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#include "constant.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "prntutil.h"
#include "router.h"
#include "utility.h"

#include <stdlib.h>

#if WIN_MVC
#include <malloc.h>
#endif

#define STRICT_ALIGN_SIZE sizeof(double)

#define SpecialMalloc(sz) malloc((STD_SIZE) sz)
#define SpecialFree(ptr) free(ptr)

/********************************************/
/* InitializeMemory: Sets up memory tables. */
/********************************************/
void InitializeMemory(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,MEMORY_DATA,sizeof(struct memoryData),NULL);

   MemoryData(theEnv)->OutOfMemoryCallback = DefaultOutOfMemoryFunction;

#if (MEM_TABLE_SIZE > 0)
   MemoryData(theEnv)->MemoryTable = (struct memoryPtr **)
                 malloc((STD_SIZE) (sizeof(struct memoryPtr *) * MEM_TABLE_SIZE));

   if (MemoryData(theEnv)->MemoryTable == NULL)
     {
      PrintErrorID(theEnv,"MEMORY",1,true);
      WriteString(theEnv,STDERR,"Out of memory.\n");
      ExitRouter(theEnv,EXIT_FAILURE);
     }
   else
     {
      int i;

      for (i = 0; i < MEM_TABLE_SIZE; i++) MemoryData(theEnv)->MemoryTable[i] = NULL;
     }
#else // MEM_TABLE_SIZE == 0
      MemoryData(theEnv)->MemoryTable = NULL;
#endif
  }

/***************************************************/
/* genalloc: A generic memory allocation function. */
/***************************************************/
void *genalloc(
  Environment *theEnv,
  size_t size)
  {
   void *memPtr;

   memPtr = malloc(size);

   if (memPtr == NULL)
     {
      ReleaseMem(theEnv,(long long) ((size * 5 > 4096) ? size * 5 : 4096));
      memPtr = malloc(size);
      if (memPtr == NULL)
        {
         ReleaseMem(theEnv,-1);
         memPtr = malloc(size);
         while (memPtr == NULL)
           {
            if ((*MemoryData(theEnv)->OutOfMemoryCallback)(theEnv,size))
              return NULL;
            memPtr = malloc(size);
           }
        }
     }

   MemoryData(theEnv)->MemoryAmount += size;
   MemoryData(theEnv)->MemoryCalls++;

   return memPtr;
  }

/***********************************************/
/* DefaultOutOfMemoryFunction: Function called */
/*   when the KB runs out of memory.           */
/***********************************************/
bool DefaultOutOfMemoryFunction(
  Environment *theEnv,
  size_t size)
  {
#if MAC_XCD
#pragma unused(size)
#endif

   PrintErrorID(theEnv,"MEMORY",1,true);
   WriteString(theEnv,STDERR,"Out of memory.\n");
   ExitRouter(theEnv,EXIT_FAILURE);
   return true;
  }

/**********************************************************/
/* SetOutOfMemoryFunction: Allows the function which is   */
/*   called when the KB runs out of memory to be changed. */
/**********************************************************/
OutOfMemoryFunction *SetOutOfMemoryFunction(
  Environment *theEnv,
  OutOfMemoryFunction *functionPtr)
  {
   OutOfMemoryFunction *tmpPtr;

   tmpPtr = MemoryData(theEnv)->OutOfMemoryCallback;
   MemoryData(theEnv)->OutOfMemoryCallback = functionPtr;
   return tmpPtr;
  }

/****************************************************/
/* genfree: A generic memory deallocation function. */
/****************************************************/
void genfree(
  Environment *theEnv,
  void *waste,
  size_t size)
  {
   free(waste);

   MemoryData(theEnv)->MemoryAmount -= size;
   MemoryData(theEnv)->MemoryCalls--;
  }

/******************************************************/
/* genrealloc: Simple (i.e. dumb) version of realloc. */
/******************************************************/
void *genrealloc(
  Environment *theEnv,
  void *oldaddr,
  size_t oldsz,
  size_t newsz)
  {
   char *newaddr;
   unsigned i;
   size_t limit;

   newaddr = ((newsz != 0) ? (char *) gm2(theEnv,newsz) : NULL);

   if (oldaddr != NULL)
     {
      limit = (oldsz < newsz) ? oldsz : newsz;
      for (i = 0 ; i < limit ; i++)
        { newaddr[i] = ((char *) oldaddr)[i]; }
      for ( ; i < newsz; i++)
        { newaddr[i] = '\0'; }
      rm(theEnv,oldaddr,oldsz);
     }

   return((void *) newaddr);
  }

/********************************/
/* MemUsed: C access routine */
/*   for the mem-used command.  */
/********************************/
long long MemUsed(
  Environment *theEnv)
  {
   return MemoryData(theEnv)->MemoryAmount;
  }

/***********************************/
/* MemRequests: C access routine   */
/*   for the mem-requests command. */
/***********************************/
long long MemRequests(
  Environment *theEnv)
  {
   return MemoryData(theEnv)->MemoryCalls;
  }

/***************************************/
/* UpdateMemoryUsed: Allows the amount */
/*   of memory used to be updated.     */
/***************************************/
long long UpdateMemoryUsed(
  Environment *theEnv,
  long long value)
  {
   MemoryData(theEnv)->MemoryAmount += value;
   return MemoryData(theEnv)->MemoryAmount;
  }

/*******************************************/
/* UpdateMemoryRequests: Allows the number */
/*   of memory requests to be updated.     */
/*******************************************/
long long UpdateMemoryRequests(
  Environment *theEnv,
  long long value)
  {
   MemoryData(theEnv)->MemoryCalls += value;
   return MemoryData(theEnv)->MemoryCalls;
  }

/**********************************/
/* ReleaseMem: C access routine   */
/*   for the release-mem command. */
/**********************************/
long long ReleaseMem(
  Environment *theEnv,
  long long maximum)
  {
   struct memoryPtr *tmpPtr, *memPtr;
   unsigned int i;
   long long returns = 0;
   long long amount = 0;

   for (i = (MEM_TABLE_SIZE - 1) ; i >= sizeof(char *) ; i--)
     {
      YieldTime(theEnv);
      memPtr = MemoryData(theEnv)->MemoryTable[i];
      while (memPtr != NULL)
        {
         tmpPtr = memPtr->next;
         genfree(theEnv,memPtr,i);
         memPtr = tmpPtr;
         amount += i;
         returns++;
         if ((returns % 100) == 0)
           { YieldTime(theEnv); }
        }
      MemoryData(theEnv)->MemoryTable[i] = NULL;
      if ((amount > maximum) && (maximum > 0))
        { return amount; }
     }

   return amount;
  }

/*****************************************************/
/* gm1: Allocates memory and sets all bytes to zero. */
/*****************************************************/
void *gm1(
  Environment *theEnv,
  size_t size)
  {
   struct memoryPtr *memPtr;
   char *tmpPtr;
   size_t i;

   if ((size < sizeof(char *)) ||
       (size >= MEM_TABLE_SIZE))
     {
      tmpPtr = (char *) genalloc(theEnv,size);
      for (i = 0 ; i < size ; i++)
        { tmpPtr[i] = '\0'; }
      return((void *) tmpPtr);
     }

   memPtr = (struct memoryPtr *) MemoryData(theEnv)->MemoryTable[size];
   if (memPtr == NULL)
     {
      tmpPtr = (char *) genalloc(theEnv,size);
      for (i = 0 ; i < size ; i++)
        { tmpPtr[i] = '\0'; }
      return((void *) tmpPtr);
     }

   MemoryData(theEnv)->MemoryTable[size] = memPtr->next;

   tmpPtr = (char *) memPtr;
   for (i = 0 ; i < size ; i++)
     { tmpPtr[i] = '\0'; }

   return ((void *) tmpPtr);
  }

/*****************************************************/
/* gm2: Allocates memory and does not initialize it. */
/*****************************************************/
void *gm2(
  Environment *theEnv,
  size_t size)
  {
#if (MEM_TABLE_SIZE > 0)
   struct memoryPtr *memPtr;
#endif

   if (size < sizeof(char *))
     { return genalloc(theEnv,size); }

#if (MEM_TABLE_SIZE > 0)
   if (size >= MEM_TABLE_SIZE) return genalloc(theEnv,size);

   memPtr = (struct memoryPtr *) MemoryData(theEnv)->MemoryTable[size];
   if (memPtr == NULL)
     { return genalloc(theEnv,size); }

   MemoryData(theEnv)->MemoryTable[size] = memPtr->next;

   return ((void *) memPtr);
#else
   return genalloc(theEnv,size);
#endif
  }

/****************************************/
/* rm: Returns a block of memory to the */
/*   maintained pool of free memory.    */
/****************************************/
void rm(
  Environment *theEnv,
  void *str,
  size_t size)
  {
#if (MEM_TABLE_SIZE > 0)
   struct memoryPtr *memPtr;
#endif

   if (size == 0)
     {
      SystemError(theEnv,"MEMORY",1);
      ExitRouter(theEnv,EXIT_FAILURE);
      return;
     }

   if (size < sizeof(char *)) 
     {
      genfree(theEnv,str,size);
      return;
     }

#if (MEM_TABLE_SIZE > 0)
   if (size >= MEM_TABLE_SIZE)
     {
      genfree(theEnv,str,size);
      return;
     }

   memPtr = (struct memoryPtr *) str;
   memPtr->next = MemoryData(theEnv)->MemoryTable[size];
   MemoryData(theEnv)->MemoryTable[size] = memPtr;
#else
   genfree(theEnv,str,size);
#endif
  }

/***************************************************/
/* PoolSize: Returns number of bytes in free pool. */
/***************************************************/
unsigned long PoolSize(
  Environment *theEnv)
  {
   unsigned long cnt = 0;

#if (MEM_TABLE_SIZE > 0)
   int i;
   struct memoryPtr *memPtr;

   for (i = sizeof(char *) ; i < MEM_TABLE_SIZE ; i++)
     {
      memPtr = MemoryData(theEnv)->MemoryTable[i];
      while (memPtr != NULL)
        {
         cnt += (unsigned long) i;
         memPtr = memPtr->next;
        }
     }
#endif

   return(cnt);
  }

/***************************************************************/
/* ActualPoolSize : Returns number of bytes DOS requires to    */
/*   store the free pool.  This routine is functionally        */
/*   equivalent to pool_size on anything other than the IBM-PC */
/***************************************************************/
unsigned long ActualPoolSize(
  Environment *theEnv)
  {
   return(PoolSize(theEnv));
  }

/*****************************************/
/* SetConserveMemory: Allows the setting */
/*    of the memory conservation flag.   */
/*****************************************/
bool SetConserveMemory(
  Environment *theEnv,
  bool value)
  {
   bool ov;

   ov = MemoryData(theEnv)->ConserveMemory;
   MemoryData(theEnv)->ConserveMemory = value;
   return ov;
  }

/****************************************/
/* GetConserveMemory: Returns the value */
/*    of the memory conservation flag.  */
/****************************************/
bool GetConserveMemory(
  Environment *theEnv)
  {
   return MemoryData(theEnv)->ConserveMemory;
  }

/**************************/
/* genmemcpy:             */
/**************************/
void genmemcpy(
  char *dst,
  char *src,
  unsigned long size)
  {
   unsigned long i;

   for (i = 0L ; i < size ; i++)
     dst[i] = src[i];
  }
