   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/02/18             */
   /*                                                     */
   /*         DEFMODULE BASIC COMMANDS HEADER FILE        */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements core commands for the defmodule       */
/*   construct such as clear, reset, save, ppdefmodule       */
/*   list-defmodules, and get-defmodule-list.                */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
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
/*            Pretty print functions accept optional logical */
/*            name argument.                                 */
/*                                                           */
/*************************************************************/

#include "setup.h"

#include <stdio.h>
#include <string.h>

#include "argacces.h"
#include "bload.h"
#include "constrct.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "modulbin.h"
#include "modulcmp.h"
#include "multifld.h"
#include "prntutil.h"
#include "router.h"

#include "modulbsc.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    ClearDefmodules(Environment *,void *);
#if DEFMODULE_CONSTRUCT
   static void                    SaveDefmodules(Environment *,Defmodule *,const char *,void *);
#endif

/*****************************************************************/
/* DefmoduleBasicCommands: Initializes basic defmodule commands. */
/*****************************************************************/
void DefmoduleBasicCommands(
  Environment *theEnv)
  {
   AddClearFunction(theEnv,"defmodule",ClearDefmodules,2000,NULL);

#if DEFMODULE_CONSTRUCT
   AddSaveFunction(theEnv,"defmodule",SaveDefmodules,1100,NULL);

#if ! RUN_TIME
   AddUDF(theEnv,"get-defmodule-list","m",0,0,NULL,GetDefmoduleListFunction,"GetDefmoduleListFunction",NULL);

#if DEBUGGING_FUNCTIONS
   AddUDF(theEnv,"list-defmodules","v",0,0,NULL,ListDefmodulesCommand,"ListDefmodulesCommand",NULL);
   AddUDF(theEnv,"ppdefmodule","v",1,2,";y;ldsyn",PPDefmoduleCommand,"PPDefmoduleCommand",NULL);
#endif
#endif
#endif

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE)
   DefmoduleBinarySetup(theEnv);
#endif

#if CONSTRUCT_COMPILER && (! RUN_TIME)
   DefmoduleCompilerSetup(theEnv);
#endif
  }

/*********************************************************/
/* ClearDefmodules: Defmodule clear routine for use with */
/*   the clear command. Creates the MAIN module.         */
/*********************************************************/
static void ClearDefmodules(
  Environment *theEnv,
  void *context)
  {
#if (BLOAD || BLOAD_AND_BSAVE || BLOAD_ONLY) && (! RUN_TIME)
   if (Bloaded(theEnv) == true) return;
#endif
#if (! RUN_TIME)
   RemoveAllDefmodules(theEnv,NULL);

   CreateMainModule(theEnv,NULL);
   DefmoduleData(theEnv)->MainModuleRedefinable = true;
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }

#if DEFMODULE_CONSTRUCT

/******************************************/
/* SaveDefmodules: Defmodule save routine */
/*   for use with the save command.       */
/******************************************/
static void SaveDefmodules(
  Environment *theEnv,
  Defmodule *theModule,
  const char *logicalName,
  void *context)
  {
   const char *ppform;

   ppform = DefmodulePPForm(theModule);
   if (ppform != NULL)
     {
      WriteString(theEnv,logicalName,ppform);
      WriteString(theEnv,logicalName,"\n");
     }
  }

/************************************************/
/* GetDefmoduleListFunction: H/L access routine */
/*   for the get-defmodule-list function.       */
/************************************************/
void GetDefmoduleListFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   CLIPSValue result;
   
   GetDefmoduleList(theEnv,&result);
   CLIPSToUDFValue(&result,returnValue);
  }

/******************************************/
/* GetDefmoduleList: C access routine     */
/*   for the get-defmodule-list function. */
/******************************************/
void GetDefmoduleList(
  Environment *theEnv,
  CLIPSValue *returnValue)
  {
   Defmodule *theConstruct;
   unsigned long count = 0;
   Multifield *theList;

   /*====================================*/
   /* Determine the number of constructs */
   /* of the specified type.             */
   /*====================================*/

   for (theConstruct = GetNextDefmodule(theEnv,NULL);
        theConstruct != NULL;
        theConstruct = GetNextDefmodule(theEnv,theConstruct))
     { count++; }

   /*===========================*/
   /* Create a multifield large */
   /* enough to store the list. */
   /*===========================*/

   theList = CreateMultifield(theEnv,count);
   returnValue->value = theList;

   /*====================================*/
   /* Store the names in the multifield. */
   /*====================================*/

   for (theConstruct = GetNextDefmodule(theEnv,NULL), count = 0;
        theConstruct != NULL;
        theConstruct = GetNextDefmodule(theEnv,theConstruct), count++)
     {
      if (EvaluationData(theEnv)->HaltExecution == true)
        {
         returnValue->multifieldValue = CreateMultifield(theEnv,0L);
         return;
        }
      theList->contents[count].lexemeValue = CreateSymbol(theEnv,DefmoduleName(theConstruct));
     }
  }

#if DEBUGGING_FUNCTIONS

/********************************************/
/* PPDefmoduleCommand: H/L access routine   */
/*   for the ppdefmodule command.           */
/********************************************/
void PPDefmoduleCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *defmoduleName;
   const char *logicalName;
   const char *ppForm;

   defmoduleName = GetConstructName(context,"ppdefmodule","defmodule name");
   if (defmoduleName == NULL) return;

   if (UDFHasNextArgument(context))
     {
      logicalName = GetLogicalName(context,STDOUT);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"ppdefmodule");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         return;
        }
     }
   else
     { logicalName = STDOUT; }

   if (strcmp(logicalName,"nil") == 0)
     {
      ppForm = PPDefmoduleNil(theEnv,defmoduleName);
      
      if (ppForm == NULL)
        { CantFindItemErrorMessage(theEnv,"defmodule",defmoduleName,true); }

      returnValue->lexemeValue = CreateString(theEnv,ppForm);
      
      return;
     }

   PPDefmodule(theEnv,defmoduleName,logicalName);

   return;
  }

/****************************************/
/* PPDefmoduleNil: C access routine for */
/*   the ppdefmodule command.           */
/****************************************/
const char *PPDefmoduleNil(
  Environment *theEnv,
  const char *defmoduleName)
  {
   Defmodule *defmodulePtr;

   defmodulePtr = FindDefmodule(theEnv,defmoduleName);
   if (defmodulePtr == NULL)
     {
      CantFindItemErrorMessage(theEnv,"defmodule",defmoduleName,true);
      return NULL;
     }

   if (DefmodulePPForm(defmodulePtr) == NULL) return "";
   
   return DefmodulePPForm(defmodulePtr);
  }

/*************************************/
/* PPDefmodule: C access routine for */
/*   the ppdefmodule command.        */
/*************************************/
bool PPDefmodule(
  Environment *theEnv,
  const char *defmoduleName,
  const char *logicalName)
  {
   Defmodule *defmodulePtr;

   defmodulePtr = FindDefmodule(theEnv,defmoduleName);
   if (defmodulePtr == NULL)
     {
      CantFindItemErrorMessage(theEnv,"defmodule",defmoduleName,true);
      return false;
     }

   if (DefmodulePPForm(defmodulePtr) == NULL) return true;
   WriteString(theEnv,logicalName,DefmodulePPForm(defmodulePtr));

   return true;
  }

/***********************************************/
/* ListDefmodulesCommand: H/L access routine   */
/*   for the list-defmodules command.          */
/***********************************************/
void ListDefmodulesCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   ListDefmodules(theEnv,STDOUT);
  }

/**************************************/
/* ListDefmodules: C access routine   */
/*   for the list-defmodules command. */
/**************************************/
void ListDefmodules(
  Environment *theEnv,
  const char *logicalName)
  {
   Defmodule *theModule;
   unsigned int count = 0;

   for (theModule = GetNextDefmodule(theEnv,NULL);
        theModule != NULL;
        theModule = GetNextDefmodule(theEnv,theModule))
    {
     WriteString(theEnv,logicalName,DefmoduleName(theModule));
     WriteString(theEnv,logicalName,"\n");
     count++;
    }

   PrintTally(theEnv,logicalName,count,"defmodule","defmodules");
  }

#endif /* DEBUGGING_FUNCTIONS */

#endif /* DEFMODULE_CONSTRUCT */


