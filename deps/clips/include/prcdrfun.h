   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  08/25/16            */
   /*                                                     */
   /*          PROCEDURAL FUNCTIONS HEADER FILE           */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*            Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Local variables set with the bind function     */
/*            persist until a reset/clear command is issued. */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Support for long long integers.                */
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
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#ifndef _H_prcdrfun

#pragma once

#define _H_prcdrfun

#include "evaluatn.h"

typedef struct loopCounterStack
  {
   long long loopCounter;
   struct loopCounterStack *nxt;
  } LOOP_COUNTER_STACK;

#define PRCDRFUN_DATA 13

struct procedureFunctionData
  {
   bool ReturnFlag;
   bool BreakFlag;
   LOOP_COUNTER_STACK *LoopCounterStack;
   UDFValue *BindList;
  };

#define ProcedureFunctionData(theEnv) ((struct procedureFunctionData *) GetEnvironmentData(theEnv,PRCDRFUN_DATA))

   void                           ProceduralFunctionDefinitions(Environment *);
   void                           WhileFunction(Environment *,UDFContext *,UDFValue *);
   void                           LoopForCountFunction(Environment *,UDFContext *,UDFValue *);
   void                           GetLoopCount(Environment *,UDFContext *,UDFValue *);
   void                           IfFunction(Environment *,UDFContext *,UDFValue *);
   void                           BindFunction(Environment *,UDFContext *,UDFValue *);
   void                           PrognFunction(Environment *,UDFContext *,UDFValue *);
   void                           ReturnFunction(Environment *,UDFContext *,UDFValue *);
   void                           BreakFunction(Environment *,UDFContext *,UDFValue *);
   void                           SwitchFunction(Environment *,UDFContext *,UDFValue *);
   bool                           GetBoundVariable(Environment *,UDFValue *,CLIPSLexeme *);
   void                           FlushBindList(Environment *,void *);

#endif /* _H_prcdrfun */






