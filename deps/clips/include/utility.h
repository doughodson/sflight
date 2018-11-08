   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  11/15/17            */
   /*                                                     */
   /*                 UTILITY HEADER FILE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides a set of utility functions useful to    */
/*   other modules. Primarily these are the functions for    */
/*   handling periodic garbage collection and appending      */
/*   string data.                                            */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Added CopyString, DeleteString,                */
/*            InsertInString,and EnlargeString functions.    */
/*                                                           */
/*            Used genstrncpy function instead of strncpy    */
/*            function.                                      */
/*                                                           */
/*            Support for typed EXTERNAL_ADDRESS_TYPE.       */
/*                                                           */
/*            Support for tracked memory (allows memory to   */
/*            be freed if CLIPS is exited while executing).  */
/*                                                           */
/*            Added UTF-8 routines.                          */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
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
/*            Callbacks must be environment aware.           */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Added GCBlockStart and GCBlockEnd functions    */
/*            for garbage collection blocks.                 */
/*                                                           */
/*            Added StringBuilder functions.                 */
/*                                                           */
/*************************************************************/

#ifndef _H_utility

#pragma once

#define _H_utility

#include <stdlib.h>

typedef struct callFunctionItem CallFunctionItem;
typedef struct callFunctionItemWithArg CallFunctionItemWithArg;
typedef struct boolCallFunctionItem BoolCallFunctionItem;
typedef struct voidCallFunctionItem VoidCallFunctionItem;

#include "evaluatn.h"
#include "moduldef.h"

typedef struct gcBlock GCBlock;
typedef struct stringBuilder StringBuilder;

struct voidCallFunctionItem
  {
   const char *name;
   VoidCallFunction *func;
   int priority;
   struct voidCallFunctionItem *next;
   void *context;
  };

struct boolCallFunctionItem
  {
   const char *name;
   BoolCallFunction *func;
   int priority;
   struct boolCallFunctionItem *next;
   void *context;
  };

struct callFunctionItemWithArg
  {
   const char *name;
   VoidCallFunctionWithArg *func;
   int priority;
   struct callFunctionItemWithArg *next;
   void *context;
  };

struct trackedMemory
  {
   void *theMemory;
   struct trackedMemory *next;
   struct trackedMemory *prev;
   size_t memSize;
  };

struct garbageFrame
  {
   bool dirty;
   struct garbageFrame *priorFrame;
   struct ephemeron *ephemeralSymbolList;
   struct ephemeron *ephemeralFloatList;
   struct ephemeron *ephemeralIntegerList;
   struct ephemeron *ephemeralBitMapList;
   struct ephemeron *ephemeralExternalAddressList;
   Multifield *ListOfMultifields;
   Multifield *LastMultifield;
  };

struct gcBlock
  {
   struct garbageFrame newGarbageFrame;
   struct garbageFrame *oldGarbageFrame;
   UDFValue *result;
  };

struct stringBuilder
  {
   Environment *sbEnv;
   char *contents;
   size_t bufferReset;
   size_t length;
   size_t bufferMaximum;
  };

#define UTILITY_DATA 55

struct utilityData
  {
   struct voidCallFunctionItem *ListOfCleanupFunctions;
   struct voidCallFunctionItem *ListOfPeriodicFunctions;
   bool PeriodicFunctionsEnabled;
   bool YieldFunctionEnabled;
   void (*YieldTimeFunction)(void);
   struct trackedMemory *trackList;
   struct garbageFrame MasterGarbageFrame;
   struct garbageFrame *CurrentGarbageFrame;
  };

#define UtilityData(theEnv) ((struct utilityData *) GetEnvironmentData(theEnv,UTILITY_DATA))

  /* Is c the start of a utf8 sequence? */
#define IsUTF8Start(ch) (((ch) & 0xC0) != 0x80)
#define IsUTF8MultiByteStart(ch) ((((unsigned char) ch) >= 0xC0) && (((unsigned char) ch) <= 0xF7))
#define IsUTF8MultiByteContinuation(ch) ((((unsigned char) ch) >= 0x80) && (((unsigned char) ch) <= 0xBF))

   void                           InitializeUtilityData(Environment *);
   bool                           AddCleanupFunction(Environment *,const char *,VoidCallFunction *,int,void *);
   bool                           AddPeriodicFunction(Environment *,const char *,VoidCallFunction *,int,void *);
   bool                           RemoveCleanupFunction(Environment *,const char *);
   bool                           RemovePeriodicFunction(Environment *,const char *);
   char                          *CopyString(Environment *,const char *);
   void                           DeleteString(Environment *,char *);
   const char                    *AppendStrings(Environment *,const char *,const char *);
   const char                    *StringPrintForm(Environment *,const char *);
   char                          *AppendToString(Environment *,const char *,char *,size_t *,size_t *);
   char                          *InsertInString(Environment *,const char *,size_t,char *,size_t *,size_t *);
   char                          *AppendNToString(Environment *,const char *,char *,size_t,size_t *,size_t *);
   char                          *EnlargeString(Environment *,size_t,char *,size_t *,size_t *);
   char                          *ExpandStringWithChar(Environment *,int,char *,size_t *,size_t *,size_t);
   VoidCallFunctionItem          *AddVoidFunctionToCallList(Environment *,const char *,int,VoidCallFunction *,
                                                            VoidCallFunctionItem *,void *);
   BoolCallFunctionItem          *AddBoolFunctionToCallList(Environment *,const char *,int,BoolCallFunction *,
                                                            BoolCallFunctionItem *,void *);
   VoidCallFunctionItem          *RemoveVoidFunctionFromCallList(Environment *,const char *,
                                                                 VoidCallFunctionItem *,bool *);
   BoolCallFunctionItem          *RemoveBoolFunctionFromCallList(Environment *,const char *,
                                                                 BoolCallFunctionItem *,bool *);
   void                           DeallocateVoidCallList(Environment *,VoidCallFunctionItem *);
   void                           DeallocateBoolCallList(Environment *,BoolCallFunctionItem *);
   CallFunctionItemWithArg       *AddFunctionToCallListWithArg(Environment *,const char *,int,
                                                               VoidCallFunctionWithArg *,
                                                               CallFunctionItemWithArg *,void *);
   CallFunctionItemWithArg       *RemoveFunctionFromCallListWithArg(Environment *,const char *,
                                                                            struct callFunctionItemWithArg *,
                                                                            bool *);
   void                           DeallocateCallListWithArg(Environment *,struct callFunctionItemWithArg *);
   VoidCallFunctionItem          *GetVoidFunctionFromCallList(Environment *,const char *,VoidCallFunctionItem *);
   BoolCallFunctionItem          *GetBoolFunctionFromCallList(Environment *,const char *,BoolCallFunctionItem *);
   size_t                         ItemHashValue(Environment *,unsigned short,void *,size_t);
   void                           YieldTime(Environment *);
   bool                           EnablePeriodicFunctions(Environment *,bool);
   bool                           EnableYieldFunction(Environment *,bool);
   struct trackedMemory          *AddTrackedMemory(Environment *,void *,size_t);
   void                           RemoveTrackedMemory(Environment *,struct trackedMemory *);
   void                           UTF8Increment(const char *,size_t *);
   size_t                         UTF8Offset(const char *,size_t);
   size_t                         UTF8Length(const char *);
   size_t                         UTF8CharNum(const char *,size_t);
   void                           RestorePriorGarbageFrame(Environment *,struct garbageFrame *,struct garbageFrame *,UDFValue *);
   void                           CallCleanupFunctions(Environment *);
   void                           CallPeriodicTasks(Environment *);
   void                           CleanCurrentGarbageFrame(Environment *,UDFValue *);
   void                           GCBlockStart(Environment *,GCBlock *);
   void                           GCBlockEnd(Environment *,GCBlock *);
   void                           GCBlockEndUDF(Environment *,GCBlock *,UDFValue *);
   StringBuilder                 *CreateStringBuilder(Environment *,size_t);
   void                           SBDispose(StringBuilder *);
   void                           SBAppend(StringBuilder *,const char *);
   void                           SBAppendInteger(StringBuilder *,long long);
   void                           SBAppendFloat(StringBuilder *,double);
   void                           SBAddChar(StringBuilder *,int);
   void                           SBReset(StringBuilder *);
   char                          *SBCopy(StringBuilder *);
   void                          *GetPeriodicFunctionContext(Environment *,const char *);

#endif /* _H_utility */




