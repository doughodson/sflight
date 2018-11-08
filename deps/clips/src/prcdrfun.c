   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/18/16             */
   /*                                                     */
   /*             PROCEDURAL FUNCTIONS MODULE             */
   /*******************************************************/

/*************************************************************/
/* Purpose: Contains the code for several procedural         */
/*   functions including if, while, loop-for-count, bind,    */
/*   progn, return, break, and switch                        */
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
/*      6.40: Added Env prefix to GetHaltExecution and       */
/*            SetHaltExecution functions.                    */
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
/*            Added GCBlockStart and GCBlockEnd functions    */
/*            for garbage collection blocks.                 */
/*                                                           */
/*            Eval support for run time and bload only.      */
/*                                                           */
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#include "argacces.h"
#include "constrnt.h"
#include "cstrnchk.h"
#include "cstrnops.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "memalloc.h"
#include "multifld.h"
#include "prcdrpsr.h"
#include "router.h"
#include "scanner.h"
#include "utility.h"

#include "prcdrfun.h"

#if DEFGLOBAL_CONSTRUCT
#include "globldef.h"
#endif

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    DeallocateProceduralFunctionData(Environment *);

/**********************************************/
/* ProceduralFunctionDefinitions: Initializes */
/*   the procedural functions.                */
/**********************************************/
void ProceduralFunctionDefinitions(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,PRCDRFUN_DATA,sizeof(struct procedureFunctionData),DeallocateProceduralFunctionData);

#if ! RUN_TIME
   AddUDF(theEnv,"if","*",0,UNBOUNDED,NULL,IfFunction,"IfFunction",NULL);
   AddUDF(theEnv,"while","*",0,UNBOUNDED,NULL,WhileFunction,"WhileFunction",NULL);
   AddUDF(theEnv,"loop-for-count","*",0,UNBOUNDED,NULL,LoopForCountFunction,"LoopForCountFunction",NULL);
   AddUDF(theEnv,"(get-loop-count)","l",1,1,NULL,GetLoopCount,"GetLoopCount",NULL);
   AddUDF(theEnv,"bind","*",0,UNBOUNDED,NULL,BindFunction,"BindFunction",NULL);
   AddUDF(theEnv,"progn","*",0,UNBOUNDED,NULL,PrognFunction,"PrognFunction",NULL);
   AddUDF(theEnv,"return","*",0,UNBOUNDED,NULL,ReturnFunction,"ReturnFunction",NULL);
   AddUDF(theEnv,"break","v",0,0,NULL,BreakFunction,"BreakFunction",NULL);
   AddUDF(theEnv,"switch","*",0,UNBOUNDED,NULL,SwitchFunction,"SwitchFunction",NULL);
#endif

   ProceduralFunctionParsers(theEnv);

#if ! RUN_TIME
   FuncSeqOvlFlags(theEnv,"progn",false,false);
   FuncSeqOvlFlags(theEnv,"if",false,false);
   FuncSeqOvlFlags(theEnv,"while",false,false);
   FuncSeqOvlFlags(theEnv,"loop-for-count",false,false);
   FuncSeqOvlFlags(theEnv,"return",false,false);
   FuncSeqOvlFlags(theEnv,"switch",false,false);
#endif

   AddResetFunction(theEnv,"bind",FlushBindList,0,NULL);
   AddClearFunction(theEnv,"bind",FlushBindList,0,NULL);
  }

/*************************************************************/
/* DeallocateProceduralFunctionData: Deallocates environment */
/*    data for procedural functions.                         */
/*************************************************************/
static void DeallocateProceduralFunctionData(
  Environment *theEnv)
  {
   UDFValue *nextPtr, *garbagePtr;

   garbagePtr = ProcedureFunctionData(theEnv)->BindList;

   while (garbagePtr != NULL)
     {
      nextPtr = garbagePtr->next;
      rtn_struct(theEnv,udfValue,garbagePtr);
      garbagePtr = nextPtr;
     }
  }

/***************************************/
/* WhileFunction: H/L access routine   */
/*   for the while function.           */
/***************************************/
void WhileFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theResult;
   GCBlock gcb;

   /*====================================================*/
   /* Evaluate the body of the while loop as long as the */
   /* while condition evaluates to a non-FALSE value.    */
   /*====================================================*/

   GCBlockStart(theEnv,&gcb);

   UDFNthArgument(context,1,ANY_TYPE_BITS,&theResult);
   while ((theResult.value != FalseSymbol(theEnv)) &&
          (EvaluationData(theEnv)->HaltExecution != true))
     {
      if ((ProcedureFunctionData(theEnv)->BreakFlag == true) || (ProcedureFunctionData(theEnv)->ReturnFlag == true))
        break;

      UDFNthArgument(context,2,ANY_TYPE_BITS,&theResult);

      if ((ProcedureFunctionData(theEnv)->BreakFlag == true) || (ProcedureFunctionData(theEnv)->ReturnFlag == true))
        break;

      CleanCurrentGarbageFrame(theEnv,NULL);
      CallPeriodicTasks(theEnv);

      UDFNthArgument(context,1,ANY_TYPE_BITS,&theResult);
     }

   /*=====================================================*/
   /* Reset the break flag. The return flag is not reset  */
   /* because the while loop is probably contained within */
   /* a deffunction or RHS of a rule which needs to be    */
   /* returned from as well.                              */
   /*=====================================================*/

   ProcedureFunctionData(theEnv)->BreakFlag = false;

   /*====================================================*/
   /* If the return command was issued, then return that */
   /* value, otherwise return the symbol FALSE.          */
   /*====================================================*/

   if (ProcedureFunctionData(theEnv)->ReturnFlag == true)
     {
      returnValue->value = theResult.value;
      returnValue->begin = theResult.begin;
      returnValue->range = theResult.range;
     }
   else
     {
      returnValue->value = FalseSymbol(theEnv);
     }

   GCBlockEndUDF(theEnv,&gcb,returnValue);
   CallPeriodicTasks(theEnv);
  }

/********************************************/
/* LoopForCountFunction: H/L access routine */
/*   for the loop-for-count function.       */
/********************************************/
void LoopForCountFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *loopResult)
  {
   UDFValue theArg;
   long long iterationEnd;
   LOOP_COUNTER_STACK *tmpCounter;
   GCBlock gcb;

   tmpCounter = get_struct(theEnv,loopCounterStack);
   tmpCounter->loopCounter = 0L;
   tmpCounter->nxt = ProcedureFunctionData(theEnv)->LoopCounterStack;
   ProcedureFunctionData(theEnv)->LoopCounterStack = tmpCounter;

   if (! UDFNthArgument(context,1,INTEGER_BIT,&theArg))
     {
      loopResult->value = FalseSymbol(theEnv);
      ProcedureFunctionData(theEnv)->LoopCounterStack = tmpCounter->nxt;
      rtn_struct(theEnv,loopCounterStack,tmpCounter);
      return;
     }
   tmpCounter->loopCounter = theArg.integerValue->contents;
   if (! UDFNthArgument(context,2,INTEGER_BIT,&theArg))
     {
      loopResult->value = FalseSymbol(theEnv);
      ProcedureFunctionData(theEnv)->LoopCounterStack = tmpCounter->nxt;
      rtn_struct(theEnv,loopCounterStack,tmpCounter);
      return;
     }

   GCBlockStart(theEnv,&gcb);

   iterationEnd = theArg.integerValue->contents;
   while ((tmpCounter->loopCounter <= iterationEnd) &&
          (EvaluationData(theEnv)->HaltExecution != true))
     {
      if ((ProcedureFunctionData(theEnv)->BreakFlag == true) || (ProcedureFunctionData(theEnv)->ReturnFlag == true))
        break;

      UDFNthArgument(context,3,ANY_TYPE_BITS,&theArg);

      if ((ProcedureFunctionData(theEnv)->BreakFlag == true) || (ProcedureFunctionData(theEnv)->ReturnFlag == true))
        break;

      CleanCurrentGarbageFrame(theEnv,NULL);
      CallPeriodicTasks(theEnv);

      tmpCounter->loopCounter++;
     }

   ProcedureFunctionData(theEnv)->BreakFlag = false;
   if (ProcedureFunctionData(theEnv)->ReturnFlag == true)
     {
      loopResult->value = theArg.value;
      loopResult->begin = theArg.begin;
      loopResult->range = theArg.range;
     }
   else
     {
      loopResult->value = FalseSymbol(theEnv);
     }
   ProcedureFunctionData(theEnv)->LoopCounterStack = tmpCounter->nxt;
   rtn_struct(theEnv,loopCounterStack,tmpCounter);

   GCBlockEndUDF(theEnv,&gcb,loopResult);
   CallPeriodicTasks(theEnv);
  }

/*****************/
/* GetLoopCount: */
/*****************/
void GetLoopCount(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   long long depth;
   UDFValue theArg;
   LOOP_COUNTER_STACK *tmpCounter;

   if (! UDFFirstArgument(context,INTEGER_BIT,&theArg))
     { return; }
   depth = theArg.integerValue->contents;
   tmpCounter = ProcedureFunctionData(theEnv)->LoopCounterStack;
   while (depth > 0)
     {
      tmpCounter = tmpCounter->nxt;
      depth--;
     }

   returnValue->integerValue = CreateInteger(theEnv,tmpCounter->loopCounter);
  }

/************************************/
/* IfFunction: H/L access routine   */
/*   for the if function.           */
/************************************/
void IfFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   unsigned int numArgs;

   /*=========================*/
   /* Evaluate the condition. */
   /*=========================*/

   if (! UDFNthArgument(context,1,ANY_TYPE_BITS,returnValue))
     {
      returnValue->value = FalseSymbol(theEnv);
      return;
     }

   if ((ProcedureFunctionData(theEnv)->BreakFlag == true) ||
       (ProcedureFunctionData(theEnv)->ReturnFlag == true))
     {
      returnValue->value = FalseSymbol(theEnv);
      return;
     }

   /*=========================================*/
   /* If the condition evaluated to FALSE and */
   /* an "else" portion exists, evaluate it   */
   /* and return the value.                   */
   /*=========================================*/

   numArgs = UDFArgumentCount(context);
   if ((returnValue->value == FalseSymbol(theEnv)) &&
       (numArgs == 3))
     {
      UDFNthArgument(context,3,ANY_TYPE_BITS,returnValue);
      return;
     }

   /*===================================================*/
   /* Otherwise if the symbol evaluated to a non-FALSE  */
   /* value, evaluate the "then" portion and return it. */
   /*===================================================*/

   else if (returnValue->value != FalseSymbol(theEnv))
     {
      UDFNthArgument(context,2,ANY_TYPE_BITS,returnValue);
      return;
     }

   /*=========================================*/
   /* Return FALSE if the condition evaluated */
   /* to FALSE and there is no "else" portion */
   /* of the if statement.                    */
   /*=========================================*/

   returnValue->value = FalseSymbol(theEnv);
  }

/**************************************/
/* BindFunction: H/L access routine   */
/*   for the bind function.           */
/**************************************/
void BindFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue *theBind, *lastBind;
   bool found = false,
       unbindVar = false;
   CLIPSLexeme *variableName = NULL;
#if DEFGLOBAL_CONSTRUCT
   Defglobal *theGlobal = NULL;
#endif

   /*===============================================*/
   /* Determine the name of the variable to be set. */
   /*===============================================*/

#if DEFGLOBAL_CONSTRUCT
   if (GetFirstArgument()->type == DEFGLOBAL_PTR)
     { theGlobal = (Defglobal *) GetFirstArgument()->value; }
   else
#endif
     {
      EvaluateExpression(theEnv,GetFirstArgument(),returnValue);
      variableName = returnValue->lexemeValue;
     }

   /*===========================================*/
   /* Determine the new value for the variable. */
   /*===========================================*/

   if (GetFirstArgument()->nextArg == NULL)
     { unbindVar = true; }
   else if (GetFirstArgument()->nextArg->nextArg == NULL)
     { EvaluateExpression(theEnv,GetFirstArgument()->nextArg,returnValue); }
   else
     { StoreInMultifield(theEnv,returnValue,GetFirstArgument()->nextArg,true); }

   /*==================================*/
   /* Bind a defglobal if appropriate. */
   /*==================================*/

#if DEFGLOBAL_CONSTRUCT
   if (theGlobal != NULL)
     {
      QSetDefglobalValue(theEnv,theGlobal,returnValue,unbindVar);
      return;
     }
#endif

   /*===============================================*/
   /* Search for the variable in the list of binds. */
   /*===============================================*/

   theBind = ProcedureFunctionData(theEnv)->BindList;
   lastBind = NULL;

   while ((theBind != NULL) && (found == false))
     {
      if (theBind->supplementalInfo == (void *) variableName)
        { found = true; }
      else
        {
         lastBind = theBind;
         theBind = theBind->next;
        }
     }

   /*========================================================*/
   /* If variable was not in the list of binds, then add it. */
   /* Make sure that this operation preserves the bind list  */
   /* as a stack.                                            */
   /*========================================================*/

   if (found == false)
     {
      if (unbindVar == false)
        {
         theBind = get_struct(theEnv,udfValue);
         theBind->supplementalInfo = (void *) variableName;
         IncrementLexemeCount(variableName);
         theBind->next = NULL;
         if (lastBind == NULL)
           { ProcedureFunctionData(theEnv)->BindList = theBind; }
         else
           { lastBind->next = theBind; }
        }
      else
        {
         returnValue->value = FalseSymbol(theEnv);
         return;
        }
     }
   else
     { ReleaseUDFV(theEnv,theBind); }

   /*================================*/
   /* Set the value of the variable. */
   /*================================*/

   if (unbindVar == false)
     {
      theBind->value = returnValue->value;
      theBind->begin = returnValue->begin;
      theBind->range = returnValue->range;
      RetainUDFV(theEnv,returnValue);
     }
   else
     {
      if (lastBind == NULL) ProcedureFunctionData(theEnv)->BindList = theBind->next;
      else lastBind->next = theBind->next;
      ReleaseLexeme(theEnv,(CLIPSLexeme *) theBind->supplementalInfo);
      rtn_struct(theEnv,udfValue,theBind);
      returnValue->value = FalseSymbol(theEnv);
     }
  }

/*******************************************/
/* GetBoundVariable: Searches the BindList */
/*   for a specified variable.             */
/*******************************************/
bool GetBoundVariable(
  Environment *theEnv,
  UDFValue *vPtr,
  CLIPSLexeme *varName)
  {
   UDFValue *bindPtr;

   for (bindPtr = ProcedureFunctionData(theEnv)->BindList; bindPtr != NULL; bindPtr = bindPtr->next)
     {
      if (bindPtr->supplementalInfo == (void *) varName)
        {
         vPtr->value = bindPtr->value;
         vPtr->begin = bindPtr->begin;
         vPtr->range = bindPtr->range;
         return true;
        }
     }

   return false;
  }

/*************************************************/
/* FlushBindList: Removes all variables from the */
/*   list of currently bound local variables.    */
/*************************************************/
void FlushBindList(
  Environment *theEnv,
  void *context)
  {
   ReturnValues(theEnv,ProcedureFunctionData(theEnv)->BindList,true);
   ProcedureFunctionData(theEnv)->BindList = NULL;
  }

/***************************************/
/* PrognFunction: H/L access routine   */
/*   for the progn function.           */
/***************************************/
void PrognFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   struct expr *argPtr;

   argPtr = EvaluationData(theEnv)->CurrentExpression->argList;

   if (argPtr == NULL)
     {
      returnValue->value = FalseSymbol(theEnv);
      return;
     }

   while ((argPtr != NULL) && (GetHaltExecution(theEnv) != true))
     {
      EvaluateExpression(theEnv,argPtr,returnValue);

      if ((ProcedureFunctionData(theEnv)->BreakFlag == true) || (ProcedureFunctionData(theEnv)->ReturnFlag == true))
        break;
      argPtr = argPtr->nextArg;
     }

   if (GetHaltExecution(theEnv) == true)
     {
      returnValue->value = FalseSymbol(theEnv);
      return;
     }

   return;
  }

/***************************************************************/
/* ReturnFunction: H/L access routine for the return function. */
/***************************************************************/
void ReturnFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   if (! UDFHasNextArgument(context))
     {
      returnValue->voidValue = VoidConstant(theEnv);
     }
   else
     { UDFNextArgument(context,ANY_TYPE_BITS,returnValue); }
   ProcedureFunctionData(theEnv)->ReturnFlag = true;
  }

/***************************************************************/
/* BreakFunction: H/L access routine for the break function.   */
/***************************************************************/
void BreakFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   ProcedureFunctionData(theEnv)->BreakFlag = true;
  }

/*****************************************************************/
/* SwitchFunction: H/L access routine for the switch function.   */
/*****************************************************************/
void SwitchFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue switch_val,case_val;
   Expression *theExp;

   returnValue->lexemeValue = FalseSymbol(theEnv);

   /* ==========================
      Get the value to switch on
      ========================== */
   EvaluateExpression(theEnv,GetFirstArgument(),&switch_val);
   if (EvaluationData(theEnv)->EvaluationError)
     return;
   for (theExp = GetFirstArgument()->nextArg ; theExp != NULL ; theExp = theExp->nextArg->nextArg)
     {
      /* =================================================
         VOID_TYPE is the default case (if any) for the switch
         ================================================= */
      if (theExp->type == VOID_TYPE)
        {
         EvaluateExpression(theEnv,theExp->nextArg,returnValue);
         return;
        }

      /* ====================================================
         If the case matches, evaluate the actions and return
         ==================================================== */
      EvaluateExpression(theEnv,theExp,&case_val);
      if (EvaluationData(theEnv)->EvaluationError)
        return;
      if (switch_val.header->type == case_val.header->type)
        {
         if ((case_val.header->type == MULTIFIELD_TYPE) ? MultifieldDOsEqual(&switch_val,&case_val) :
             (switch_val.value == case_val.value))
           {
            EvaluateExpression(theEnv,theExp->nextArg,returnValue);
            return;
           }
        }
     }
  }





