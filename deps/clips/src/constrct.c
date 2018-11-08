   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  11/10/17             */
   /*                                                     */
   /*                  CONSTRUCT MODULE                   */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides basic functionality for creating new    */
/*   types of constructs, saving constructs to a file, and   */
/*   adding new functionality to the clear and reset         */
/*   commands.                                               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Added environment parameter to GenClose.       */
/*            Added environment parameter to GenOpen.        */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Changed garbage collection algorithm.          */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Added code for capturing errors/warnings       */
/*            (EnvSetParserErrorCallback).                   */
/*                                                           */
/*            Fixed issue with save function when multiple   */
/*            defmodules exist.                              */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Fixed linkage issue when BLOAD_ONLY compiler   */
/*            flag is set to 1.                              */
/*                                                           */
/*            Added code to prevent a clear command from     */
/*            being executed during fact assertions via      */
/*            Increment/DecrementClearReadyLocks API.        */
/*                                                           */
/*            Added code to keep track of pointers to        */
/*            constructs that are contained externally to    */
/*            to constructs, DanglingConstructs.             */
/*                                                           */
/*      6.31: Error flags reset before Clear processed when  */
/*            called from embedded controller.               */
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
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*            Callbacks must be environment aware.           */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Modified EnvClear to return completion status. */
/*                                                           */
/*            Compilation watch flag defaults to off.        */
/*                                                           */
/*            File name/line count displayed for errors      */
/*            and warnings during load command.              */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <string.h>

#include "setup.h"

#include "argacces.h"
#include "commline.h"
#include "constant.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "memalloc.h"
#include "miscfun.h"
#include "moduldef.h"
#include "modulutl.h"
#include "multifld.h"
#include "prcdrfun.h"
#include "prcdrpsr.h"
#include "prntutil.h"
#include "router.h"
#include "ruledef.h"
#include "scanner.h"
#include "sysdep.h"
#include "utility.h"
#include "watch.h"

#include "constrct.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                        DeallocateConstructData(Environment *);

/**************************************************/
/* InitializeConstructData: Allocates environment */
/*    data for constructs.                        */
/**************************************************/
void InitializeConstructData(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,CONSTRUCT_DATA,sizeof(struct constructData),DeallocateConstructData);
  }

/****************************************************/
/* DeallocateConstructData: Deallocates environment */
/*    data for constructs.                          */
/****************************************************/
static void DeallocateConstructData(
  Environment *theEnv)
  {
   Construct *tmpPtr, *nextPtr;

#if (! RUN_TIME) && (! BLOAD_ONLY)
   DeallocateSaveCallList(theEnv,ConstructData(theEnv)->ListOfSaveFunctions);
#endif
   DeallocateVoidCallList(theEnv,ConstructData(theEnv)->ListOfResetFunctions);
   DeallocateVoidCallList(theEnv,ConstructData(theEnv)->ListOfClearFunctions);
   DeallocateBoolCallList(theEnv,ConstructData(theEnv)->ListOfClearReadyFunctions);

#if (! RUN_TIME) && (! BLOAD_ONLY)
   if (ConstructData(theEnv)->ErrorString != NULL)
     { genfree(theEnv,ConstructData(theEnv)->ErrorString,sizeof(ConstructData(theEnv)->ErrorString) + 1); }

   if (ConstructData(theEnv)->WarningString != NULL)
     { genfree(theEnv,ConstructData(theEnv)->WarningString,sizeof(ConstructData(theEnv)->WarningString) + 1); }

   ConstructData(theEnv)->ErrorString = NULL;
   ConstructData(theEnv)->WarningString = NULL;

   SetParsingFileName(theEnv,NULL);
   SetWarningFileName(theEnv,NULL);
   SetErrorFileName(theEnv,NULL);
#endif

   tmpPtr = ConstructData(theEnv)->ListOfConstructs;
   while (tmpPtr != NULL)
     {
      nextPtr = tmpPtr->next;
      rtn_struct(theEnv,construct,tmpPtr);
      tmpPtr = nextPtr;
     }
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/***********************************************/
/* SetParserErrorCallback: Allows the function */
/*   which is called when a construct parsing  */
/*    error occurs to be changed.              */
/***********************************************/
ParserErrorFunction *SetParserErrorCallback(
   Environment *theEnv,
   ParserErrorFunction *functionPtr,
   void *context)
  {
   ParserErrorFunction *tmpPtr;

   tmpPtr = ConstructData(theEnv)->ParserErrorCallback;
   ConstructData(theEnv)->ParserErrorCallback = functionPtr;
   ConstructData(theEnv)->ParserErrorContext = context;
   return tmpPtr;
  }

/*************************************************/
/* FindConstruct: Determines whether a construct */
/*   type is in the ListOfConstructs.            */
/*************************************************/
Construct *FindConstruct(
  Environment *theEnv,
  const char *name)
  {
   Construct *currentPtr;

   for (currentPtr = ConstructData(theEnv)->ListOfConstructs;
        currentPtr != NULL;
        currentPtr = currentPtr->next)
     {
      if (strcmp(name,currentPtr->constructName) == 0)
        { return currentPtr; }
     }

   return NULL;
  }

/***********************************************************/
/* RemoveConstruct: Removes a construct and its associated */
/*   parsing function from the ListOfConstructs. Returns   */
/*   true if the construct type was removed, otherwise     */
/*   false.                                                */
/***********************************************************/
bool RemoveConstruct(
  Environment *theEnv,
  const char *name)
  {
   Construct *currentPtr, *lastPtr = NULL;

   for (currentPtr = ConstructData(theEnv)->ListOfConstructs;
        currentPtr != NULL;
        currentPtr = currentPtr->next)
     {
      if (strcmp(name,currentPtr->constructName) == 0)
        {
         if (lastPtr == NULL)
           { ConstructData(theEnv)->ListOfConstructs = currentPtr->next; }
         else
           { lastPtr->next = currentPtr->next; }
         rtn_struct(theEnv,construct,currentPtr);
         return true;
        }

      lastPtr = currentPtr;
     }

   return false;
  }

/************************************************/
/* Save: C access routine for the save command. */
/************************************************/
bool Save(
  Environment *theEnv,
  const char *fileName)
  {
   struct saveCallFunctionItem *saveFunction;
   FILE *filePtr;
   Defmodule *defmodulePtr;
   bool updated = false;
   bool unvisited = true;

   /*=====================================*/
   /* If embedded, clear the error flags. */
   /*=====================================*/
   
   if (EvaluationData(theEnv)->CurrentExpression == NULL)
     { ResetErrorFlags(theEnv); }

   /*=====================*/
   /* Open the save file. */
   /*=====================*/

   if ((filePtr = GenOpen(theEnv,fileName,"w")) == NULL)
     { return false; }

   /*===========================*/
   /* Bypass the router system. */
   /*===========================*/

   SetFastSave(theEnv,filePtr);

   /*================================*/
   /* Mark all modules as unvisited. */
   /*================================*/

   MarkModulesAsUnvisited(theEnv);

   /*===============================================*/
   /* Save the constructs. Repeatedly loop over the */
   /* modules until each module has been save.      */
   /*===============================================*/

   while (unvisited)
     {
      unvisited = false;
      updated = false;

      for (defmodulePtr = GetNextDefmodule(theEnv,NULL);
           defmodulePtr != NULL;
           defmodulePtr = GetNextDefmodule(theEnv,defmodulePtr))
        {
         /*=================================================================*/
         /* We only want to save a module if all of the modules it imports  */
         /* from have already been saved. Since there can't be circular     */
         /* dependencies in imported modules, this should save the modules  */
         /* that don't import anything first and then work back from those. */
         /*=================================================================*/

         if (defmodulePtr->visitedFlag)
           { /* Module has already been saved. */ }
         else if (AllImportedModulesVisited(theEnv,defmodulePtr))
           {
            for (saveFunction = ConstructData(theEnv)->ListOfSaveFunctions;
                 saveFunction != NULL;
                 saveFunction = saveFunction->next)
              { (*saveFunction->func)(theEnv,defmodulePtr,(char *) filePtr,saveFunction->context); }

            updated = true;
            defmodulePtr->visitedFlag = true;
           }
         else
           { unvisited = true; }
        }

      /*=====================================================================*/
      /* At least one module should be saved in every pass. If all have been */
      /* visited/saved, then both flags will be false. If all remaining      */
      /* unvisited/unsaved modules were visited/saved, then unvisited will   */
      /* be false and updated will be true. If some, but not all, remaining  */
      /* unvisited/unsaved modules are visited/saved, then  unvisited will   */
      /* be true and updated will be true. This leaves the case where there  */
      /* are remaining unvisited/unsaved modules, but none were              */
      /* visited/saved: unvisited is true and updated is false.              */
      /*=====================================================================*/

      if (unvisited && (! updated))
        {
         SystemError(theEnv,"CONSTRCT",2);
         break;
        }
     }

   /*======================*/
   /* Close the save file. */
   /*======================*/

   GenClose(theEnv,filePtr);

   /*===========================*/
   /* Remove the router bypass. */
   /*===========================*/

   SetFastSave(theEnv,NULL);

   /*=========================*/
   /* Return true to indicate */
   /* successful completion.  */
   /*=========================*/

   return true;
  }

/*******************************************************/
/* RemoveSaveFunction: Removes a function from the     */
/*   ListOfSaveFunctions. Returns true if the function */
/*   was successfully removed, otherwise false.        */
/*******************************************************/
bool RemoveSaveFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   ConstructData(theEnv)->ListOfSaveFunctions =
     RemoveSaveFunctionFromCallList(theEnv,name,ConstructData(theEnv)->ListOfSaveFunctions,&found);

   if (found) return true;

   return false;
  }

/**********************************/
/* SetCompilationsWatch: Sets the */
/*   value of WatchCompilations.  */
/**********************************/
void SetCompilationsWatch(
  Environment *theEnv,
  bool value)
  {
   ConstructData(theEnv)->WatchCompilations = value;
  }

/*************************************/
/* GetCompilationsWatch: Returns the */
/*   value of WatchCompilations.     */
/*************************************/
bool GetCompilationsWatch(
  Environment *theEnv)
  {
   return ConstructData(theEnv)->WatchCompilations;
  }

/**********************************/
/* SetPrintWhileLoading: Sets the */
/*   value of PrintWhileLoading.  */
/**********************************/
void SetPrintWhileLoading(
  Environment *theEnv,
  bool value)
  {
   ConstructData(theEnv)->PrintWhileLoading = value;
  }

/*************************************/
/* GetPrintWhileLoading: Returns the */
/*   value of PrintWhileLoading.     */
/*************************************/
bool GetPrintWhileLoading(
  Environment *theEnv)
  {
   return(ConstructData(theEnv)->PrintWhileLoading);
  }

/*******************************/
/* SetLoadInProgress: Sets the */
/*   value of LoadInProgress.  */
/*******************************/
void SetLoadInProgress(
  Environment *theEnv,
  bool value)
  {
   ConstructData(theEnv)->LoadInProgress = value;
  }

/**********************************/
/* GetLoadInProgress: Returns the */
/*   value of LoadInProgress.     */
/**********************************/
bool GetLoadInProgress(
  Environment *theEnv)
  {
   return(ConstructData(theEnv)->LoadInProgress);
  }

#endif

/*************************************/
/* InitializeConstructs: Initializes */
/*   the Construct Manager.          */
/*************************************/
void InitializeConstructs(
  Environment *theEnv)
  {
#if (! RUN_TIME)
   AddUDF(theEnv,"clear","v",0,0,NULL,ClearCommand,"ClearCommand",NULL);
   AddUDF(theEnv,"reset","v",0,0,NULL,ResetCommand,"ResetCommand",NULL);

#if DEBUGGING_FUNCTIONS && (! BLOAD_ONLY)
   AddWatchItem(theEnv,"compilations",0,&ConstructData(theEnv)->WatchCompilations,30,NULL,NULL);
#endif
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }

/**************************************/
/* ClearCommand: H/L access routine   */
/*   for the clear command.           */
/**************************************/
void ClearCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Clear(theEnv);
  }

/**************************************/
/* ResetCommand: H/L access routine   */
/*   for the reset command.           */
/**************************************/
void ResetCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Reset(theEnv);
  }

/****************************/
/* Reset: C access routine  */
/*   for the reset command. */
/****************************/
void Reset(
  Environment *theEnv)
  {
   struct voidCallFunctionItem *resetPtr;
   GCBlock gcb;

   /*=====================================*/
   /* The reset command can't be executed */
   /* while a reset is in progress.       */
   /*=====================================*/

   if (ConstructData(theEnv)->ResetInProgress) return;

   ConstructData(theEnv)->ResetInProgress = true;
   ConstructData(theEnv)->ResetReadyInProgress = true;
   
   /*=====================================*/
   /* If embedded, clear the error flags. */
   /*=====================================*/
   
   if (EvaluationData(theEnv)->CurrentExpression == NULL)
     { ResetErrorFlags(theEnv); }
   SetErrorValue(theEnv,NULL);
    
   /*========================================*/
   /* Set up the frame for tracking garbage. */
   /*========================================*/

   GCBlockStart(theEnv,&gcb);

   /*=======================================================*/
   /* Call the before reset function to determine if the    */
   /* reset should continue. [Used by the some of the       */
   /* windowed interfaces to query the user whether a       */
   /* reset should proceed with activations on the agenda.] */
   /*=======================================================*/

   if ((ConstructData(theEnv)->BeforeResetCallback != NULL) ?
       ((*ConstructData(theEnv)->BeforeResetCallback)(theEnv) == false) : false)
     {
      ConstructData(theEnv)->ResetReadyInProgress = false;
      ConstructData(theEnv)->ResetInProgress = false;
      return;
     }
   ConstructData(theEnv)->ResetReadyInProgress = false;

   /*===========================*/
   /* Call each reset function. */
   /*===========================*/

   for (resetPtr = ConstructData(theEnv)->ListOfResetFunctions;
        (resetPtr != NULL) && (GetHaltExecution(theEnv) == false);
        resetPtr = resetPtr->next)
     { (*resetPtr->func)(theEnv,resetPtr->context); }

   /*============================================*/
   /* Set the current module to the MAIN module. */
   /*============================================*/

   SetCurrentModule(theEnv,FindDefmodule(theEnv,"MAIN"));

   /*===========================================*/
   /* Perform periodic cleanup if the reset was */
   /* issued from an embedded controller.       */
   /*===========================================*/

   GCBlockEnd(theEnv,&gcb);
   CallPeriodicTasks(theEnv);

   /*===================================*/
   /* A reset is no longer in progress. */
   /*===================================*/

   ConstructData(theEnv)->ResetInProgress = false;
  }

/************************************/
/* SetBeforeResetFunction: Sets the */
/*  value of BeforeResetFunction.   */
/************************************/
BeforeResetFunction *SetBeforeResetFunction(
  Environment *theEnv,
  BeforeResetFunction *theFunction)
  {
   BeforeResetFunction *tempFunction;

   tempFunction = ConstructData(theEnv)->BeforeResetCallback;
   ConstructData(theEnv)->BeforeResetCallback = theFunction;
   return tempFunction;
  }

/*************************************/
/* AddResetFunction: Adds a function */
/*   to ListOfResetFunctions.        */
/*************************************/
bool AddResetFunction(
  Environment *theEnv,
  const char *name,
  VoidCallFunction *functionPtr,
  int priority,
  void *context)
  {
   ConstructData(theEnv)->ListOfResetFunctions =
      AddVoidFunctionToCallList(theEnv,name,priority,functionPtr,
                                ConstructData(theEnv)->ListOfResetFunctions,context);
   return true;
  }

/*******************************************/
/* RemoveResetFunction: Removes a function */
/*   from the ListOfResetFunctions.        */
/*******************************************/
bool RemoveResetFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   ConstructData(theEnv)->ListOfResetFunctions =
      RemoveVoidFunctionFromCallList(theEnv,name,ConstructData(theEnv)->ListOfResetFunctions,&found);

   return found;
  }

/****************************************/
/* IncrementClearReadyLocks: Increments */
/*   the number of clear ready locks.   */
/****************************************/
void IncrementClearReadyLocks(
  Environment *theEnv)
  {
   ConstructData(theEnv)->ClearReadyLocks++;
  }

/*******************************************/
/* DecrementClearReadyLocks: Decrements    */
/*   the number of clear locks.            */
/*******************************************/
void DecrementClearReadyLocks(
  Environment *theEnv)
  {
   if (ConstructData(theEnv)->ClearReadyLocks > 0)
     { ConstructData(theEnv)->ClearReadyLocks--; }
  }

/**************************************************/
/* Clear: C access routine for the clear command. */
/**************************************************/
bool Clear(
  Environment *theEnv)
  {
   struct voidCallFunctionItem *theFunction;
   GCBlock gcb;

   /*=====================================*/
   /* If embedded, clear the error flags. */
   /*=====================================*/

   if (EvaluationData(theEnv)->CurrentExpression == NULL)
     { ResetErrorFlags(theEnv); }
   SetErrorValue(theEnv,NULL);
   
   /*===================================*/
   /* Determine if a clear is possible. */
   /*===================================*/

   ConstructData(theEnv)->ClearReadyInProgress = true;
   if ((ConstructData(theEnv)->ClearReadyLocks > 0) ||
       (ConstructData(theEnv)->DanglingConstructs > 0) ||
       (ClearReady(theEnv) == false))
     {
      PrintErrorID(theEnv,"CONSTRCT",1,false);
      WriteString(theEnv,STDERR,"Some constructs are still in use. Clear cannot continue.\n");
      ConstructData(theEnv)->ClearReadyInProgress = false;
      return false;
     }
   ConstructData(theEnv)->ClearReadyInProgress = false;

   /*========================================*/
   /* Set up the frame for tracking garbage. */
   /*========================================*/
   
   GCBlockStart(theEnv,&gcb);

   /*===========================*/
   /* Call all clear functions. */
   /*===========================*/

   ConstructData(theEnv)->ClearInProgress = true;

   for (theFunction = ConstructData(theEnv)->ListOfClearFunctions;
        theFunction != NULL;
        theFunction = theFunction->next)
     { (*theFunction->func)(theEnv,theFunction->context); }

   /*================================*/
   /* Restore the old garbage frame. */
   /*================================*/
   
   GCBlockEnd(theEnv,&gcb);
   CallPeriodicTasks(theEnv);

   /*===========================*/
   /* Clear has been completed. */
   /*===========================*/

   ConstructData(theEnv)->ClearInProgress = false;

#if DEFRULE_CONSTRUCT
   if ((DefruleData(theEnv)->RightPrimeJoins != NULL) ||
       (DefruleData(theEnv)->LeftPrimeJoins != NULL))
     { SystemError(theEnv,"CONSTRCT",1); }
#endif

   /*============================*/
   /* Perform reset after clear. */
   /*============================*/

   Reset(theEnv);

   return true;
  }

/*********************************************************/
/* ClearReady: Returns true if a clear can be performed, */
/*   otherwise false. Note that this is destructively    */
/*   determined (e.g. facts will be deleted as part of   */
/*   the determination).                                 */
/*********************************************************/
bool ClearReady(
  Environment *theEnv)
  {
   struct boolCallFunctionItem *theFunction;

   for (theFunction = ConstructData(theEnv)->ListOfClearReadyFunctions;
        theFunction != NULL;
        theFunction = theFunction->next)
     {
      if ((*theFunction->func)(theEnv,theFunction->context) == false)
        { return false; }
     }

   return true;
  }

/******************************************/
/* AddClearReadyFunction: Adds a function */
/*   to ListOfClearReadyFunctions.        */
/******************************************/
bool AddClearReadyFunction(
  Environment *theEnv,
  const char *name,
  BoolCallFunction *functionPtr,
  int priority,
  void *context)
  {
   ConstructData(theEnv)->ListOfClearReadyFunctions =
     AddBoolFunctionToCallList(theEnv,name,priority,functionPtr,
                               ConstructData(theEnv)->ListOfClearReadyFunctions,context);
   return true;
  }

/************************************************/
/* RemoveClearReadyFunction: Removes a function */
/*   from the ListOfClearReadyFunctions.        */
/************************************************/
bool RemoveClearReadyFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   ConstructData(theEnv)->ListOfClearReadyFunctions =
      RemoveBoolFunctionFromCallList(theEnv,name,ConstructData(theEnv)->ListOfClearReadyFunctions,&found);

   if (found) return true;

   return false;
  }

/*************************************/
/* AddClearFunction: Adds a function */
/*   to ListOfClearFunctions.        */
/*************************************/
bool AddClearFunction(
  Environment *theEnv,
  const char *name,
  VoidCallFunction *functionPtr,
  int priority,
  void *context)
  {
   ConstructData(theEnv)->ListOfClearFunctions =
      AddVoidFunctionToCallList(theEnv,name,priority,functionPtr,
                                ConstructData(theEnv)->ListOfClearFunctions,context);
   return true;
  }

/*******************************************/
/* RemoveClearFunction: Removes a function */
/*    from the ListOfClearFunctions.       */
/*******************************************/
bool RemoveClearFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   ConstructData(theEnv)->ListOfClearFunctions =
     RemoveVoidFunctionFromCallList(theEnv,name,ConstructData(theEnv)->ListOfClearFunctions,&found);

   if (found) return true;

   return false;
  }

/********************************************/
/* ExecutingConstruct: Returns true if a    */
/*   construct is currently being executed, */
/*   otherwise false.                       */
/********************************************/
bool ExecutingConstruct(
  Environment *theEnv)
  {
   return ConstructData(theEnv)->Executing;
  }

/********************************************/
/* SetExecutingConstruct: Sets the value of */
/*   the executing variable indicating that */
/*   actions such as reset, clear, etc      */
/*   should not be performed.               */
/********************************************/
void SetExecutingConstruct(
  Environment *theEnv,
  bool value)
  {
   ConstructData(theEnv)->Executing = value;
  }

/*******************************************************/
/* DeinstallConstructHeader: Decrements the busy count */
/*   of a construct name and frees its pretty print    */
/*   representation string (both of which are stored   */
/*   in the generic construct header).                 */
/*******************************************************/
void DeinstallConstructHeader(
  Environment *theEnv,
  ConstructHeader *theHeader)
  {
   ReleaseLexeme(theEnv,theHeader->name);
   if (theHeader->ppForm != NULL)
     {
      rm(theEnv,(void *) theHeader->ppForm,
         sizeof(char) * (strlen(theHeader->ppForm) + 1));
      theHeader->ppForm = NULL;
     }

   if (theHeader->usrData != NULL)
     {
      ClearUserDataList(theEnv,theHeader->usrData);
      theHeader->usrData = NULL;
     }
  }

/**************************************************/
/* DestroyConstructHeader: Frees the pretty print */
/*   representation string and user data (both of */
/*   which are stored in the generic construct    */
/*   header).                                     */
/**************************************************/
void DestroyConstructHeader(
  Environment *theEnv,
  ConstructHeader *theHeader)
  {
   if (theHeader->ppForm != NULL)
     {
      rm(theEnv,(void *) theHeader->ppForm,
         sizeof(char) * (strlen(theHeader->ppForm) + 1));
      theHeader->ppForm = NULL;
     }

   if (theHeader->usrData != NULL)
     {
      ClearUserDataList(theEnv,theHeader->usrData);
      theHeader->usrData = NULL;
     }
  }

/*****************************************************/
/* AddConstruct: Adds a construct and its associated */
/*   parsing function to the ListOfConstructs.       */
/*****************************************************/
Construct *AddConstruct(
  Environment *theEnv,
  const char *name,
  const char *pluralName,
  bool (*parseFunction)(Environment *,const char *),
  FindConstructFunction *findFunction,
  CLIPSLexeme *(*getConstructNameFunction)(ConstructHeader *),
  const char *(*getPPFormFunction)(ConstructHeader *),
  struct defmoduleItemHeader *(*getModuleItemFunction)(ConstructHeader *),
  GetNextConstructFunction *getNextItemFunction,
  void (*setNextItemFunction)(ConstructHeader *,ConstructHeader *),
  IsConstructDeletableFunction *isConstructDeletableFunction,
  DeleteConstructFunction *deleteFunction,
  FreeConstructFunction *freeFunction)
  {
   Construct *newPtr;

   /*=============================*/
   /* Allocate and initialize the */
   /* construct data structure.   */
   /*=============================*/

   newPtr = get_struct(theEnv,construct);

   newPtr->constructName = name;
   newPtr->pluralName = pluralName;
   newPtr->parseFunction = parseFunction;
   newPtr->findFunction = findFunction;
   newPtr->getConstructNameFunction = getConstructNameFunction;
   newPtr->getPPFormFunction = getPPFormFunction;
   newPtr->getModuleItemFunction = getModuleItemFunction;
   newPtr->getNextItemFunction = getNextItemFunction;
   newPtr->setNextItemFunction = setNextItemFunction;
   newPtr->isConstructDeletableFunction = isConstructDeletableFunction;
   newPtr->deleteFunction = deleteFunction;
   newPtr->freeFunction = freeFunction;

   /*===============================*/
   /* Add the construct to the list */
   /* of constructs and return it.  */
   /*===============================*/

   newPtr->next = ConstructData(theEnv)->ListOfConstructs;
   ConstructData(theEnv)->ListOfConstructs = newPtr;
   return(newPtr);
  }

/************************************/
/* AddSaveFunction: Adds a function */
/*   to the ListOfSaveFunctions.    */
/************************************/
bool AddSaveFunction(
  Environment *theEnv,
  const char *name,
  SaveCallFunction *functionPtr,
  int priority,
  void *context)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)
   ConstructData(theEnv)->ListOfSaveFunctions =
     AddSaveFunctionToCallList(theEnv,name,priority,
                           functionPtr,
                           ConstructData(theEnv)->ListOfSaveFunctions,context);
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif

   return true;
  }

/**********************************************************/
/* AddSaveFunctionToCallList: Adds a function to a list   */
/*   of functions which are called to perform certain     */
/*   operations (e.g. clear, reset, and bload functions). */
/**********************************************************/
SaveCallFunctionItem *AddSaveFunctionToCallList(
  Environment *theEnv,
  const char *name,
  int priority,
  SaveCallFunction *func,
  struct saveCallFunctionItem *head,
  void *context)
  {
   struct saveCallFunctionItem *newPtr, *currentPtr, *lastPtr = NULL;
   char  *nameCopy;

   newPtr = get_struct(theEnv,saveCallFunctionItem);

   nameCopy = (char *) genalloc(theEnv,strlen(name) + 1);
   genstrcpy(nameCopy,name);
   newPtr->name = nameCopy;

   newPtr->func = func;
   newPtr->priority = priority;
   newPtr->context = context;

   if (head == NULL)
     {
      newPtr->next = NULL;
      return(newPtr);
     }

   currentPtr = head;
   while ((currentPtr != NULL) ? (priority < currentPtr->priority) : false)
     {
      lastPtr = currentPtr;
      currentPtr = currentPtr->next;
     }

   if (lastPtr == NULL)
     {
      newPtr->next = head;
      head = newPtr;
     }
   else
     {
      newPtr->next = currentPtr;
      lastPtr->next = newPtr;
     }

   return(head);
  }

/******************************************************************/
/* RemoveSaveFunctionFromCallList: Removes a function from a list */
/*   of functions which are called to perform certain operations  */
/*   (e.g. clear, reset, and bload functions).                    */
/******************************************************************/
struct saveCallFunctionItem *RemoveSaveFunctionFromCallList(
  Environment *theEnv,
  const char *name,
  struct saveCallFunctionItem *head,
  bool *found)
  {
   struct saveCallFunctionItem *currentPtr, *lastPtr;

   *found = false;
   lastPtr = NULL;
   currentPtr = head;

   while (currentPtr != NULL)
     {
      if (strcmp(name,currentPtr->name) == 0)
        {
         *found = true;
         if (lastPtr == NULL)
           { head = currentPtr->next; }
         else
           { lastPtr->next = currentPtr->next; }

         genfree(theEnv,(void *) currentPtr->name,strlen(currentPtr->name) + 1);
         rtn_struct(theEnv,saveCallFunctionItem,currentPtr);
         return head;
        }

      lastPtr = currentPtr;
      currentPtr = currentPtr->next;
     }

   return head;
  }

/*************************************************************/
/* DeallocateSaveCallList: Removes all functions from a list */
/*   of functions which are called to perform certain        */
/*   operations (e.g. clear, reset, and bload functions).    */
/*************************************************************/
void DeallocateSaveCallList(
  Environment *theEnv,
  struct saveCallFunctionItem *theList)
  {
   struct saveCallFunctionItem *tmpPtr, *nextPtr;

   tmpPtr = theList;
   while (tmpPtr != NULL)
     {
      nextPtr = tmpPtr->next;
      genfree(theEnv,(void *) tmpPtr->name,strlen(tmpPtr->name) + 1);
      rtn_struct(theEnv,saveCallFunctionItem,tmpPtr);
      tmpPtr = nextPtr;
     }
  }

