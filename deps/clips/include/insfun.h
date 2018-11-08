   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  11/18/17            */
   /*                                                     */
   /*               INSTANCE FUNCTIONS MODULE             */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*            Changed name of variable log to logName        */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*            Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Link error occurs for the SlotExistError       */
/*            function when OBJECT_SYSTEM is set to 0 in     */
/*            setup.h. DR0865                                */
/*                                                           */
/*            Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Moved EvaluateAndStoreInDataObject to          */
/*            evaluatn.c                                     */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Fixed slot override default ?NONE bug.         */
/*                                                           */
/*      6.31: Fix for compilation with -std=c89.             */
/*                                                           */
/*      6.40: Removed LOCALE definition.                     */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#ifndef _H_insfun

#pragma once

#define _H_insfun

#include "entities.h"
#include "object.h"

typedef struct igarbage
  {
   Instance *ins;
   struct igarbage *nxt;
  } IGARBAGE;

#define INSTANCE_TABLE_HASH_SIZE 8191
#define InstanceSizeHeuristic(ins)      sizeof(Instance)

   void                           RetainInstance(Instance *);
   void                           ReleaseInstance(Instance *);
   void                           IncrementInstanceCallback(Environment *,Instance *);
   void                           DecrementInstanceCallback(Environment *,Instance *);
   void                           InitializeInstanceTable(Environment *);
   void                           CleanupInstances(Environment *,void *);
   unsigned                       HashInstance(CLIPSLexeme *);
   void                           DestroyAllInstances(Environment *,void *);
   void                           RemoveInstanceData(Environment *,Instance *);
   Instance                      *FindInstanceBySymbol(Environment *,CLIPSLexeme *);
   Instance                      *FindInstanceInModule(Environment *,CLIPSLexeme *,Defmodule *,
                                                       Defmodule *,bool);
   InstanceSlot                  *FindInstanceSlot(Environment *,Instance *,CLIPSLexeme *);
   int                            FindInstanceTemplateSlot(Environment *,Defclass *,CLIPSLexeme *);
   PutSlotError                   PutSlotValue(Environment *,Instance *,InstanceSlot *,UDFValue *,UDFValue *,const char *);
   PutSlotError                   DirectPutSlotValue(Environment *,Instance *,InstanceSlot *,UDFValue *,UDFValue *);
   PutSlotError                   ValidSlotValue(Environment *,UDFValue *,SlotDescriptor *,Instance *,const char *);
   Instance                      *CheckInstance(UDFContext *);
   void                           NoInstanceError(Environment *,const char *,const char *);
   void                           StaleInstanceAddress(Environment *,const char *,int);
   bool                           GetInstancesChanged(Environment *);
   void                           SetInstancesChanged(Environment *,bool);
   void                           PrintSlot(Environment *,const char *,SlotDescriptor *,Instance *,const char *);
   void                           PrintInstanceNameAndClass(Environment *,const char *,Instance *,bool);
   void                           PrintInstanceName(Environment *,const char *,Instance *);
   void                           PrintInstanceLongForm(Environment *,const char *,Instance *);
#if DEFRULE_CONSTRUCT && OBJECT_SYSTEM
   void                           DecrementObjectBasisCount(Environment *,Instance *);
   void                           IncrementObjectBasisCount(Environment *,Instance *);
   void                           MatchObjectFunction(Environment *,Instance *);
   bool                           NetworkSynchronized(Environment *,Instance *);
   bool                           InstanceIsDeleted(Environment *,Instance *);
#endif

#endif /* _H_insfun */







