   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/02/18             */
   /*                                                     */
   /*                 FACT MANAGER MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides core routines for maintaining the fact  */
/*   list including assert/retract operations, data          */
/*   structure creation/deletion, printing, slot access,     */
/*   and other utility functions.                            */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Added support for templates maintaining their  */
/*            own list of facts.                             */
/*                                                           */
/*      6.24: Removed LOGICAL_DEPENDENCIES compilation flag. */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            AssignFactSlotDefaults function does not       */
/*            properly handle defaults for multifield slots. */
/*            DR0869                                         */
/*                                                           */
/*            Support for ppfact command.                    */
/*                                                           */
/*      6.30: Callback function support for assertion,       */
/*            retraction, and modification of facts.         */
/*                                                           */
/*            Updates to fact pattern entity record.         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Removed unused global variables.               */
/*                                                           */
/*            Added code to prevent a clear command from     */
/*            being executed during fact assertions via      */
/*            JoinOperationInProgress mechanism.             */
/*                                                           */
/*            Added code to keep track of pointers to        */
/*            constructs that are contained externally to    */
/*            to constructs, DanglingConstructs.             */
/*                                                           */
/*      6.31: Added NULL check for slotName in function      */
/*            EnvGetFactSlot. Return value of FALSE now      */
/*            returned if garbage flag set for fact.         */
/*                                                           */
/*            Added constraint checking for slot value in    */
/*            EnvPutFactSlot function.                       */
/*                                                           */
/*            Calling EnvFactExistp for a fact that has      */
/*            been created, but not asserted now returns     */
/*            FALSE.                                         */
/*                                                           */
/*            Calling EnvRetract for a fact that has been    */
/*            created, but not asserted now returns FALSE.   */
/*                                                           */
/*            Calling EnvAssignFactSlotDefaults or           */
/*            EnvPutFactSlot for a fact that has been        */
/*            asserted now returns FALSE.                    */
/*                                                           */
/*            Retracted and existing facts cannot be         */
/*            asserted.                                      */
/*                                                           */
/*            Crash bug fix for modifying fact with invalid  */
/*            slot name.                                     */
/*                                                           */
/*      6.40: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
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
/*            Removed initial-fact support.                  */
/*                                                           */
/*            Watch facts for modify command only prints     */
/*            changed slots.                                 */
/*                                                           */
/*            Modify command preserves fact id and address.  */
/*                                                           */
/*            Assert returns duplicate fact. FALSE is now    */
/*            returned only if an error occurs.              */
/*                                                           */
/*            Pretty print functions accept optional logical */
/*            name argument.                                 */
/*                                                           */
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT

#include "commline.h"
#include "default.h"
#include "engine.h"
#include "factbin.h"
#include "factcmp.h"
#include "factcom.h"
#include "factfun.h"
#include "factmch.h"
#include "factqury.h"
#include "factrhs.h"
#include "lgcldpnd.h"
#include "memalloc.h"
#include "multifld.h"
#include "retract.h"
#include "prntutil.h"
#include "router.h"
#include "strngrtr.h"
#include "sysdep.h"
#include "tmpltbsc.h"
#include "tmpltfun.h"
#include "tmpltutl.h"
#include "utility.h"
#include "watch.h"
#include "cstrnchk.h"

#include "factmngr.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    ResetFacts(Environment *,void *);
   static bool                    ClearFactsReady(Environment *,void *);
   static void                    RemoveGarbageFacts(Environment *,void *);
   static void                    DeallocateFactData(Environment *);
   static bool                    RetractCallback(Fact *,Environment *);

/**************************************************************/
/* InitializeFacts: Initializes the fact data representation. */
/*   Facts are only available when both the defrule and       */
/*   deftemplate constructs are available.                    */
/**************************************************************/
void InitializeFacts(
  Environment *theEnv)
  {
   struct patternEntityRecord factInfo =
      { { "FACT_ADDRESS_TYPE", FACT_ADDRESS_TYPE,1,0,0,
          (EntityPrintFunction *) PrintFactIdentifier,
          (EntityPrintFunction *) PrintFactIdentifierInLongForm,
          (bool (*)(void *,Environment *)) RetractCallback,
          NULL,
          (void *(*)(void *,void *)) GetNextFact,
          (EntityBusyCountFunction *) DecrementFactCallback,
          (EntityBusyCountFunction *) IncrementFactCallback,
          NULL,NULL,NULL,NULL,NULL
        },
        (void (*)(Environment *,void *)) DecrementFactBasisCount,
        (void (*)(Environment *,void *)) IncrementFactBasisCount,
        (void (*)(Environment *,void *)) MatchFactFunction,
        NULL,
        (bool (*)(Environment *,void *)) FactIsDeleted
      };

   Fact dummyFact = { { { { FACT_ADDRESS_TYPE } , NULL, NULL, 0, 0L } },
                      NULL, NULL, -1L, 0, 1,
                      NULL, NULL, NULL, NULL, NULL, 
                      { {MULTIFIELD_TYPE } , 1, 0UL, NULL, { { { NULL } } } } };

   AllocateEnvironmentData(theEnv,FACTS_DATA,sizeof(struct factsData),DeallocateFactData);

   memcpy(&FactData(theEnv)->FactInfo,&factInfo,sizeof(struct patternEntityRecord));
   dummyFact.patternHeader.theInfo = &FactData(theEnv)->FactInfo;
   memcpy(&FactData(theEnv)->DummyFact,&dummyFact,sizeof(struct fact));
   FactData(theEnv)->LastModuleIndex = -1;

   /*=========================================*/
   /* Initialize the fact hash table (used to */
   /* quickly determine if a fact exists).    */
   /*=========================================*/

   InitializeFactHashTable(theEnv);

   /*============================================*/
   /* Initialize the fact callback functions for */
   /* use with the reset and clear commands.     */
   /*============================================*/

   AddResetFunction(theEnv,"facts",ResetFacts,60,NULL);
   AddClearReadyFunction(theEnv,"facts",ClearFactsReady,0,NULL);

   /*=============================*/
   /* Initialize periodic garbage */
   /* collection for facts.       */
   /*=============================*/

   AddCleanupFunction(theEnv,"facts",RemoveGarbageFacts,0,NULL);

   /*===================================*/
   /* Initialize fact pattern matching. */
   /*===================================*/

   InitializeFactPatterns(theEnv);

   /*==================================*/
   /* Initialize the facts keyword for */
   /* use with the watch command.      */
   /*==================================*/

#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"facts",0,&FactData(theEnv)->WatchFacts,80,
                DeftemplateWatchAccess,DeftemplateWatchPrint);
#endif

   /*=========================================*/
   /* Initialize fact commands and functions. */
   /*=========================================*/

   FactCommandDefinitions(theEnv);
   FactFunctionDefinitions(theEnv);

   /*==============================*/
   /* Initialize fact set queries. */
   /*==============================*/

#if FACT_SET_QUERIES
   SetupFactQuery(theEnv);
#endif

   /*==================================*/
   /* Initialize fact patterns for use */
   /* with the bload/bsave commands.   */
   /*==================================*/

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE) && (! RUN_TIME)
   FactBinarySetup(theEnv);
#endif

   /*===================================*/
   /* Initialize fact patterns for use  */
   /* with the constructs-to-c command. */
   /*===================================*/

#if CONSTRUCT_COMPILER && (! RUN_TIME)
   FactPatternsCompilerSetup(theEnv);
#endif
  }

/***********************************/
/* DeallocateFactData: Deallocates */
/*   environment data for facts.   */
/***********************************/
static void DeallocateFactData(
  Environment *theEnv)
  {
   struct factHashEntry *tmpFHEPtr, *nextFHEPtr;
   Fact *tmpFactPtr, *nextFactPtr;
   unsigned long i;
   struct patternMatch *theMatch, *tmpMatch;

   for (i = 0; i < FactData(theEnv)->FactHashTableSize; i++)
     {
      tmpFHEPtr = FactData(theEnv)->FactHashTable[i];

      while (tmpFHEPtr != NULL)
        {
         nextFHEPtr = tmpFHEPtr->next;
         rtn_struct(theEnv,factHashEntry,tmpFHEPtr);
         tmpFHEPtr = nextFHEPtr;
        }
     }

   rm(theEnv,FactData(theEnv)->FactHashTable,
       sizeof(struct factHashEntry *) * FactData(theEnv)->FactHashTableSize);

   tmpFactPtr = FactData(theEnv)->FactList;
   while (tmpFactPtr != NULL)
     {
      nextFactPtr = tmpFactPtr->nextFact;

      theMatch = (struct patternMatch *) tmpFactPtr->list;
      while (theMatch != NULL)
        {
         tmpMatch = theMatch->next;
         rtn_struct(theEnv,patternMatch,theMatch);
         theMatch = tmpMatch;
        }

      ReturnEntityDependencies(theEnv,(struct patternEntity *) tmpFactPtr);

      ReturnFact(theEnv,tmpFactPtr);
      tmpFactPtr = nextFactPtr;
     }

   tmpFactPtr = FactData(theEnv)->GarbageFacts;
   while (tmpFactPtr != NULL)
     {
      nextFactPtr = tmpFactPtr->nextFact;
      ReturnFact(theEnv,tmpFactPtr);
      tmpFactPtr = nextFactPtr;
     }

   DeallocateCallListWithArg(theEnv,FactData(theEnv)->ListOfAssertFunctions);
   DeallocateCallListWithArg(theEnv,FactData(theEnv)->ListOfRetractFunctions);
   DeallocateModifyCallList(theEnv,FactData(theEnv)->ListOfModifyFunctions);
  }

/**********************************************/
/* PrintFactWithIdentifier: Displays a single */
/*   fact preceded by its fact identifier.    */
/**********************************************/
void PrintFactWithIdentifier(
  Environment *theEnv,
  const char *logicalName,
  Fact *factPtr,
  const char *changeMap)
  {
   char printSpace[20];

   gensprintf(printSpace,"f-%-5lld ",factPtr->factIndex);
   WriteString(theEnv,logicalName,printSpace);
   PrintFact(theEnv,logicalName,factPtr,false,false,changeMap);
  }

/****************************************************/
/* PrintFactIdentifier: Displays a fact identifier. */
/****************************************************/
void PrintFactIdentifier(
  Environment *theEnv,
  const char *logicalName,
  Fact *factPtr)
  {
   char printSpace[20];

   gensprintf(printSpace,"f-%lld",factPtr->factIndex);
   WriteString(theEnv,logicalName,printSpace);
  }

/********************************************/
/* PrintFactIdentifierInLongForm: Display a */
/*   fact identifier in a longer format.    */
/********************************************/
void PrintFactIdentifierInLongForm(
  Environment *theEnv,
  const char *logicalName,
  Fact *factPtr)
  {
   if (PrintUtilityData(theEnv)->AddressesToStrings) WriteString(theEnv,logicalName,"\"");
   if (factPtr != &FactData(theEnv)->DummyFact)
     {
      WriteString(theEnv,logicalName,"<Fact-");
      WriteInteger(theEnv,logicalName,factPtr->factIndex);
      WriteString(theEnv,logicalName,">");
     }
   else
     { WriteString(theEnv,logicalName,"<Dummy Fact>"); }

   if (PrintUtilityData(theEnv)->AddressesToStrings) WriteString(theEnv,logicalName,"\"");
  }

/*******************************************/
/* DecrementFactBasisCount: Decrements the */
/*   partial match busy count of a fact    */
/*******************************************/
void DecrementFactBasisCount(
  Environment *theEnv,
  Fact *factPtr)
  {
   Multifield *theSegment;
   size_t i;

   ReleaseFact(factPtr);

   if (factPtr->basisSlots != NULL)
     {
      theSegment = factPtr->basisSlots;
      factPtr->basisSlots->busyCount--;
     }
   else
     { theSegment = &factPtr->theProposition; }

   for (i = 0 ; i < theSegment->length ; i++)
     { AtomDeinstall(theEnv,theSegment->contents[i].header->type,theSegment->contents[i].value); }

   if ((factPtr->basisSlots != NULL) && (factPtr->basisSlots->busyCount == 0))
     {
      ReturnMultifield(theEnv,factPtr->basisSlots);
      factPtr->basisSlots = NULL;
     }
  }

/*******************************************/
/* IncrementFactBasisCount: Increments the */
/*   partial match busy count of a fact.   */
/*******************************************/
void IncrementFactBasisCount(
  Environment *theEnv,
  Fact *factPtr)
  {
   Multifield *theSegment;
   size_t i;

   RetainFact(factPtr);

   theSegment = &factPtr->theProposition;

   if (theSegment->length != 0)
     {
      if (factPtr->basisSlots != NULL)
        {
         factPtr->basisSlots->busyCount++;
        }
      else
        {
         factPtr->basisSlots = CopyMultifield(theEnv,theSegment);
         factPtr->basisSlots->busyCount = 1;
        }
      theSegment = factPtr->basisSlots;
     }

   for (i = 0 ; i < theSegment->length ; i++)
     {
      AtomInstall(theEnv,theSegment->contents[i].header->type,theSegment->contents[i].value);
     }
  }

/******************/
/* FactIsDeleted: */
/******************/
bool FactIsDeleted(
  Environment *theEnv,
  Fact *theFact)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   return theFact->garbage;
  }

/**************************************************/
/* PrintFact: Displays the printed representation */
/*   of a fact containing the relation name and   */
/*   all of the fact's slots or fields.           */
/**************************************************/
void PrintFact(
  Environment *theEnv,
  const char *logicalName,
  Fact *factPtr,
  bool separateLines,
  bool ignoreDefaults,
  const char *changeMap)
  {
   Multifield *theMultifield;

   /*=========================================*/
   /* Print a deftemplate (non-ordered) fact. */
   /*=========================================*/

   if (factPtr->whichDeftemplate->implied == false)
     {
      PrintTemplateFact(theEnv,logicalName,factPtr,separateLines,ignoreDefaults,changeMap);
      return;
     }

   /*==============================*/
   /* Print an ordered fact (which */
   /* has an implied deftemplate). */
   /*==============================*/

   WriteString(theEnv,logicalName,"(");

   WriteString(theEnv,logicalName,factPtr->whichDeftemplate->header.name->contents);

   theMultifield = factPtr->theProposition.contents[0].multifieldValue;
   if (theMultifield->length != 0)
     {
      WriteString(theEnv,logicalName," ");
      PrintMultifieldDriver(theEnv,logicalName,theMultifield,0,
                            theMultifield->length,false);
     }

   WriteString(theEnv,logicalName,")");
  }

/*********************************************/
/* MatchFactFunction: Filters a fact through */
/*   the appropriate fact pattern network.   */
/*********************************************/
void MatchFactFunction(
  Environment *theEnv,
  Fact *theFact)
  {
   FactPatternMatch(theEnv,theFact,theFact->whichDeftemplate->patternNetwork,0,0,NULL,NULL);
  }

/**********************************************/
/* RetractDriver: Driver routine for Retract. */
/**********************************************/
RetractError RetractDriver(
  Environment *theEnv,
  Fact *theFact,
  bool modifyOperation,
  char *changeMap)
  {
   Deftemplate *theTemplate = theFact->whichDeftemplate;
   struct callFunctionItemWithArg *theRetractFunction;

   FactData(theEnv)->retractError = RE_NO_ERROR;

   /*===========================================*/
   /* Retracting a retracted fact does nothing. */
   /*===========================================*/

   if (theFact->garbage)
     { return RE_NO_ERROR; }

   /*===========================================*/
   /* A fact can not be retracted while another */
   /* fact is being asserted or retracted.      */
   /*===========================================*/

   if (EngineData(theEnv)->JoinOperationInProgress)
     {
      PrintErrorID(theEnv,"FACTMNGR",1,true);
      WriteString(theEnv,STDERR,"Facts may not be retracted during pattern-matching.\n");
      SetEvaluationError(theEnv,true);
      FactData(theEnv)->retractError = RE_COULD_NOT_RETRACT_ERROR;
      return RE_COULD_NOT_RETRACT_ERROR;
     }

   /*====================================*/
   /* A NULL fact pointer indicates that */
   /* all facts should be retracted.     */
   /*====================================*/

   if (theFact == NULL)
     { return RetractAllFacts(theEnv); }

   /*=================================================*/
   /* Check to see if the fact has not been asserted. */
   /*=================================================*/
   
   if (theFact->factIndex == 0)
     {
      SystemError(theEnv,"FACTMNGR",5);
      ExitRouter(theEnv,EXIT_FAILURE);
     }
   
   /*===========================================*/
   /* Execute the list of functions that are    */
   /* to be called before each fact retraction. */
   /*===========================================*/

   for (theRetractFunction = FactData(theEnv)->ListOfRetractFunctions;
        theRetractFunction != NULL;
        theRetractFunction = theRetractFunction->next)
     {
      (*theRetractFunction->func)(theEnv,theFact,theRetractFunction->context);
     }

   /*============================*/
   /* Print retraction output if */
   /* facts are being watched.   */
   /*============================*/

#if DEBUGGING_FUNCTIONS
   if (theFact->whichDeftemplate->watch &&
       (! ConstructData(theEnv)->ClearReadyInProgress) &&
       (! ConstructData(theEnv)->ClearInProgress))
     {
      WriteString(theEnv,STDOUT,"<== ");
      PrintFactWithIdentifier(theEnv,STDOUT,theFact,changeMap);
      WriteString(theEnv,STDOUT,"\n");
     }
#endif

   /*==================================*/
   /* Set the change flag to indicate  */
   /* the fact-list has been modified. */
   /*==================================*/

   FactData(theEnv)->ChangeToFactList = true;

   /*===============================================*/
   /* Remove any links between the fact and partial */
   /* matches in the join network. These links are  */
   /* used to keep track of logical dependencies.   */
   /*===============================================*/

   RemoveEntityDependencies(theEnv,(struct patternEntity *) theFact);

   /*===========================================*/
   /* Remove the fact from the fact hash table. */
   /*===========================================*/

   RemoveHashedFact(theEnv,theFact);

   /*=========================================*/
   /* Remove the fact from its template list. */
   /*=========================================*/

   if (theFact == theTemplate->lastFact)
     { theTemplate->lastFact = theFact->previousTemplateFact; }

   if (theFact->previousTemplateFact == NULL)
     {
      theTemplate->factList = theTemplate->factList->nextTemplateFact;
      if (theTemplate->factList != NULL)
        { theTemplate->factList->previousTemplateFact = NULL; }
     }
   else
     {
      theFact->previousTemplateFact->nextTemplateFact = theFact->nextTemplateFact;
      if (theFact->nextTemplateFact != NULL)
        { theFact->nextTemplateFact->previousTemplateFact = theFact->previousTemplateFact; }
     }

   /*=====================================*/
   /* Remove the fact from the fact list. */
   /*=====================================*/

   if (theFact == FactData(theEnv)->LastFact)
     { FactData(theEnv)->LastFact = theFact->previousFact; }

   if (theFact->previousFact == NULL)
     {
      FactData(theEnv)->FactList = FactData(theEnv)->FactList->nextFact;
      if (FactData(theEnv)->FactList != NULL)
        { FactData(theEnv)->FactList->previousFact = NULL; }
     }
   else
     {
      theFact->previousFact->nextFact = theFact->nextFact;
      if (theFact->nextFact != NULL)
        { theFact->nextFact->previousFact = theFact->previousFact; }
     }

   /*===================================================*/
   /* Add the fact to the fact garbage list unless this */
   /* fact is being retract as part of a modify action. */
   /*===================================================*/

   if (! modifyOperation)
     {
      theFact->nextFact = FactData(theEnv)->GarbageFacts;
      FactData(theEnv)->GarbageFacts = theFact;
      UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
     }
   else
     {
      theFact->nextFact = NULL;
     }
   theFact->garbage = true;

   /*===================================================*/
   /* Reset the evaluation error flag since expressions */
   /* will be evaluated as part of the retract.         */
   /*===================================================*/

   SetEvaluationError(theEnv,false);

   /*===========================================*/
   /* Loop through the list of all the patterns */
   /* that matched the fact and process the     */
   /* retract operation for each one.           */
   /*===========================================*/

   EngineData(theEnv)->JoinOperationInProgress = true;
   NetworkRetract(theEnv,(struct patternMatch *) theFact->list);
   theFact->list = NULL;
   EngineData(theEnv)->JoinOperationInProgress = false;

   /*=========================================*/
   /* Free partial matches that were released */
   /* by the retraction of the fact.          */
   /*=========================================*/

   if (EngineData(theEnv)->ExecutingRule == NULL)
     { FlushGarbagePartialMatches(theEnv); }

   /*=========================================*/
   /* Retract other facts that were logically */
   /* dependent on the fact just retracted.   */
   /*=========================================*/

   ForceLogicalRetractions(theEnv);

   /*==================================*/
   /* Update busy counts and ephemeral */
   /* garbage information.             */
   /*==================================*/

   FactDeinstall(theEnv,theFact);

   /*====================================*/
   /* Return the appropriate error code. */
   /*====================================*/

   if (GetEvaluationError(theEnv))
     {
      FactData(theEnv)->retractError = RE_RULE_NETWORK_ERROR;
      return RE_RULE_NETWORK_ERROR;
     }

   FactData(theEnv)->retractError = RE_NO_ERROR;
   return RE_NO_ERROR;
  }

/*******************/
/* RetractCallback */
/*******************/
static bool RetractCallback(
  Fact *theFact,
  Environment *theEnv)
  {
   return (RetractDriver(theEnv,theFact,false,NULL) == RE_NO_ERROR);
  }

/******************************************************/
/* Retract: C access routine for the retract command. */
/******************************************************/
RetractError Retract(
  Fact *theFact)
  {
   GCBlock gcb;
   RetractError rv;
   Environment *theEnv;
   
   if (theFact == NULL)
     { return RE_NULL_POINTER_ERROR; }

   if (theFact->garbage)
     { return RE_NO_ERROR; }

   theEnv = theFact->whichDeftemplate->header.env;

   /*=====================================*/
   /* If embedded, clear the error flags. */
   /*=====================================*/
   
   if (EvaluationData(theEnv)->CurrentExpression == NULL)
     { ResetErrorFlags(theEnv); }

   GCBlockStart(theEnv,&gcb);
   rv = RetractDriver(theEnv,theFact,false,NULL);
   GCBlockEnd(theEnv,&gcb);
   
   return rv;
  }

/*******************************************************************/
/* RemoveGarbageFacts: Returns facts that have been retracted to   */
/*   the pool of available memory. It is necessary to postpone     */
/*   returning the facts to memory because RHS actions retrieve    */
/*   their variable bindings directly from the fact data structure */
/*   and the facts may be in use in other data structures.         */
/*******************************************************************/
static void RemoveGarbageFacts(
  Environment *theEnv,
  void *context)
  {
   Fact *factPtr, *nextPtr, *lastPtr = NULL;

   factPtr = FactData(theEnv)->GarbageFacts;

   while (factPtr != NULL)
     {
      nextPtr = factPtr->nextFact;
        
      if (factPtr->patternHeader.busyCount == 0)
        {
         Multifield *theSegment;
         size_t i;
         
         theSegment = &factPtr->theProposition;
         for (i = 0 ; i < theSegment->length ; i++)
           { AtomDeinstall(theEnv,theSegment->contents[i].header->type,theSegment->contents[i].value); }

         ReturnFact(theEnv,factPtr);
         if (lastPtr == NULL) FactData(theEnv)->GarbageFacts = nextPtr;
         else lastPtr->nextFact = nextPtr;
        }
      else
        { lastPtr = factPtr; }

      factPtr = nextPtr;
     }
  }

/********************************************************/
/* AssertDriver: Driver routine for the assert command. */
/********************************************************/
Fact *AssertDriver(
  Fact *theFact,
  long long reuseIndex,
  Fact *factListPosition,
  Fact *templatePosition,
  char *changeMap)
  {
   size_t hashValue;
   size_t length, i;
   CLIPSValue *theField;
   Fact *duplicate;
   struct callFunctionItemWithArg *theAssertFunction;
   Environment *theEnv = theFact->whichDeftemplate->header.env;

   FactData(theEnv)->assertError = AE_NO_ERROR;
   
   /*==================================================*/
   /* Retracted and existing facts cannot be asserted. */
   /*==================================================*/
   
   if (theFact->garbage)
     {
      FactData(theEnv)->assertError = AE_RETRACTED_ERROR;
      return NULL;
     }

   if (reuseIndex != theFact->factIndex)
     {
      SystemError(theEnv,"FACTMNGR",6);
      ExitRouter(theEnv,EXIT_FAILURE);
     }

   /*==========================================*/
   /* A fact can not be asserted while another */
   /* fact is being asserted or retracted.     */
   /*==========================================*/

   if (EngineData(theEnv)->JoinOperationInProgress)
     {
      FactData(theEnv)->assertError = AE_COULD_NOT_ASSERT_ERROR;
      ReturnFact(theEnv,theFact);
      PrintErrorID(theEnv,"FACTMNGR",2,true);
      WriteString(theEnv,STDERR,"Facts may not be asserted during pattern-matching.\n");
      return NULL;
     }

   /*=============================================================*/
   /* Replace invalid data types in the fact with the symbol nil. */
   /*=============================================================*/

   length = theFact->theProposition.length;
   theField = theFact->theProposition.contents;

   for (i = 0; i < length; i++)
     {
      if (theField[i].value == VoidConstant(theEnv))
        { theField[i].value = CreateSymbol(theEnv,"nil"); }
     }

   /*========================================================*/
   /* If fact assertions are being checked for duplications, */
   /* then search the fact list for a duplicate fact.        */
   /*========================================================*/

   hashValue = HandleFactDuplication(theEnv,theFact,&duplicate,reuseIndex);
   if (duplicate != NULL) return duplicate;

   /*==========================================================*/
   /* If necessary, add logical dependency links between the   */
   /* fact and the partial match which is its logical support. */
   /*==========================================================*/

   if (AddLogicalDependencies(theEnv,(struct patternEntity *) theFact,false) == false)
     {
      if (reuseIndex == 0)
        { ReturnFact(theEnv,theFact); }
      else
        {
         theFact->nextFact = FactData(theEnv)->GarbageFacts;
         FactData(theEnv)->GarbageFacts = theFact;
         UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
         theFact->garbage = true;
        }
        
      FactData(theEnv)->assertError = AE_COULD_NOT_ASSERT_ERROR;
      return NULL;
     }

   /*======================================*/
   /* Add the fact to the fact hash table. */
   /*======================================*/

   AddHashedFact(theEnv,theFact,hashValue);

   /*================================*/
   /* Add the fact to the fact list. */
   /*================================*/

   if (reuseIndex == 0)
     { factListPosition = FactData(theEnv)->LastFact; }

   if (factListPosition == NULL)
     {
      theFact->nextFact = FactData(theEnv)->FactList;
      FactData(theEnv)->FactList = theFact;
      theFact->previousFact = NULL;
      if (theFact->nextFact != NULL)
        { theFact->nextFact->previousFact = theFact; }
     }
   else
     {
      theFact->nextFact = factListPosition->nextFact;
      theFact->previousFact = factListPosition;
      factListPosition->nextFact = theFact;
      if (theFact->nextFact != NULL)
        { theFact->nextFact->previousFact = theFact; }
     }

   if ((FactData(theEnv)->LastFact == NULL) || (theFact->nextFact == NULL))
     { FactData(theEnv)->LastFact = theFact; }

   /*====================================*/
   /* Add the fact to its template list. */
   /*====================================*/

   if (reuseIndex == 0)
     { templatePosition = theFact->whichDeftemplate->lastFact; }

   if (templatePosition == NULL)
     {
      theFact->nextTemplateFact = theFact->whichDeftemplate->factList;
      theFact->whichDeftemplate->factList = theFact;
      theFact->previousTemplateFact = NULL;
      if (theFact->nextTemplateFact != NULL)
        { theFact->nextTemplateFact->previousTemplateFact = theFact; }
     }
   else
     {
      theFact->nextTemplateFact = templatePosition->nextTemplateFact;
      theFact->previousTemplateFact = templatePosition;
      templatePosition->nextTemplateFact = theFact;
      if (theFact->nextTemplateFact != NULL)
        { theFact->nextTemplateFact->previousTemplateFact = theFact; }
     }

   if ((theFact->whichDeftemplate->lastFact == NULL) || (theFact->nextTemplateFact == NULL))
     { theFact->whichDeftemplate->lastFact = theFact; }

   /*==================================*/
   /* Set the fact index and time tag. */
   /*==================================*/

   if (reuseIndex > 0)
     { theFact->factIndex = reuseIndex; }
   else
     { theFact->factIndex = FactData(theEnv)->NextFactIndex++; }

   theFact->patternHeader.timeTag = DefruleData(theEnv)->CurrentEntityTimeTag++;

   /*=====================*/
   /* Update busy counts. */
   /*=====================*/

   FactInstall(theEnv,theFact);

   if (reuseIndex == 0)
     {
      Multifield *theSegment = &theFact->theProposition;
      for (i = 0 ; i < theSegment->length ; i++)
        {
         AtomInstall(theEnv,theSegment->contents[i].header->type,theSegment->contents[i].value);
        }
     }

   /*==========================================*/
   /* Execute the list of functions that are   */
   /* to be called before each fact assertion. */
   /*==========================================*/

   for (theAssertFunction = FactData(theEnv)->ListOfAssertFunctions;
        theAssertFunction != NULL;
        theAssertFunction = theAssertFunction->next)
     { (*theAssertFunction->func)(theEnv,theFact,theAssertFunction->context); }

   /*==========================*/
   /* Print assert output if   */
   /* facts are being watched. */
   /*==========================*/

#if DEBUGGING_FUNCTIONS
   if (theFact->whichDeftemplate->watch &&
       (! ConstructData(theEnv)->ClearReadyInProgress) &&
       (! ConstructData(theEnv)->ClearInProgress))
     {
      WriteString(theEnv,STDOUT,"==> ");
      PrintFactWithIdentifier(theEnv,STDOUT,theFact,changeMap);
      WriteString(theEnv,STDOUT,"\n");
     }
#endif

   /*==================================*/
   /* Set the change flag to indicate  */
   /* the fact-list has been modified. */
   /*==================================*/

   FactData(theEnv)->ChangeToFactList = true;

   /*==========================================*/
   /* Check for constraint errors in the fact. */
   /*==========================================*/

   CheckTemplateFact(theEnv,theFact);

   /*===================================================*/
   /* Reset the evaluation error flag since expressions */
   /* will be evaluated as part of the assert .         */
   /*===================================================*/

   SetEvaluationError(theEnv,false);

   /*=============================================*/
   /* Pattern match the fact using the associated */
   /* deftemplate's pattern network.              */
   /*=============================================*/

   EngineData(theEnv)->JoinOperationInProgress = true;
   FactPatternMatch(theEnv,theFact,theFact->whichDeftemplate->patternNetwork,0,0,NULL,NULL);
   EngineData(theEnv)->JoinOperationInProgress = false;

   /*===================================================*/
   /* Retract other facts that were logically dependent */
   /* on the non-existence of the fact just asserted.   */
   /*===================================================*/

   ForceLogicalRetractions(theEnv);

   /*=========================================*/
   /* Free partial matches that were released */
   /* by the assertion of the fact.           */
   /*=========================================*/

   if (EngineData(theEnv)->ExecutingRule == NULL) FlushGarbagePartialMatches(theEnv);

   /*===============================*/
   /* Return a pointer to the fact. */
   /*===============================*/

   if (EvaluationData(theEnv)->EvaluationError)
     { FactData(theEnv)->assertError = AE_RULE_NETWORK_ERROR; }

   return theFact;
  }

/*****************************************************/
/* Assert: C access routine for the assert function. */
/*****************************************************/
Fact *Assert(
  Fact *theFact)
  {
   return AssertDriver(theFact,0,NULL,NULL,NULL);
  }

/*************************/
/* GetAssertStringError: */
/*************************/
AssertStringError GetAssertStringError(
  Environment *theEnv)
  {
   return FactData(theEnv)->assertStringError;
  }

/**************************************/
/* RetractAllFacts: Loops through the */
/*   fact-list and removes each fact. */
/**************************************/
RetractError RetractAllFacts(
  Environment *theEnv)
  {
   RetractError rv;
   
   while (FactData(theEnv)->FactList != NULL)
     {
      if ((rv = Retract(FactData(theEnv)->FactList)) != RE_NO_ERROR)
        { return rv; }
     }
     
   return RE_NO_ERROR;
  }

/*********************************************/
/* CreateFact: Creates a fact data structure */
/*   of the specified deftemplate.           */
/*********************************************/
Fact *CreateFact(
  Deftemplate *theDeftemplate)
  {
   Fact *newFact;
   unsigned short i;
   Environment *theEnv = theDeftemplate->header.env;

   /*=================================*/
   /* A deftemplate must be specified */
   /* in order to create a fact.      */
   /*=================================*/

   if (theDeftemplate == NULL) return NULL;

   /*============================================*/
   /* Create a fact for an explicit deftemplate. */
   /*============================================*/

   if (theDeftemplate->implied == false)
     {
      newFact = CreateFactBySize(theEnv,theDeftemplate->numberOfSlots);
      for (i = 0;
           i < theDeftemplate->numberOfSlots;
           i++)
        { newFact->theProposition.contents[i].voidValue = VoidConstant(theEnv); }
     }

   /*===========================================*/
   /* Create a fact for an implied deftemplate. */
   /*===========================================*/

   else
     {
      newFact = CreateFactBySize(theEnv,1);
      newFact->theProposition.contents[0].value = CreateUnmanagedMultifield(theEnv,0L);
     }

   /*===============================*/
   /* Return a pointer to the fact. */
   /*===============================*/

   newFact->whichDeftemplate = theDeftemplate;

   return newFact;
  }

/****************************************/
/* GetFactSlot: Returns the slot value  */
/*   from the specified slot of a fact. */
/****************************************/
GetSlotError GetFactSlot(
  Fact *theFact,
  const char *slotName,
  CLIPSValue *theValue)
  {
   Deftemplate *theDeftemplate;
   unsigned short whichSlot;
   Environment *theEnv = theFact->whichDeftemplate->header.env;
   
   if (theFact == NULL)
     {
      return GSE_NULL_POINTER_ERROR;
     }
     
   if (theFact->garbage)
     {
      theValue->lexemeValue = FalseSymbol(theEnv);
      return GSE_INVALID_TARGET_ERROR;
     }

   /*===============================================*/
   /* Get the deftemplate associated with the fact. */
   /*===============================================*/

   theDeftemplate = theFact->whichDeftemplate;

   /*==============================================*/
   /* Handle retrieving the slot value from a fact */
   /* having an implied deftemplate. An implied    */
   /* facts has a single multifield slot.          */
   /*==============================================*/

   if (theDeftemplate->implied)
     {
      if (slotName != NULL)
        {
         if (strcmp(slotName,"implied") != 0)
           { return GSE_SLOT_NOT_FOUND_ERROR; }
        }
        
      theValue->value = theFact->theProposition.contents[0].value;
      return GSE_NO_ERROR;
     }

   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   if (slotName == NULL) return GSE_NULL_POINTER_ERROR;
   if (FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),&whichSlot) == NULL)
     { return GSE_SLOT_NOT_FOUND_ERROR; }

   /*========================*/
   /* Return the slot value. */
   /*========================*/

   theValue->value = theFact->theProposition.contents[whichSlot].value;

   return GSE_NO_ERROR;
  }

/**************************************/
/* PutFactSlot: Sets the slot value   */
/*   of the specified slot of a fact. */
/**************************************/
bool PutFactSlot(
  Fact *theFact,
  const char *slotName,
  CLIPSValue *theValue)
  {
   Deftemplate *theDeftemplate;
   struct templateSlot *theSlot;
   unsigned short whichSlot;
   Environment *theEnv = theFact->whichDeftemplate->header.env;

   /*========================================*/
   /* This function cannot be used on a fact */
   /* that's already been asserted.          */
   /*========================================*/
   
   if (theFact->factIndex != 0LL)
     { return false; }
     
   /*===============================================*/
   /* Get the deftemplate associated with the fact. */
   /*===============================================*/

   theDeftemplate = theFact->whichDeftemplate;

   /*============================================*/
   /* Handle setting the slot value of a fact    */
   /* having an implied deftemplate. An implied  */
   /* facts has a single multifield slot.        */
   /*============================================*/

   if (theDeftemplate->implied)
     {
      if ((slotName != NULL) || (theValue->header->type != MULTIFIELD_TYPE))
        { return false; }

      if (theFact->theProposition.contents[0].header->type == MULTIFIELD_TYPE)
        { ReturnMultifield(theEnv,theFact->theProposition.contents[0].multifieldValue); }

      theFact->theProposition.contents[0].value = CopyMultifield(theEnv,theValue->multifieldValue);

      return true;
     }

   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),&whichSlot)) == NULL)
     { return false; }

   /*=============================================*/
   /* Make sure a single field value is not being */
   /* stored in a multifield slot or vice versa.  */
   /*=============================================*/

   if (((theSlot->multislot == 0) && (theValue->header->type == MULTIFIELD_TYPE)) ||
       ((theSlot->multislot == 1) && (theValue->header->type != MULTIFIELD_TYPE)))
     { return false; }
     
   /*=================================*/
   /* Check constraints for the slot. */
   /*=================================*/
   
   if (theSlot->constraints != NULL)
     {
      if (ConstraintCheckValue(theEnv,theValue->header->type,theValue->value,theSlot->constraints) != NO_VIOLATION)
        { return false; }
     }

   /*=====================*/
   /* Set the slot value. */
   /*=====================*/

   if (theFact->theProposition.contents[whichSlot].header->type == MULTIFIELD_TYPE)
     { ReturnMultifield(theEnv,theFact->theProposition.contents[whichSlot].multifieldValue); }

   if (theValue->header->type == MULTIFIELD_TYPE)
     { theFact->theProposition.contents[whichSlot].multifieldValue = CopyMultifield(theEnv,theValue->multifieldValue); }
   else
     { theFact->theProposition.contents[whichSlot].value = theValue->value; }

   return true;
  }

/*******************************************************/
/* AssignFactSlotDefaults: Sets a fact's slot values   */
/*   to its default value if the value of the slot has */
/*   not yet been set.                                 */
/*******************************************************/
bool AssignFactSlotDefaults(
  Fact *theFact)
  {
   Deftemplate *theDeftemplate;
   struct templateSlot *slotPtr;
   unsigned short i;
   UDFValue theResult;
   Environment *theEnv = theFact->whichDeftemplate->header.env;

   /*========================================*/
   /* This function cannot be used on a fact */
   /* that's already been asserted.          */
   /*========================================*/
   
   if (theFact->factIndex != 0LL)
     { return false; }
     
   /*===============================================*/
   /* Get the deftemplate associated with the fact. */
   /*===============================================*/

   theDeftemplate = theFact->whichDeftemplate;

   /*================================================*/
   /* The value for the implied multifield slot of   */
   /* an implied deftemplate is set to a multifield  */
   /* of length zero when the fact is created.       */
   /*================================================*/

   if (theDeftemplate->implied) return true;

   /*============================================*/
   /* Loop through each slot of the deftemplate. */
   /*============================================*/

   for (i = 0, slotPtr = theDeftemplate->slotList;
        i < theDeftemplate->numberOfSlots;
        i++, slotPtr = slotPtr->next)
     {
      /*===================================*/
      /* If the slot's value has been set, */
      /* then move on to the next slot.    */
      /*===================================*/

      if (theFact->theProposition.contents[i].value != VoidConstant(theEnv)) continue;

      /*======================================================*/
      /* Assign the default value for the slot if one exists. */
      /*======================================================*/

      if (DeftemplateSlotDefault(theEnv,theDeftemplate,slotPtr,&theResult,false))
        {
         theFact->theProposition.contents[i].value = theResult.value;
        }
     }

   /*==========================================*/
   /* Return true to indicate that the default */
   /* values have been successfully set.       */
   /*==========================================*/

   return true;
  }

/********************************************************/
/* DeftemplateSlotDefault: Determines the default value */
/*   for the specified slot of a deftemplate.           */
/********************************************************/
bool DeftemplateSlotDefault(
  Environment *theEnv,
  Deftemplate *theDeftemplate,
  struct templateSlot *slotPtr,
  UDFValue *theResult,
  bool garbageMultifield)
  {
   /*================================================*/
   /* The value for the implied multifield slot of an */
   /* implied deftemplate does not have a default.    */
   /*=================================================*/

   if (theDeftemplate->implied) return false;

   /*===============================================*/
   /* If the (default ?NONE) attribute was declared */
   /* for the slot, then return false to indicate   */
   /* the default values for the fact couldn't be   */
   /* supplied since this attribute requires that a */
   /* default value can't be used for the slot.     */
   /*===============================================*/

   if (slotPtr->noDefault) return false;

   /*==============================================*/
   /* Otherwise if a static default was specified, */
   /* use this as the default value.               */
   /*==============================================*/

   else if (slotPtr->defaultPresent)
     {
      if (slotPtr->multislot)
        {
         StoreInMultifield(theEnv,theResult,slotPtr->defaultList,garbageMultifield);
        }
      else
        {
         theResult->value = slotPtr->defaultList->value;
        }
     }

   /*================================================*/
   /* Otherwise if a dynamic-default was specified,  */
   /* evaluate it and use this as the default value. */
   /*================================================*/

   else if (slotPtr->defaultDynamic)
     {
      if (! EvaluateAndStoreInDataObject(theEnv,slotPtr->multislot,
                                         (Expression *) slotPtr->defaultList,
                                         theResult,garbageMultifield))
        { return false; }
     }

   /*====================================*/
   /* Otherwise derive the default value */
   /* from the slot's constraints.       */
   /*====================================*/

   else
     {
      DeriveDefaultFromConstraints(theEnv,slotPtr->constraints,theResult,
                                   slotPtr->multislot,garbageMultifield);
     }

   /*==========================================*/
   /* Return true to indicate that the default */
   /* values have been successfully set.       */
   /*==========================================*/

   return true;
  }

/***************************************************************/
/* CopyFactSlotValues: Copies the slot values from one fact to */
/*   another. Both facts must have the same relation name.     */
/***************************************************************/
bool CopyFactSlotValues(
  Environment *theEnv,
  Fact *theDestFact,
  Fact *theSourceFact)
  {
   Deftemplate *theDeftemplate;
   struct templateSlot *slotPtr;
   unsigned short i;

   /*===================================*/
   /* Both facts must be the same type. */
   /*===================================*/

   theDeftemplate = theSourceFact->whichDeftemplate;
   if (theDestFact->whichDeftemplate != theDeftemplate)
     { return false; }

   /*===================================================*/
   /* Loop through each slot of the deftemplate copying */
   /* the source fact value to the destination fact.    */
   /*===================================================*/

   for (i = 0, slotPtr = theDeftemplate->slotList;
        i < theDeftemplate->numberOfSlots;
        i++, slotPtr = slotPtr->next)
     {
      if (theSourceFact->theProposition.contents[i].header->type != MULTIFIELD_TYPE)
        {
         theDestFact->theProposition.contents[i].value =
           theSourceFact->theProposition.contents[i].value;
        }
      else
        {
         theDestFact->theProposition.contents[i].value = 
           CopyMultifield(theEnv,theSourceFact->theProposition.contents[i].multifieldValue);
        }
     }

   /*========================================*/
   /* Return true to indicate that fact slot */
   /* values were successfully copied.       */
   /*========================================*/

   return true;
  }

/*********************************************/
/* CreateFactBySize: Allocates a fact data   */
/*   structure based on the number of slots. */
/*********************************************/
Fact *CreateFactBySize(
  Environment *theEnv,
  size_t size)
  {
   Fact *theFact;
   size_t newSize;

   if (size <= 0) newSize = 1;
   else newSize = size;

   theFact = get_var_struct(theEnv,fact,sizeof(struct clipsValue) * (newSize - 1));

   theFact->patternHeader.header.type = FACT_ADDRESS_TYPE;
   theFact->garbage = false;
   theFact->factIndex = 0LL;
   theFact->patternHeader.busyCount = 0;
   theFact->patternHeader.theInfo = &FactData(theEnv)->FactInfo;
   theFact->patternHeader.dependents = NULL;
   theFact->whichDeftemplate = NULL;
   theFact->nextFact = NULL;
   theFact->previousFact = NULL;
   theFact->previousTemplateFact = NULL;
   theFact->nextTemplateFact = NULL;
   theFact->list = NULL;
   theFact->basisSlots = NULL;

   theFact->theProposition.length = size;
   theFact->theProposition.busyCount = 0;

   return(theFact);
  }

/*********************************************/
/* ReturnFact: Returns a fact data structure */
/*   to the pool of free memory.             */
/*********************************************/
void ReturnFact(
  Environment *theEnv,
  Fact *theFact)
  {
   Multifield *theSegment, *subSegment;
   size_t newSize, i;

   theSegment = &theFact->theProposition;

   for (i = 0; i < theSegment->length; i++)
     {
      if (theSegment->contents[i].header->type == MULTIFIELD_TYPE)
        {
         subSegment = theSegment->contents[i].multifieldValue;
         if (subSegment != NULL)
           {
            if (subSegment->busyCount == 0)
              { ReturnMultifield(theEnv,subSegment); }
            else
              { AddToMultifieldList(theEnv,subSegment); }
           }
        }
     }

   if (theFact->theProposition.length == 0) newSize = 1;
   else newSize = theFact->theProposition.length;

   rtn_var_struct(theEnv,fact,sizeof(struct clipsValue) * (newSize - 1),theFact);
  }

/*************************************************************/
/* FactInstall: Increments the fact, deftemplate, and atomic */
/*   data value busy counts associated with the fact.        */
/*************************************************************/
void FactInstall(
  Environment *theEnv,
  Fact *newFact)
  {
   FactData(theEnv)->NumberOfFacts++;
   newFact->whichDeftemplate->busyCount++;
   newFact->patternHeader.busyCount++;
  }

/***************************************************************/
/* FactDeinstall: Decrements the fact, deftemplate, and atomic */
/*   data value busy counts associated with the fact.          */
/***************************************************************/
void FactDeinstall(
  Environment *theEnv,
  Fact *newFact)
  {
   FactData(theEnv)->NumberOfFacts--;
   newFact->whichDeftemplate->busyCount--;
   newFact->patternHeader.busyCount--;
  }

/***********************************************/
/* IncrementFactCallback: Increments the       */
/*   number of references to a specified fact. */
/***********************************************/
void IncrementFactCallback(
  Environment *theEnv,
  Fact *factPtr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
   if (factPtr == NULL) return;

   factPtr->patternHeader.busyCount++;
  }

/***********************************************/
/* DecrementFactCallback: Decrements the       */
/*   number of references to a specified fact. */
/***********************************************/
void DecrementFactCallback(
  Environment *theEnv,
  Fact *factPtr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
   if (factPtr == NULL) return;

   factPtr->patternHeader.busyCount--;
  }

/****************************************/
/* RetainFact: Increments the number of */
/*   references to a specified fact.    */
/****************************************/
void RetainFact(
  Fact *factPtr)
  {
   if (factPtr == NULL) return;

   factPtr->patternHeader.busyCount++;
  }

/*****************************************/
/* ReleaseFact: Decrements the number of */
/*   references to a specified fact.     */
/*****************************************/
void ReleaseFact(
  Fact *factPtr)
  {
   if (factPtr == NULL) return;

   factPtr->patternHeader.busyCount--;
  }

/*********************************************************/
/* GetNextFact: If passed a NULL pointer, returns the */
/*   first fact in the fact-list. Otherwise returns the  */
/*   next fact following the fact passed as an argument. */
/*********************************************************/
Fact *GetNextFact(
  Environment *theEnv,
  Fact *factPtr)
  {
   if (factPtr == NULL)
     { return FactData(theEnv)->FactList; }

   if (factPtr->garbage) return NULL;

   return factPtr->nextFact;
  }

/**************************************************/
/* GetNextFactInScope: Returns the next fact that */
/*   is in scope of the current module. Works in  */
/*   a similar fashion to GetNextFact, but skips  */
/*   facts that are out of scope.                 */
/**************************************************/
Fact *GetNextFactInScope(
  Environment *theEnv,
  Fact *theFact)
  {
   /*=======================================================*/
   /* If fact passed as an argument is a NULL pointer, then */
   /* we're just beginning a traversal of the fact list. If */
   /* the module index has changed since that last time the */
   /* fact list was traversed by this routine, then         */
   /* determine all of the deftemplates that are in scope   */
   /* of the current module.                                */
   /*=======================================================*/

   if (theFact == NULL)
     {
      theFact = FactData(theEnv)->FactList;
      if (FactData(theEnv)->LastModuleIndex != DefmoduleData(theEnv)->ModuleChangeIndex)
        {
         UpdateDeftemplateScope(theEnv);
         FactData(theEnv)->LastModuleIndex = DefmoduleData(theEnv)->ModuleChangeIndex;
        }
     }

   /*==================================================*/
   /* Otherwise, if the fact passed as an argument has */
   /* been retracted, then there's no way to determine */
   /* the next fact, so return a NULL pointer.         */
   /*==================================================*/

   else if (theFact->garbage)
     { return NULL; }

   /*==================================================*/
   /* Otherwise, start the search for the next fact in */
   /* scope with the fact immediately following the    */
   /* fact passed as an argument.                      */
   /*==================================================*/

   else
     { theFact = theFact->nextFact; }

   /*================================================*/
   /* Continue traversing the fact-list until a fact */
   /* is found that's associated with a deftemplate  */
   /* that's in scope.                               */
   /*================================================*/

   while (theFact != NULL)
     {
      if (theFact->whichDeftemplate->inScope) return theFact;

      theFact = theFact->nextFact;
     }

   return NULL;
  }

/*************************************/
/* FactPPForm: Returns the pretty    */
/*   print representation of a fact. */
/*************************************/
void FactPPForm(
  Fact *theFact,
  StringBuilder *theSB,
  bool ignoreDefaults)
  {
   Environment *theEnv = theFact->whichDeftemplate->header.env;
   
   OpenStringBuilderDestination(theEnv,"FactPPForm",theSB);
   PrintFact(theEnv,"FactPPForm",theFact,true,ignoreDefaults,NULL);
   CloseStringBuilderDestination(theEnv,"FactPPForm");
  }

/**********************************/
/* FactIndex: C access routine    */
/*   for the fact-index function. */
/**********************************/
long long FactIndex(
  Fact *factPtr)
  {
   return factPtr->factIndex;
  }

/*************************************/
/* AssertString: C access routine    */
/*   for the assert-string function. */
/*************************************/
Fact *AssertString(
  Environment *theEnv,
  const char *theString)
  {
   Fact *theFact, *rv;
   GCBlock gcb;
   int danglingConstructs;
      
   if (theString == NULL)
     {
      FactData(theEnv)->assertStringError = ASE_NULL_POINTER_ERROR;
      return NULL;
     }
     
   danglingConstructs = ConstructData(theEnv)->DanglingConstructs;
   
   /*=====================================*/
   /* If embedded, clear the error flags. */
   /*=====================================*/
   
   if (EvaluationData(theEnv)->CurrentExpression == NULL)
     { ResetErrorFlags(theEnv); }

   GCBlockStart(theEnv,&gcb);

   if ((theFact = StringToFact(theEnv,theString)) == NULL)
     {
      FactData(theEnv)->assertStringError = ASE_PARSING_ERROR;
      GCBlockEnd(theEnv,&gcb);
      return NULL;
     }

   if (EvaluationData(theEnv)->CurrentExpression == NULL)
     { ConstructData(theEnv)->DanglingConstructs = danglingConstructs; }

   rv = Assert(theFact);
   
   GCBlockEnd(theEnv,&gcb);
   
   switch(FactData(theEnv)->assertError)
     {
      case AE_NO_ERROR:
        FactData(theEnv)->assertStringError = ASE_NO_ERROR;
        break;
             
      case AE_COULD_NOT_ASSERT_ERROR:
        FactData(theEnv)->assertStringError = ASE_COULD_NOT_ASSERT_ERROR;
        
      case AE_RULE_NETWORK_ERROR:
        FactData(theEnv)->assertStringError = ASE_RULE_NETWORK_ERROR;
        break;
      
      case AE_NULL_POINTER_ERROR:
      case AE_RETRACTED_ERROR:
        SystemError(theEnv,"FACTMNGR",4);
        ExitRouter(theEnv,EXIT_FAILURE);
        break;
     }
   
   return rv;
  }

/******************************************************/
/* GetFactListChanged: Returns the flag indicating    */
/*   whether a change to the fact-list has been made. */
/******************************************************/
bool GetFactListChanged(
  Environment *theEnv)
  {
   return(FactData(theEnv)->ChangeToFactList);
  }

/********************************************************/
/* SetFactListChanged: Sets the flag indicating whether */
/*   a change to the fact-list has been made.           */
/********************************************************/
void SetFactListChanged(
  Environment *theEnv,
  bool value)
  {
   FactData(theEnv)->ChangeToFactList = value;
  }

/****************************************/
/* GetNumberOfFacts: Returns the number */
/* of facts in the fact-list.           */
/****************************************/
unsigned long GetNumberOfFacts(
  Environment *theEnv)
  {
   return(FactData(theEnv)->NumberOfFacts);
  }

/***********************************************************/
/* ResetFacts: Reset function for facts. Sets the starting */
/*   fact index to zero and removes all facts.             */
/***********************************************************/
static void ResetFacts(
  Environment *theEnv,
  void *context)
  {
   /*====================================*/
   /* Initialize the fact index to zero. */
   /*====================================*/

   FactData(theEnv)->NextFactIndex = 1L;

   /*======================================*/
   /* Remove all facts from the fact list. */
   /*======================================*/

   RetractAllFacts(theEnv);
  }

/************************************************************/
/* ClearFactsReady: Clear ready function for facts. Returns */
/*   true if facts were successfully removed and the clear  */
/*   command can continue, otherwise false.                 */
/************************************************************/
static bool ClearFactsReady(
  Environment *theEnv,
  void *context)
  {
   /*======================================*/
   /* Facts can not be deleted when a join */
   /* operation is already in progress.    */
   /*======================================*/

   if (EngineData(theEnv)->JoinOperationInProgress) return false;

   /*====================================*/
   /* Initialize the fact index to zero. */
   /*====================================*/

   FactData(theEnv)->NextFactIndex = 1L;

   /*======================================*/
   /* Remove all facts from the fact list. */
   /*======================================*/

   RetractAllFacts(theEnv);

   /*==============================================*/
   /* If for some reason there are any facts still */
   /* remaining, don't continue with the clear.    */
   /*==============================================*/

   if (GetNextFact(theEnv,NULL) != NULL) return false;

   /*=============================*/
   /* Return true to indicate the */
   /* clear command can continue. */
   /*=============================*/

   return true;
  }

/***************************************************/
/* FindIndexedFact: Returns a pointer to a fact in */
/*   the fact list with the specified fact index.  */
/***************************************************/
Fact *FindIndexedFact(
  Environment *theEnv,
  long long factIndexSought)
  {
   Fact *theFact;

   for (theFact = GetNextFact(theEnv,NULL);
        theFact != NULL;
        theFact = GetNextFact(theEnv,theFact))
     {
      if (theFact->factIndex == factIndexSought)
        { return(theFact); }
     }

   return NULL;
  }

/**************************************/
/* AddAssertFunction: Adds a function */
/*   to the ListOfAssertFunctions.    */
/**************************************/
bool AddAssertFunction(
  Environment *theEnv,
  const char *name,
  VoidCallFunctionWithArg *functionPtr,
  int priority,
  void *context)
  {
   FactData(theEnv)->ListOfAssertFunctions =
      AddFunctionToCallListWithArg(theEnv,name,priority,functionPtr,
                                   FactData(theEnv)->ListOfAssertFunctions,context);
   return true;
  }

/********************************************/
/* RemoveAssertFunction: Removes a function */
/*   from the ListOfAssertFunctions.        */
/********************************************/
bool RemoveAssertFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   FactData(theEnv)->ListOfAssertFunctions =
      RemoveFunctionFromCallListWithArg(theEnv,name,FactData(theEnv)->ListOfAssertFunctions,&found);

   if (found) return true;

   return false;
  }

/***************************************/
/* AddRetractFunction: Adds a function */
/*   to the ListOfRetractFunctions.    */
/***************************************/
bool AddRetractFunction(
  Environment *theEnv,
  const char *name,
  VoidCallFunctionWithArg *functionPtr,
  int priority,
  void *context)
  {
   FactData(theEnv)->ListOfRetractFunctions =
      AddFunctionToCallListWithArg(theEnv,name,priority,functionPtr,
                                   FactData(theEnv)->ListOfRetractFunctions,context);
   return true;
  }

/*********************************************/
/* RemoveRetractFunction: Removes a function */
/*   from the ListOfRetractFunctions.        */
/*********************************************/
bool RemoveRetractFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   FactData(theEnv)->ListOfRetractFunctions =
      RemoveFunctionFromCallListWithArg(theEnv,name,FactData(theEnv)->ListOfRetractFunctions,&found);

   if (found) return true;

   return false;
  }

/**************************************/
/* AddModifyFunction: Adds a function */
/*   to the ListOfModifyFunctions.    */
/**************************************/
bool AddModifyFunction(
  Environment *theEnv,
  const char *name,
  ModifyCallFunction *functionPtr,
  int priority,
  void *context)
  {
   FactData(theEnv)->ListOfModifyFunctions =
      AddModifyFunctionToCallList(theEnv,name,priority,functionPtr,
                                  FactData(theEnv)->ListOfModifyFunctions,context);
      
   return true;
  }

/********************************************/
/* RemoveModifyFunction: Removes a function */
/*   from the ListOfModifyFunctions.        */
/********************************************/
bool RemoveModifyFunction(
  Environment *theEnv,
  const char *name)
  {
   bool found;

   FactData(theEnv)->ListOfModifyFunctions =
      RemoveModifyFunctionFromCallList(theEnv,name,FactData(theEnv)->ListOfModifyFunctions,&found);

   if (found) return true;

   return false;
  }

/**********************************************************/
/* AddModifyFunctionToCallList: Adds a function to a list */
/*   of functions which are called to perform certain     */
/*   operations (e.g. clear, reset, and bload functions). */
/**********************************************************/
ModifyCallFunctionItem *AddModifyFunctionToCallList(
  Environment *theEnv,
  const char *name,
  int priority,
  ModifyCallFunction *func,
  ModifyCallFunctionItem *head,
  void *context)
  {
   ModifyCallFunctionItem *newPtr, *currentPtr, *lastPtr = NULL;
   char  *nameCopy;

   newPtr = get_struct(theEnv,modifyCallFunctionItem);

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

/********************************************************************/
/* RemoveModifyFunctionFromCallList: Removes a function from a list */
/*   of functions which are called to perform certain operations    */
/*   (e.g. clear, reset, and bload functions).                      */
/********************************************************************/
ModifyCallFunctionItem *RemoveModifyFunctionFromCallList(
  Environment *theEnv,
  const char *name,
  ModifyCallFunctionItem *head,
  bool *found)
  {
   ModifyCallFunctionItem *currentPtr, *lastPtr;

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
         rtn_struct(theEnv,modifyCallFunctionItem,currentPtr);
         return head;
        }

      lastPtr = currentPtr;
      currentPtr = currentPtr->next;
     }

   return head;
  }


/***************************************************************/
/* DeallocateModifyCallList: Removes all functions from a list */
/*   of functions which are called to perform certain          */
/*   operations (e.g. clear, reset, and bload functions).      */
/***************************************************************/
void DeallocateModifyCallList(
  Environment *theEnv,
  ModifyCallFunctionItem *theList)
  {
   ModifyCallFunctionItem *tmpPtr, *nextPtr;

   tmpPtr = theList;
   while (tmpPtr != NULL)
     {
      nextPtr = tmpPtr->next;
      genfree(theEnv,(void *) tmpPtr->name,strlen(tmpPtr->name) + 1);
      rtn_struct(theEnv,modifyCallFunctionItem,tmpPtr);
      tmpPtr = nextPtr;
     }
  }

/**********************/
/* CreateFactBuilder: */
/**********************/
FactBuilder *CreateFactBuilder(
  Environment *theEnv,
  const char *deftemplateName)
  {
   FactBuilder *theFB;
   Deftemplate *theDeftemplate;
   int i;
   
   if (theEnv == NULL) return NULL;
      
   if (deftemplateName != NULL)
     {
      theDeftemplate = FindDeftemplate(theEnv,deftemplateName);
      if (theDeftemplate == NULL)
        {
         FactData(theEnv)->factBuilderError = FBE_DEFTEMPLATE_NOT_FOUND_ERROR;
         return NULL;
        }
   
      if (theDeftemplate->implied)
        {
         FactData(theEnv)->factBuilderError = FBE_IMPLIED_DEFTEMPLATE_ERROR;
         return NULL;
        }
     }
   else
     { theDeftemplate = NULL; }
     
   theFB = get_struct(theEnv,factBuilder);
   theFB->fbEnv = theEnv;
   theFB->fbDeftemplate = theDeftemplate;
      
   if ((theDeftemplate == NULL) || (theDeftemplate->numberOfSlots == 0))
     { theFB->fbValueArray = NULL; }
   else
     {
      theFB->fbValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * theDeftemplate->numberOfSlots);
      for (i = 0; i < theDeftemplate->numberOfSlots; i++)
        { theFB->fbValueArray[i].voidValue = VoidConstant(theEnv); }
     }

   FactData(theEnv)->factBuilderError = FBE_NO_ERROR;
   
   return theFB;
  }

/*************************/
/* FBPutSlotCLIPSInteger */
/*************************/
PutSlotError FBPutSlotCLIPSInteger(
  FactBuilder *theFB,
  const char *slotName,
  CLIPSInteger *slotValue)
  {
   CLIPSValue theValue;

   theValue.integerValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/********************/
/* FBPutSlotInteger */
/********************/
PutSlotError FBPutSlotInteger(
  FactBuilder *theFB,
  const char *slotName,
  long long longLongValue)
  {
   CLIPSValue theValue;
   
   if (theFB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.integerValue = CreateInteger(theFB->fbEnv,longLongValue);
   return FBPutSlot(theFB,slotName,&theValue);
  }

/************************/
/* FBPutSlotCLIPSLexeme */
/************************/
PutSlotError FBPutSlotCLIPSLexeme(
  FactBuilder *theFB,
  const char *slotName,
  CLIPSLexeme *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.lexemeValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/*******************/
/* FBPutSlotSymbol */
/*******************/
PutSlotError FBPutSlotSymbol(
  FactBuilder *theFB,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theFB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateSymbol(theFB->fbEnv,stringValue);
   return FBPutSlot(theFB,slotName,&theValue);
  }

/*******************/
/* FBPutSlotString */
/*******************/
PutSlotError FBPutSlotString(
  FactBuilder *theFB,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theFB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateString(theFB->fbEnv,stringValue);
   return FBPutSlot(theFB,slotName,&theValue);
  }

/*************************/
/* FBPutSlotInstanceName */
/*************************/
PutSlotError FBPutSlotInstanceName(
  FactBuilder *theFB,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theFB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateInstanceName(theFB->fbEnv,stringValue);
   return FBPutSlot(theFB,slotName,&theValue);
  }

/***********************/
/* FBPutSlotCLIPSFloat */
/***********************/
PutSlotError FBPutSlotCLIPSFloat(
  FactBuilder *theFB,
  const char *slotName,
  CLIPSFloat *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.floatValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/******************/
/* FBPutSlotFloat */
/******************/
PutSlotError FBPutSlotFloat(
  FactBuilder *theFB,
  const char *slotName,
  double floatValue)
  {
   CLIPSValue theValue;
   
   if (theFB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.floatValue = CreateFloat(theFB->fbEnv,floatValue);
   return FBPutSlot(theFB,slotName,&theValue);
  }

/*****************/
/* FBPutSlotFact */
/*****************/
PutSlotError FBPutSlotFact(
  FactBuilder *theFB,
  const char *slotName,
  Fact *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.factValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/*********************/
/* FBPutSlotInstance */
/*********************/
PutSlotError FBPutSlotInstance(
  FactBuilder *theFB,
  const char *slotName,
  Instance *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.instanceValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/*********************************/
/* FBPutSlotCLIPSExternalAddress */
/*********************************/
PutSlotError FBPutSlotCLIPSExternalAddress(
  FactBuilder *theFB,
  const char *slotName,
  CLIPSExternalAddress *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.externalAddressValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/***********************/
/* FBPutSlotMultifield */
/***********************/
PutSlotError FBPutSlotMultifield(
  FactBuilder *theFB,
  const char *slotName,
  Multifield *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.multifieldValue = slotValue;
   return FBPutSlot(theFB,slotName,&theValue);
  }

/**************/
/* FBPutSlot: */
/**************/
PutSlotError FBPutSlot(
  FactBuilder *theFB,
  const char *slotName,
  CLIPSValue *slotValue)
  {
   Environment *theEnv;
   struct templateSlot *theSlot;
   unsigned short whichSlot;
   CLIPSValue oldValue;
   int i;
   ConstraintViolationType cvType;
     
   /*==========================*/
   /* Check for NULL pointers. */
   /*==========================*/
   
   if ((theFB == NULL) || (slotName == NULL) || (slotValue == NULL))
     { return PSE_NULL_POINTER_ERROR; }
     
   if ((theFB->fbDeftemplate == NULL) || (slotValue->value == NULL))
     { return PSE_NULL_POINTER_ERROR; }
   
   theEnv = theFB->fbEnv;
     
   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   if ((theSlot = FindSlot(theFB->fbDeftemplate,CreateSymbol(theFB->fbEnv,slotName),&whichSlot)) == NULL)
     { return PSE_SLOT_NOT_FOUND_ERROR; }
     
   /*=============================================*/
   /* Make sure a single field value is not being */
   /* stored in a multifield slot or vice versa.  */
   /*=============================================*/

   if (((theSlot->multislot == 0) && (slotValue->header->type == MULTIFIELD_TYPE)) ||
       ((theSlot->multislot == 1) && (slotValue->header->type != MULTIFIELD_TYPE)))
     { return PSE_CARDINALITY_ERROR; }
     
   /*=================================*/
   /* Check constraints for the slot. */
   /*=================================*/
   
   if (theSlot->constraints != NULL)
     {
      if ((cvType = ConstraintCheckValue(theEnv,slotValue->header->type,slotValue->value,theSlot->constraints)) != NO_VIOLATION)
        {
         switch(cvType)
           {
            case NO_VIOLATION:
            case FUNCTION_RETURN_TYPE_VIOLATION:
              SystemError(theEnv,"FACTMNGR",2);
              ExitRouter(theEnv,EXIT_FAILURE);
              break;
        
            case TYPE_VIOLATION:
              return PSE_TYPE_ERROR;
              
            case RANGE_VIOLATION:
              return PSE_RANGE_ERROR;
              
            case ALLOWED_VALUES_VIOLATION:
              return PSE_ALLOWED_VALUES_ERROR;
              
            case CARDINALITY_VIOLATION:
              return PSE_CARDINALITY_ERROR;
            
            case ALLOWED_CLASSES_VIOLATION:
              return PSE_ALLOWED_CLASSES_ERROR;
           }
        }
     }

   /*==========================*/
   /* Set up the change array. */
   /*==========================*/

   if (theFB->fbValueArray == NULL)
     {
      theFB->fbValueArray = (CLIPSValue *) gm2(theFB->fbEnv,sizeof(CLIPSValue) * theFB->fbDeftemplate->numberOfSlots);
      for (i = 0; i < theFB->fbDeftemplate->numberOfSlots; i++)
        { theFB->fbValueArray[i].voidValue = theFB->fbEnv->VoidConstant; }
     }

   /*=====================*/
   /* Set the slot value. */
   /*=====================*/
   
   oldValue.value = theFB->fbValueArray[whichSlot].value;
   
   if (oldValue.header->type == MULTIFIELD_TYPE)
     {
      if (MultifieldsEqual(oldValue.multifieldValue,slotValue->multifieldValue))
        { return PSE_NO_ERROR; }
     }
   else
     {
      if (oldValue.value == slotValue->value)
        { return PSE_NO_ERROR; }
     }
   
   Release(theEnv,oldValue.header);
   
   if (oldValue.header->type == MULTIFIELD_TYPE)
     { ReturnMultifield(theEnv,oldValue.multifieldValue); }

   if (slotValue->header->type == MULTIFIELD_TYPE)
     { theFB->fbValueArray[whichSlot].multifieldValue = CopyMultifield(theEnv,slotValue->multifieldValue); }
   else
     { theFB->fbValueArray[whichSlot].value = slotValue->value; }
      
   Retain(theEnv,theFB->fbValueArray[whichSlot].header);
   
   return PSE_NO_ERROR;
  }

/*************/
/* FBAssert: */
/*************/
Fact *FBAssert(
  FactBuilder *theFB)
  {
   Environment *theEnv;
   int i;
   Fact *theFact;
   
   if (theFB == NULL) return NULL;
   theEnv = theFB->fbEnv;
   
   if (theFB->fbDeftemplate == NULL)
     {
      FactData(theEnv)->factBuilderError = FBE_NULL_POINTER_ERROR;
      return NULL;
     }
     
   theFact = CreateFact(theFB->fbDeftemplate);
   
   for (i = 0; i < theFB->fbDeftemplate->numberOfSlots; i++)
     {
      if (theFB->fbValueArray[i].voidValue != VoidConstant(theEnv))
        {
         theFact->theProposition.contents[i].value = theFB->fbValueArray[i].value;
         Release(theEnv,theFB->fbValueArray[i].header);
         theFB->fbValueArray[i].voidValue = VoidConstant(theEnv);
        }
     }

   AssignFactSlotDefaults(theFact);
   
   theFact = Assert(theFact);
      
   switch (FactData(theEnv)->assertError)
     {
      case AE_NO_ERROR:
        FactData(theEnv)->factBuilderError = FBE_NO_ERROR;
        break;
        
      case AE_NULL_POINTER_ERROR:
      case AE_RETRACTED_ERROR:
        SystemError(theEnv,"FACTMNGR",1);
        ExitRouter(theEnv,EXIT_FAILURE);
        break;
        
      case AE_COULD_NOT_ASSERT_ERROR:
        FactData(theEnv)->factBuilderError = FBE_COULD_NOT_ASSERT_ERROR;
        break;
      
      case AE_RULE_NETWORK_ERROR:
        FactData(theEnv)->factBuilderError = FBE_RULE_NETWORK_ERROR;
        break;
     }
   
   return theFact;
  }

/**************/
/* FBDispose: */
/**************/
void FBDispose(
  FactBuilder *theFB)
  {
   Environment *theEnv;

   if (theFB == NULL) return;
   
   theEnv = theFB->fbEnv;

   FBAbort(theFB);
   
   if (theFB->fbValueArray != NULL)
     { rm(theEnv,theFB->fbValueArray,sizeof(CLIPSValue) * theFB->fbDeftemplate->numberOfSlots); }
   
   rtn_struct(theEnv,factBuilder,theFB);
  }

/************/
/* FBAbort: */
/************/
void FBAbort(
  FactBuilder *theFB)
  {
   Environment *theEnv;
   GCBlock gcb;
   int i;
   
   if (theFB == NULL) return;
   
   if (theFB->fbDeftemplate == NULL) return;

   theEnv = theFB->fbEnv;

   GCBlockStart(theEnv,&gcb);
   
   for (i = 0; i < theFB->fbDeftemplate->numberOfSlots; i++)
     {
      Release(theEnv,theFB->fbValueArray[i].header);
      
      if (theFB->fbValueArray[i].header->type == MULTIFIELD_TYPE)
        { ReturnMultifield(theEnv,theFB->fbValueArray[i].multifieldValue); }
        
      theFB->fbValueArray[i].voidValue = VoidConstant(theEnv);
     }
     
   GCBlockEnd(theEnv,&gcb);
  }

/********************/
/* FBSetDeftemplate */
/********************/
FactBuilderError FBSetDeftemplate(
  FactBuilder *theFB,
  const char *deftemplateName)
  {
   Deftemplate *theDeftemplate;
   Environment *theEnv;
   int i;
   
   if (theFB == NULL)
     { return FBE_NULL_POINTER_ERROR; }
   
   theEnv = theFB->fbEnv;

   FBAbort(theFB);
   
   if (deftemplateName != NULL)
     {
      theDeftemplate = FindDeftemplate(theFB->fbEnv,deftemplateName);
   
      if (theDeftemplate == NULL)
        {
         FactData(theEnv)->factBuilderError = FBE_DEFTEMPLATE_NOT_FOUND_ERROR;
         return FBE_DEFTEMPLATE_NOT_FOUND_ERROR;
        }
     
      if (theDeftemplate->implied)
        {
         FactData(theEnv)->factBuilderError = FBE_IMPLIED_DEFTEMPLATE_ERROR;
         return FBE_IMPLIED_DEFTEMPLATE_ERROR;
        }
     }
   else
     { theDeftemplate = NULL; }

   if (theFB->fbValueArray != NULL)
     { rm(theEnv,theFB->fbValueArray,sizeof(CLIPSValue) * theFB->fbDeftemplate->numberOfSlots); }

   theFB->fbDeftemplate = theDeftemplate;
   
   if ((theDeftemplate == NULL) || (theDeftemplate->numberOfSlots == 0))
     { theFB->fbValueArray = NULL; }
   else
     {
      theFB->fbValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * theDeftemplate->numberOfSlots);
      for (i = 0; i < theDeftemplate->numberOfSlots; i++)
        { theFB->fbValueArray[i].voidValue = VoidConstant(theEnv); }
     }

   FactData(theEnv)->factBuilderError = FBE_NO_ERROR;
   return FBE_NO_ERROR;
  }

/************/
/* FBError: */
/************/
FactBuilderError FBError(
  Environment *theEnv)
  {
   return FactData(theEnv)->factBuilderError;
  }

/***********************/
/* CreateFactModifier: */
/***********************/
FactModifier *CreateFactModifier(
  Environment *theEnv,
  Fact *oldFact)
  {
   FactModifier *theFM;
   int i;

   if (theEnv == NULL) return NULL;
     
   if (oldFact != NULL)
     {
      if (oldFact->garbage)
        {
         FactData(theEnv)->factModifierError = FME_RETRACTED_ERROR;
         return NULL;
        }
     
      if (oldFact->whichDeftemplate->implied)
        {
         FactData(theEnv)->factModifierError = FME_IMPLIED_DEFTEMPLATE_ERROR;
         return NULL;
        }
        
      RetainFact(oldFact);
     }
     
   theFM = get_struct(theEnv,factModifier);
   theFM->fmEnv = theEnv;
   theFM->fmOldFact = oldFact;
      
   if ((oldFact == NULL) || (oldFact->whichDeftemplate->numberOfSlots == 0))
     {
      theFM->fmValueArray = NULL;
      theFM->changeMap = NULL;
     }
   else
     {
      theFM->fmValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * oldFact->whichDeftemplate->numberOfSlots);

      for (i = 0; i < oldFact->whichDeftemplate->numberOfSlots; i++)
        { theFM->fmValueArray[i].voidValue = VoidConstant(theEnv); }

      theFM->changeMap = (char *) gm2(theEnv,CountToBitMapSize(oldFact->whichDeftemplate->numberOfSlots));
      ClearBitString((void *) theFM->changeMap,CountToBitMapSize(oldFact->whichDeftemplate->numberOfSlots));
     }

   FactData(theEnv)->factModifierError = FME_NO_ERROR;
   return theFM;
  }

/*************************/
/* FMPutSlotCLIPSInteger */
/*************************/
PutSlotError FMPutSlotCLIPSInteger(
  FactModifier *theFM,
  const char *slotName,
  CLIPSInteger *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.integerValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/********************/
/* FMPutSlotInteger */
/********************/
PutSlotError FMPutSlotInteger(
  FactModifier *theFM,
  const char *slotName,
  long long longLongValue)
  {
   CLIPSValue theValue;
      
   if (theFM == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.integerValue = CreateInteger(theFM->fmEnv,longLongValue);
   return FMPutSlot(theFM,slotName,&theValue);
  }

/************************/
/* FMPutSlotCLIPSLexeme */
/************************/
PutSlotError FMPutSlotCLIPSLexeme(
  FactModifier *theFM,
  const char *slotName,
  CLIPSLexeme *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.lexemeValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/*******************/
/* FMPutSlotSymbol */
/*******************/
PutSlotError FMPutSlotSymbol(
  FactModifier *theFM,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
      
   if (theFM == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateSymbol(theFM->fmEnv,stringValue);
   return FMPutSlot(theFM,slotName,&theValue);
  }

/*******************/
/* FMPutSlotString */
/*******************/
PutSlotError FMPutSlotString(
  FactModifier *theFM,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
      
   if (theFM == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateString(theFM->fmEnv,stringValue);
   return FMPutSlot(theFM,slotName,&theValue);
  }

/*************************/
/* FMPutSlotInstanceName */
/*************************/
PutSlotError FMPutSlotInstanceName(
  FactModifier *theFM,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
      
   if (theFM == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateInstanceName(theFM->fmEnv,stringValue);
   return FMPutSlot(theFM,slotName,&theValue);
  }

/***********************/
/* FMPutSlotCLIPSFloat */
/***********************/
PutSlotError FMPutSlotCLIPSFloat(
  FactModifier *theFM,
  const char *slotName,
  CLIPSFloat *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.floatValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/******************/
/* FMPutSlotFloat */
/******************/
PutSlotError FMPutSlotFloat(
  FactModifier *theFM,
  const char *slotName,
  double floatValue)
  {
   CLIPSValue theValue;
      
   if (theFM == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.floatValue = CreateFloat(theFM->fmEnv,floatValue);
   return FMPutSlot(theFM,slotName,&theValue);
  }

/*****************/
/* FMPutSlotFact */
/*****************/
PutSlotError FMPutSlotFact(
  FactModifier *theFM,
  const char *slotName,
  Fact *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.factValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/*********************/
/* FMPutSlotInstance */
/*********************/
PutSlotError FMPutSlotInstance(
  FactModifier *theFM,
  const char *slotName,
  Instance *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.instanceValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/****************************/
/* FMPutSlotExternalAddress */
/****************************/
PutSlotError FMPutSlotExternalAddress(
  FactModifier *theFM,
  const char *slotName,
  CLIPSExternalAddress *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.externalAddressValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/***********************/
/* FMPutSlotMultifield */
/***********************/
PutSlotError FMPutSlotMultifield(
  FactModifier *theFM,
  const char *slotName,
  Multifield *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.multifieldValue = slotValue;
   return FMPutSlot(theFM,slotName,&theValue);
  }

/**************/
/* FMPutSlot: */
/**************/
PutSlotError FMPutSlot(
  FactModifier *theFM,
  const char *slotName,
  CLIPSValue *slotValue)
  {
   Environment *theEnv;
   struct templateSlot *theSlot;
   unsigned short whichSlot;
   CLIPSValue oldValue;
   CLIPSValue oldFactValue;
   int i;
   ConstraintViolationType cvType;
   
   /*==========================*/
   /* Check for NULL pointers. */
   /*==========================*/

   if ((theFM == NULL) || (slotName == NULL) || (slotValue == NULL))
     { return PSE_NULL_POINTER_ERROR; }
     
   if ((theFM->fmOldFact == NULL) || (slotValue->value == NULL))
     { return PSE_NULL_POINTER_ERROR; }
     
   theEnv = theFM->fmEnv;
   
   /*==================================*/
   /* Deleted facts can't be modified. */
   /*==================================*/
   
   if (theFM->fmOldFact->garbage)
     { return PSE_INVALID_TARGET_ERROR; }
     
   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   if ((theSlot = FindSlot(theFM->fmOldFact->whichDeftemplate,CreateSymbol(theEnv,slotName),&whichSlot)) == NULL)
     { return PSE_SLOT_NOT_FOUND_ERROR; }

   /*=============================================*/
   /* Make sure a single field value is not being */
   /* stored in a multifield slot or vice versa.  */
   /*=============================================*/

   if (((theSlot->multislot == 0) && (slotValue->header->type == MULTIFIELD_TYPE)) ||
       ((theSlot->multislot == 1) && (slotValue->header->type != MULTIFIELD_TYPE)))
     { return PSE_CARDINALITY_ERROR; }
   
   /*=================================*/
   /* Check constraints for the slot. */
   /*=================================*/
   
   if (theSlot->constraints != NULL)
     {
      if ((cvType = ConstraintCheckValue(theEnv,slotValue->header->type,slotValue->value,theSlot->constraints)) != NO_VIOLATION)
        {
         switch(cvType)
           {
            case NO_VIOLATION:
            case FUNCTION_RETURN_TYPE_VIOLATION:
              SystemError(theEnv,"FACTMNGR",3);
              ExitRouter(theEnv,EXIT_FAILURE);
              break;
              
            case TYPE_VIOLATION:
              return PSE_TYPE_ERROR;
              
            case RANGE_VIOLATION:
              return PSE_RANGE_ERROR;
              
            case ALLOWED_VALUES_VIOLATION:
              return PSE_ALLOWED_VALUES_ERROR;
              
            case CARDINALITY_VIOLATION:
              return PSE_CARDINALITY_ERROR;
            
            case ALLOWED_CLASSES_VIOLATION:
              return PSE_ALLOWED_CLASSES_ERROR;
           }
        }
     }

   /*===========================*/
   /* Set up the change arrays. */
   /*===========================*/
   
   if (theFM->fmValueArray == NULL)
     {
      theFM->fmValueArray = (CLIPSValue *) gm2(theFM->fmEnv,sizeof(CLIPSValue) * theFM->fmOldFact->whichDeftemplate->numberOfSlots);
      for (i = 0; i < theFM->fmOldFact->whichDeftemplate->numberOfSlots; i++)
        { theFM->fmValueArray[i].voidValue = theFM->fmEnv->VoidConstant; }
     }

   if (theFM->changeMap == NULL)
     {
      theFM->changeMap = (char *) gm2(theFM->fmEnv,CountToBitMapSize(theFM->fmOldFact->whichDeftemplate->numberOfSlots));
      ClearBitString((void *) theFM->changeMap,CountToBitMapSize(theFM->fmOldFact->whichDeftemplate->numberOfSlots));
     }

   /*=====================*/
   /* Set the slot value. */
   /*=====================*/

   oldValue.value = theFM->fmValueArray[whichSlot].value;
   oldFactValue.value = theFM->fmOldFact->theProposition.contents[whichSlot].value;

   if (oldFactValue.header->type == MULTIFIELD_TYPE)
     {
      if (MultifieldsEqual(oldFactValue.multifieldValue,slotValue->multifieldValue))
        {
         Release(theFM->fmEnv,oldValue.header);
         if (oldValue.header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theFM->fmEnv,oldValue.multifieldValue); }
         theFM->fmValueArray[whichSlot].voidValue = theFM->fmEnv->VoidConstant;
         ClearBitMap(theFM->changeMap,whichSlot);
         return PSE_NO_ERROR;
        }

      if (MultifieldsEqual(oldValue.multifieldValue,slotValue->multifieldValue))
        { return PSE_NO_ERROR; }
     }
   else
     {
      if (slotValue->value == oldFactValue.value)
        {
         Release(theFM->fmEnv,oldValue.header);
         theFM->fmValueArray[whichSlot].voidValue = theFM->fmEnv->VoidConstant;
         ClearBitMap(theFM->changeMap,whichSlot);
         return PSE_NO_ERROR;
        }
        
      if (oldValue.value == slotValue->value)
        { return PSE_NO_ERROR; }
     }

   SetBitMap(theFM->changeMap,whichSlot);

   Release(theFM->fmEnv,oldValue.header);

   if (oldValue.header->type == MULTIFIELD_TYPE)
     { ReturnMultifield(theFM->fmEnv,oldValue.multifieldValue); }
      
   if (slotValue->header->type == MULTIFIELD_TYPE)
     { theFM->fmValueArray[whichSlot].multifieldValue = CopyMultifield(theFM->fmEnv,slotValue->multifieldValue); }
   else
     { theFM->fmValueArray[whichSlot].value = slotValue->value; }

   Retain(theFM->fmEnv,theFM->fmValueArray[whichSlot].header);

   return PSE_NO_ERROR;
  }

/*************/
/* FMModify: */
/*************/
Fact *FMModify(
  FactModifier *theFM)
  {
   Environment *theEnv;
   Fact *rv;
   
   if (theFM == NULL)
     { return NULL; }
     
   theEnv = theFM->fmEnv;
   
   if (theFM->fmOldFact == NULL)
     {
      FactData(theEnv)->factModifierError = FME_NULL_POINTER_ERROR;
      return NULL;
     }

   if (theFM->fmOldFact->garbage)
     {
      FactData(theEnv)->factModifierError = FME_RETRACTED_ERROR;
      return NULL;
     }
     
   if (theFM->changeMap == NULL)
     { return theFM->fmOldFact; }
     
   if (! BitStringHasBitsSet(theFM->changeMap,CountToBitMapSize(theFM->fmOldFact->whichDeftemplate->numberOfSlots)))
     { return theFM->fmOldFact; }
     
   rv = ReplaceFact(theFM->fmEnv,theFM->fmOldFact,theFM->fmValueArray,theFM->changeMap);

   if ((FactData(theEnv)->assertError == AE_RULE_NETWORK_ERROR) ||
       (FactData(theEnv)->retractError == RE_RULE_NETWORK_ERROR))
     { FactData(theEnv)->factModifierError = FME_RULE_NETWORK_ERROR; }
   else if ((FactData(theEnv)->retractError == RE_COULD_NOT_RETRACT_ERROR) ||
            (FactData(theEnv)->assertError == AE_COULD_NOT_ASSERT_ERROR))
     { FactData(theEnv)->factModifierError = FME_COULD_NOT_MODIFY_ERROR; }
   else
     { FactData(theEnv)->factModifierError = FME_NO_ERROR; }

   FMAbort(theFM);
   
   if ((rv != NULL) && (rv != theFM->fmOldFact))
     {
      ReleaseFact(theFM->fmOldFact);
      theFM->fmOldFact = rv;
      RetainFact(theFM->fmOldFact);
     }
   
   return rv;
  }

/**************/
/* FMDispose: */
/**************/
void FMDispose(
  FactModifier *theFM)
  {
   GCBlock gcb;
   Environment *theEnv = theFM->fmEnv;
   int i;

   GCBlockStart(theEnv,&gcb);
   
   /*========================*/
   /* Clear the value array. */
   /*========================*/
   
   if (theFM->fmOldFact != NULL)
     {
      for (i = 0; i < theFM->fmOldFact->whichDeftemplate->numberOfSlots; i++)
        {
         Release(theEnv,theFM->fmValueArray[i].header);

         if (theFM->fmValueArray[i].header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theEnv,theFM->fmValueArray[i].multifieldValue); }
        }
     }
   
   /*=====================================*/
   /* Return the value and change arrays. */
   /*=====================================*/
   
   if (theFM->fmValueArray != NULL)
     { rm(theEnv,theFM->fmValueArray,sizeof(CLIPSValue) * theFM->fmOldFact->whichDeftemplate->numberOfSlots); }
      
   if (theFM->changeMap != NULL)
     { rm(theEnv,(void *) theFM->changeMap,CountToBitMapSize(theFM->fmOldFact->whichDeftemplate->numberOfSlots)); }

   /*====================================*/
   /* Return the FactModifier structure. */
   /*====================================*/
   
   if (theFM->fmOldFact != NULL)
     { ReleaseFact(theFM->fmOldFact); }
      
   rtn_struct(theEnv,factModifier,theFM);

   GCBlockEnd(theEnv,&gcb);
  }

/************/
/* FMAbort: */
/************/
void FMAbort(
  FactModifier *theFM)
  {
   GCBlock gcb;
   Environment *theEnv;
   unsigned int i;
   
   if (theFM == NULL) return;
   
   if (theFM->fmOldFact == NULL) return;

   theEnv = theFM->fmEnv;

   GCBlockStart(theEnv,&gcb);

   for (i = 0; i < theFM->fmOldFact->whichDeftemplate->numberOfSlots; i++)
     {
      Release(theEnv,theFM->fmValueArray[i].header);

      if (theFM->fmValueArray[i].header->type == MULTIFIELD_TYPE)
        { ReturnMultifield(theEnv,theFM->fmValueArray[i].multifieldValue); }
        
      theFM->fmValueArray[i].voidValue = theFM->fmEnv->VoidConstant;
     }
      
   if (theFM->changeMap != NULL)
     { ClearBitString((void *) theFM->changeMap,CountToBitMapSize(theFM->fmOldFact->whichDeftemplate->numberOfSlots)); }
     
   GCBlockEnd(theEnv,&gcb);
  }

/**************/
/* FMSetFact: */
/**************/
FactModifierError FMSetFact(
  FactModifier *theFM,
  Fact *oldFact)
  {
   Environment *theEnv;
   unsigned short currentSlotCount, newSlotCount;
   unsigned int i;
      
   if (theFM == NULL)
     { return FME_NULL_POINTER_ERROR; }
     
   theEnv = theFM->fmEnv;

   /*=================================================*/
   /* Modifiers can only be created for non-retracted */
   /* deftemplate facts with at least one slot.       */
   /*=================================================*/
   
   if (oldFact != NULL)
     {
      if (oldFact->garbage)
        {
         FactData(theEnv)->factModifierError = FME_RETRACTED_ERROR;
         return FME_RETRACTED_ERROR;
        }
      
      if (oldFact->whichDeftemplate->implied)
        {
         FactData(theEnv)->factModifierError = FME_IMPLIED_DEFTEMPLATE_ERROR;
         return FME_IMPLIED_DEFTEMPLATE_ERROR;
        }
     }
     
   /*========================*/
   /* Clear the value array. */
   /*========================*/
   
   if (theFM->fmValueArray != NULL)
     {
      for (i = 0; i < theFM->fmOldFact->whichDeftemplate->numberOfSlots; i++)
        {
         Release(theEnv,theFM->fmValueArray[i].header);

         if (theFM->fmValueArray[i].header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theEnv,theFM->fmValueArray[i].multifieldValue); }
        }
     }

   /*==================================================*/
   /* Resize the value and change arrays if necessary. */
   /*==================================================*/

   if (theFM->fmOldFact == NULL)
     { currentSlotCount = 0; }
   else
     { currentSlotCount = theFM->fmOldFact->whichDeftemplate->numberOfSlots; }
   
   if (oldFact == NULL)
     { newSlotCount = 0; }
   else
     { newSlotCount = oldFact->whichDeftemplate->numberOfSlots; }
     
   if (newSlotCount != currentSlotCount)
     {
      if (theFM->fmValueArray != NULL)
        { rm(theEnv,theFM->fmValueArray,sizeof(CLIPSValue) * currentSlotCount); }
      
      if (theFM->changeMap != NULL)
        { rm(theEnv,(void *) theFM->changeMap,currentSlotCount); }
        
      if (newSlotCount == 0)
        {
         theFM->fmValueArray = NULL;
         theFM->changeMap = NULL;
        }
      else
        {
         theFM->fmValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * newSlotCount);
         theFM->changeMap = (char *) gm2(theEnv,CountToBitMapSize(newSlotCount));
        }
     }

   /*=================================*/
   /* Update the fact being modified. */
   /*=================================*/
   
   RetainFact(oldFact);
   ReleaseFact(theFM->fmOldFact);
   theFM->fmOldFact = oldFact;

   /*=========================================*/
   /* Initialize the value and change arrays. */
   /*=========================================*/
   
   for (i = 0; i < newSlotCount; i++)
     { theFM->fmValueArray[i].voidValue = theFM->fmEnv->VoidConstant; }
   
   if (newSlotCount != 0)
     { ClearBitString((void *) theFM->changeMap,CountToBitMapSize(newSlotCount)); }

   /*================================================================*/
   /* Return true to indicate the modifier was successfully created. */
   /*================================================================*/
      
   FactData(theEnv)->factModifierError = FME_NO_ERROR;
   return FME_NO_ERROR;
  }

/************/
/* FMError: */
/************/
FactModifierError FMError(
  Environment *theEnv)
  {
   return FactData(theEnv)->factModifierError;
  }

#endif /* DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT */

