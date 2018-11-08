   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  09/20/17             */
   /*                                                     */
   /*               ARGUMENT ACCESS MODULE                */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides access routines for accessing arguments */
/*   passed to user or system functions defined using the    */
/*   DefineFunction protocol.                                */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Added IllegalLogicalNameMessage function.      */
/*                                                           */
/*      6.30: Support for long long integers.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Support for fact-address arguments.            */
/*                                                           */
/*      6.31: Modified the GetFactOrInstanceArgument         */
/*            function so that error messages are now        */
/*            generated when the timetag, dependencies, and  */
/*            dependents functions are given a retracted     */
/*            fact.                                          */
/*                                                           */
/*      6.40: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*            Added Env prefix to GetHaltExecution and       */
/*            SetHaltExecution functions.                    */
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

#include "setup.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cstrnchk.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "factmngr.h"
#include "insfun.h"
#include "prntutil.h"
#include "router.h"
#include "sysdep.h"

#include "argacces.h"

/*********************************************************************/
/* GetLogicalName: Retrieves the nth argument passed to the function */
/*   call currently being evaluated and determines if it is a valid  */
/*   logical name. If valid, the logical name is returned, otherwise */
/*   NULL is returned.                                               */
/*********************************************************************/
const char *GetLogicalName(
  UDFContext *context,
  const char *defaultLogicalName)
  {
   Environment *theEnv = context->environment;
   const char *logicalName;
   UDFValue theArg;

   if (! UDFNextArgument(context,ANY_TYPE_BITS,&theArg))
     { return NULL; }

   if (CVIsType(&theArg,LEXEME_BITS) ||
       CVIsType(&theArg,INSTANCE_NAME_BIT))
     {
      logicalName = theArg.lexemeValue->contents;
      if ((strcmp(logicalName,"t") == 0) || (strcmp(logicalName,"T") == 0))
        { logicalName = defaultLogicalName; }
     }
   else if (CVIsType(&theArg,FLOAT_BIT))
     {
      logicalName = CreateSymbol(theEnv,FloatToString(theEnv,theArg.floatValue->contents))->contents;
     }
   else if (CVIsType(&theArg,INTEGER_BIT))
     {
      logicalName = CreateSymbol(theEnv,LongIntegerToString(theEnv,theArg.integerValue->contents))->contents;
     }
   else
     { logicalName = NULL; }

   return(logicalName);
  }

/************************************************************/
/* GetFileName: Retrieves the nth argument passed to the    */
/*   function call currently being evaluated and determines */
/*   if it is a valid file name. If valid, the file name is */
/*   returned, otherwise NULL is returned.                  */
/************************************************************/
const char *GetFileName(
  UDFContext *context)
  {
   UDFValue theArg;

   if (! UDFNextArgument(context,LEXEME_BITS,&theArg))
     { return NULL; }

   return theArg.lexemeValue->contents;
  }

/******************************************************************/
/* OpenErrorMessage: Generalized error message for opening files. */
/******************************************************************/
void OpenErrorMessage(
  Environment *theEnv,
  const char *functionName,
  const char *fileName)
  {
   PrintErrorID(theEnv,"ARGACCES",3,false);
   WriteString(theEnv,STDERR,"Function '");
   WriteString(theEnv,STDERR,functionName);
   WriteString(theEnv,STDERR,"' was unable to open file '");
   WriteString(theEnv,STDERR,fileName);
   WriteString(theEnv,STDERR,"'.\n");
  }

/************************************************************/
/* GetModuleName: Retrieves the nth argument passed to the  */
/*   function call currently being evaluated and determines */
/*   if it is a valid module name. If valid, the module     */
/*   name is returned or NULL is returned to indicate all   */
/*   modules.                                               */
/************************************************************/
Defmodule *GetModuleName(
  UDFContext *context,
  unsigned int whichArgument,
  bool *error)
  {
   UDFValue returnValue;
   Defmodule *theModule;
   Environment *theEnv = context->environment;
   const char *functionName = UDFContextFunctionName(context);

   *error = false;

   /*========================*/
   /* Retrieve the argument. */
   /*========================*/

   if (! UDFNthArgument(context,1,SYMBOL_BIT,&returnValue))
     {
      *error = true;
      return NULL;
     }

   /*=======================================*/
   /* Check to see that the symbol actually */
   /* corresponds to a defined module.      */
   /*=======================================*/

   if ((theModule = FindDefmodule(theEnv,returnValue.lexemeValue->contents)) == NULL)
     {
      if (strcmp("*",returnValue.lexemeValue->contents) != 0)
        {
         ExpectedTypeError1(theEnv,functionName,whichArgument,"'defmodule name'");
         *error = true;
        }
      return NULL;
     }

   /*=================================*/
   /* Return a pointer to the module. */
   /*=================================*/

   return(theModule);
  }

/****************************************************************/
/* GetConstructName: Retrieves the 1st argument passed to the   */
/*   function call currently being evaluated and determines if  */
/*   it is a valid name for a construct. Also checks that the   */
/*   function is only passed a single argument. This routine    */
/*   is used by functions such as ppdeftemplate, undefrule,     */
/*   etc... to retrieve the construct name on which to operate. */
/****************************************************************/
const char *GetConstructName(
  UDFContext *context,
  const char *functionName,
  const char *constructType)
  {
   UDFValue returnValue;

   if (! UDFFirstArgument(context,ANY_TYPE_BITS,&returnValue))
     { return NULL; }

   if (! CVIsType(&returnValue,SYMBOL_BIT))
     {
      UDFInvalidArgumentMessage(context,constructType);
      return NULL;
     }

   return(returnValue.lexemeValue->contents);
  }

/*********************************************************/
/* ExpectedCountError: Prints the error message for an   */
/*   incorrect number of arguments passed to a function. */
/*********************************************************/
void ExpectedCountError(
  Environment *theEnv,
  const char *functionName,
  int countRelation,
  unsigned int expectedNumber)
  {
   PrintErrorID(theEnv,"ARGACCES",1,false);
   WriteString(theEnv,STDERR,"Function '");
   WriteString(theEnv,STDERR,functionName);
   WriteString(theEnv,STDERR,"'");

   if (countRelation == EXACTLY)
     { WriteString(theEnv,STDERR," expected exactly "); }
   else if (countRelation == AT_LEAST)
     { WriteString(theEnv,STDERR," expected at least "); }
   else if (countRelation == NO_MORE_THAN)
     { WriteString(theEnv,STDERR," expected no more than "); }
   else
     { WriteString(theEnv,STDERR," generated an illegal argument check for "); }

   PrintUnsignedInteger(theEnv,STDERR,expectedNumber);
   
   if (expectedNumber == 1)
     { WriteString(theEnv,STDERR," argument.\n"); }
   else
     { WriteString(theEnv,STDERR," arguments.\n"); }
  }

/*************************************************************/
/*  NAME         : CheckFunctionArgCount                     */
/*  DESCRIPTION  : Checks the number of arguments against    */
/*                 the system function restriction list      */
/*  INPUTS       : 1) Name of the calling function           */
/*                 2) The restriction list can be NULL       */
/*                 3) The number of arguments                */
/*  RETURNS      : True if OK, false otherwise               */
/*  SIDE EFFECTS : EvaluationError set on errrors            */
/*  NOTES        : Used to check generic function implicit   */
/*                 method (system function) calls and system */
/*                 function calls which have the sequence    */
/*                 expansion operator in their argument list */
/*************************************************************/
bool CheckFunctionArgCount(
  Environment *theEnv,
  struct functionDefinition *func,
  int argumentCount)
  {
   unsigned short minArguments, maxArguments;
   const char *functionName;

   functionName = func->callFunctionName->contents;

   /*===========================================*/
   /* Determine the minimum number of arguments */
   /* required by the function.                 */
   /*===========================================*/

   minArguments = func->minArgs;

   /*===========================================*/
   /* Determine the maximum number of arguments */
   /* required by the function.                 */
   /*===========================================*/

   maxArguments = func->maxArgs;

   /*=====================================*/
   /* If the function has no restrictions */
   /* on function arguments, return true. */
   /*=====================================*/

   if ((minArguments == UNBOUNDED) && (maxArguments == UNBOUNDED))
     { return true; }

   /*==============================================*/
   /* If the function expects exactly N arguments, */
   /* then check to see if there are N arguments.  */
   /*==============================================*/

   if (minArguments == maxArguments)
     {
      if (argumentCount != minArguments)
        {
         ExpectedCountError(theEnv,functionName,EXACTLY,minArguments);
         SetEvaluationError(theEnv,true);
         return false;
        }
      return true;
     }

   /*==================================*/
   /* Check to see if there were fewer */
   /* arguments passed than expected.  */
   /*==================================*/

   if (argumentCount < minArguments)
     {
      ExpectedCountError(theEnv,functionName,AT_LEAST,minArguments);
      SetEvaluationError(theEnv,true);
      return false;
     }

   /*=================================*/
   /* Check to see if there were more */
   /* arguments passed than expected. */
   /*=================================*/

   if ((maxArguments != UNBOUNDED) && (argumentCount > maxArguments))
     {
      ExpectedCountError(theEnv,functionName,NO_MORE_THAN,maxArguments);
      SetEvaluationError(theEnv,true);
      return false;
     }

   /*===============================*/
   /* The number of arguments falls */
   /* within the expected range.    */
   /*===============================*/

   return true;
  }

/*******************************************************************/
/* ExpectedTypeError0: Prints the error message for the wrong type */
/*   of argument passed to a user or system defined function.      */
/*******************************************************************/
void ExpectedTypeError0(
  Environment *theEnv,
  const char *functionName,
  unsigned int whichArg)
  {
   PrintErrorID(theEnv,"ARGACCES",2,false);
   WriteString(theEnv,STDERR,"Function '");
   WriteString(theEnv,STDERR,functionName);
   WriteString(theEnv,STDERR,"' expected argument #");
   WriteInteger(theEnv,STDERR,whichArg);
   WriteString(theEnv,STDERR," to be of type ");
  }

/*******************************************************************/
/* ExpectedTypeError1: Prints the error message for the wrong type */
/*   of argument passed to a user or system defined function. The  */
/*   expected type is passed as a string to this function.         */
/*******************************************************************/
void ExpectedTypeError1(
  Environment *theEnv,
  const char *functionName,
  unsigned int whichArg,
  const char *expectedType)
  {
   ExpectedTypeError0(theEnv,functionName,whichArg);
   WriteString(theEnv,STDERR,expectedType);
   WriteString(theEnv,STDERR,".\n");
  }

/**************************************************************/
/* ExpectedTypeError2: Prints the error message for the wrong */
/*   type of argument passed to a user or system defined      */
/*   function. The expected type is derived by examining the  */
/*   function's argument restriction list.                    */
/**************************************************************/
void ExpectedTypeError2(
  Environment *theEnv,
  const char *functionName,
  unsigned int whichArg)
  {
   unsigned theRestriction;
   struct functionDefinition *theFunction;

   theFunction = FindFunction(theEnv,functionName);

   if (theFunction == NULL) return;

   theRestriction = GetNthRestriction(theEnv,theFunction,whichArg);
   ExpectedTypeError0(theEnv,functionName,whichArg);
   PrintTypesString(theEnv,STDERR,theRestriction,true);
  }

/***************************************************/
/* GetFactOrInstanceArgument: Utility routine for  */
/*   retrieving a fact or instance argument        */
/***************************************************/
void *GetFactOrInstanceArgument(
  UDFContext *context,
  unsigned int thePosition,
  UDFValue *item)
  {
   Environment *theEnv = context->environment;
#if DEFTEMPLATE_CONSTRUCT || OBJECT_SYSTEM
   void *ptr;
#endif

   /*==============================*/
   /* Retrieve the first argument. */
   /*==============================*/

   UDFNthArgument(context,thePosition,ANY_TYPE_BITS,item);

   /*==================================================*/
   /* Fact and instance addresses are valid arguments. */
   /*==================================================*/

   if (CVIsType(item,FACT_ADDRESS_BIT))
     {
      if (item->factValue->garbage)
        {
         FactRetractedErrorMessage(theEnv,item->factValue);
         return NULL;
        }
        
      return item->value;
     }

   else if (CVIsType(item,INSTANCE_ADDRESS_BIT))
     {
      if (item->instanceValue->garbage)
        {
         CantFindItemErrorMessage(theEnv,"instance",item->instanceValue->name->contents,false);
         return NULL;
        }
        
      return item->value;
     }

   /*==================================================*/
   /* An integer is a valid argument if it corresponds */
   /* to the fact index of an existing fact.           */
   /*==================================================*/

#if DEFTEMPLATE_CONSTRUCT
   else if (item->header->type == INTEGER_TYPE)
     {
      if ((ptr = (void *) FindIndexedFact(theEnv,item->integerValue->contents)) == NULL)
        {
         char tempBuffer[20];
         gensprintf(tempBuffer,"f-%lld",item->integerValue->contents);
         CantFindItemErrorMessage(theEnv,"fact",tempBuffer,false);
        }
      return ptr;
     }
#endif

   /*================================================*/
   /* Instance names and symbols are valid arguments */
   /* if they correspond to an existing instance.    */
   /*================================================*/

#if OBJECT_SYSTEM
   else if (CVIsType(item,INSTANCE_NAME_BIT | SYMBOL_BIT))
     {
      if ((ptr = (void *) FindInstanceBySymbol(theEnv,item->lexemeValue)) == NULL)
        {
         CantFindItemErrorMessage(theEnv,"instance",item->lexemeValue->contents,false);
        }
      return ptr;
     }
#endif

   /*========================================*/
   /* Any other type is an invalid argument. */
   /*========================================*/

   ExpectedTypeError2(theEnv,UDFContextFunctionName(context),thePosition);
   return NULL;
  }

/****************************************************/
/* IllegalLogicalNameMessage: Generic error message */
/*   for illegal logical names.                     */
/****************************************************/
void IllegalLogicalNameMessage(
  Environment *theEnv,
  const char *theFunction)
  {
   PrintErrorID(theEnv,"IOFUN",1,false);
   WriteString(theEnv,STDERR,"Illegal logical name used for '");
   WriteString(theEnv,STDERR,theFunction);
   WriteString(theEnv,STDERR,"' function.\n");
  }
