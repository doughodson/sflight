   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  08/25/16             */
   /*                                                     */
   /*              DEFGLOBAL COMMANDS MODULE              */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides the show-defglobals, set-reset-globals, */
/*   and get-reset-globals commands.                         */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
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
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if DEFGLOBAL_CONSTRUCT

#include "argacces.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "globldef.h"
#include "prntutil.h"
#include "router.h"

#include "globlcom.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if DEBUGGING_FUNCTIONS
   static void                       PrintDefglobalValueForm(Environment *,const char *,Defglobal *);
#endif

/************************************************************/
/* DefglobalCommandDefinitions: Defines defglobal commands. */
/************************************************************/
void DefglobalCommandDefinitions(
  Environment *theEnv)
  {
#if ! RUN_TIME
   AddUDF(theEnv,"set-reset-globals","b",1,1,NULL,SetResetGlobalsCommand,"SetResetGlobalsCommand",NULL);
   AddUDF(theEnv,"get-reset-globals","b",0,0,NULL,GetResetGlobalsCommand,"GetResetGlobalsCommand",NULL);

#if DEBUGGING_FUNCTIONS
   AddUDF(theEnv,"show-defglobals","v",0,1,"y",ShowDefglobalsCommand,"ShowDefglobalsCommand",NULL);
#endif

#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }

/************************************************/
/* SetResetGlobalsCommand: H/L access routine   */
/*   for the get-reset-globals command.         */
/************************************************/
void SetResetGlobalsCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   bool oldValue;
   UDFValue theArg;

   /*===========================================*/
   /* Remember the old value of this attribute. */
   /*===========================================*/

   oldValue = GetResetGlobals(theEnv);

   /*===========================================*/
   /* Determine the new value of the attribute. */
   /*===========================================*/

   if (! UDFFirstArgument(context,ANY_TYPE_BITS,&theArg))
     { return; }

   if (theArg.value == FalseSymbol(theEnv))
     { SetResetGlobals(theEnv,false); }
   else
     { SetResetGlobals(theEnv,true); }

   /*========================================*/
   /* Return the old value of the attribute. */
   /*========================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,oldValue);
  }

/****************************************/
/* SetResetGlobals: C access routine */
/*   for the set-reset-globals command. */
/****************************************/
bool SetResetGlobals(
  Environment *theEnv,
  bool value)
  {
   bool ov;

   ov = DefglobalData(theEnv)->ResetGlobals;
   DefglobalData(theEnv)->ResetGlobals = value;
   return(ov);
  }

/************************************************/
/* GetResetGlobalsCommand: H/L access routine   */
/*   for the get-reset-globals command.         */
/************************************************/
void GetResetGlobalsCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   returnValue->lexemeValue = CreateBoolean(theEnv,GetResetGlobals(theEnv));
  }

/****************************************/
/* GetResetGlobals: C access routine    */
/*   for the get-reset-globals command. */
/****************************************/
bool GetResetGlobals(
  Environment *theEnv)
  {
   return(DefglobalData(theEnv)->ResetGlobals);
  }

#if DEBUGGING_FUNCTIONS

/***********************************************/
/* ShowDefglobalsCommand: H/L access routine   */
/*   for the show-defglobals command.          */
/***********************************************/
void ShowDefglobalsCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Defmodule *theModule;
   unsigned int numArgs;
   bool error;

   numArgs = UDFArgumentCount(context);

   if (numArgs == 1)
     {
      theModule = GetModuleName(context,1,&error);
      if (error) return;
     }
   else
     { theModule = GetCurrentModule(theEnv); }

   ShowDefglobals(theEnv,STDOUT,theModule);
  }

/**************************************/
/* ShowDefglobals: C access routine   */
/*   for the show-defglobals command. */
/**************************************/
void ShowDefglobals(
  Environment *theEnv,
  const char *logicalName,
  Defmodule *theModule)
  {
   ConstructHeader *constructPtr;
   bool allModules = false;
   struct defmoduleItemHeader *theModuleItem;

   /*=======================================*/
   /* If the module specified is NULL, then */
   /* list all constructs in all modules.   */
   /*=======================================*/

   if (theModule == NULL)
     {
      theModule = GetNextDefmodule(theEnv,NULL);
      allModules = true;
     }

   /*======================================================*/
   /* Print out the constructs in the specified module(s). */
   /*======================================================*/

   for (;
        theModule != NULL;
        theModule = GetNextDefmodule(theEnv,theModule))
     {
      /*===========================================*/
      /* Print the module name before every group  */
      /* of defglobals listed if we're listing the */
      /* defglobals from every module.             */
      /*===========================================*/

      if (allModules)
        {
         WriteString(theEnv,logicalName,DefmoduleName(theModule));
         WriteString(theEnv,logicalName,":\n");
        }

      /*=====================================*/
      /* Print every defglobal in the module */
      /* currently being examined.           */
      /*=====================================*/

      theModuleItem = (struct defmoduleItemHeader *) GetModuleItem(theEnv,theModule,DefglobalData(theEnv)->DefglobalModuleIndex);

      for (constructPtr = theModuleItem->firstItem;
           constructPtr != NULL;
           constructPtr = constructPtr->next)
        {
         if (EvaluationData(theEnv)->HaltExecution == true) return;

         if (allModules) WriteString(theEnv,logicalName,"   ");
         PrintDefglobalValueForm(theEnv,logicalName,(Defglobal *) constructPtr);
         WriteString(theEnv,logicalName,"\n");
        }

      /*===================================*/
      /* If we're only listing the globals */
      /* for one module, then return.      */
      /*===================================*/

      if (! allModules) return;
     }
  }

/*****************************************************/
/* PrintDefglobalValueForm: Prints the value form of */
/*   a defglobal (the current value). For example,   */
/*   ?*x* = 3                                        */
/*****************************************************/
static void PrintDefglobalValueForm(
  Environment *theEnv,
  const char *logicalName,
  Defglobal *theGlobal)
  {
   WriteString(theEnv,logicalName,"?*");
   WriteString(theEnv,logicalName,theGlobal->header.name->contents);
   WriteString(theEnv,logicalName,"* = ");
   WriteCLIPSValue(theEnv,logicalName,&theGlobal->current);
  }

#endif /* DEBUGGING_FUNCTIONS */

#endif /* DEFGLOBAL_CONSTRUCT */

