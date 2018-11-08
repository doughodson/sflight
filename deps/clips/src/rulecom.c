   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/10/18             */
   /*                                                     */
   /*                RULE COMMANDS MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides the matches command. Also provides the  */
/*   the developer commands show-joins and rule-complexity.  */
/*   Also provides the initialization routine which          */
/*   registers rule commands found in other modules.         */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed CONFLICT_RESOLUTION_STRATEGIES         */
/*            INCREMENTAL_RESET, and LOGICAL_DEPENDENCIES    */
/*            compilation flags.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Added support for hashed memories.             */
/*                                                           */
/*            Improvements to matches command.               */
/*                                                           */
/*            Add join-activity and join-activity-reset      */
/*            commands.                                      */
/*                                                           */
/*            Added get-beta-memory-resizing and             */
/*            set-beta-memory-resizing functions.            */
/*                                                           */
/*            Added timetag function.                        */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*      6.31: Fixes for show-joins command.                  */
/*                                                           */
/*            Fixes for matches command where the            */
/*            activations listed were not correct if the     */
/*            current module was different than the module   */
/*            for the specified rule.                        */
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
/*            Incremental reset is always enabled.           */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <string.h>

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include "argacces.h"
#include "constant.h"
#include "constrct.h"
#include "crstrtgy.h"
#include "engine.h"
#include "envrnmnt.h"
#include "evaluatn.h"
#include "extnfunc.h"
#include "incrrset.h"
#include "lgcldpnd.h"
#include "memalloc.h"
#include "multifld.h"
#include "pattern.h"
#include "prntutil.h"
#include "reteutil.h"
#include "router.h"
#include "ruledlt.h"
#include "sysdep.h"
#include "utility.h"
#include "watch.h"

#if BLOAD || BLOAD_AND_BSAVE || BLOAD_ONLY
#include "rulebin.h"
#endif

#include "rulecom.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if DEVELOPER
   static void                    ShowJoins(Environment *,Defrule *);
#endif
#if DEBUGGING_FUNCTIONS
   static long long               ListAlphaMatches(Environment *,struct joinInformation *,int);
   static long long               ListBetaMatches(Environment *,struct joinInformation *,long,unsigned short,int);
   static void                    ListBetaJoinActivity(Environment *,struct joinInformation *,long,long,int,UDFValue *);
   static unsigned short          AlphaJoinCountDriver(Environment *,struct joinNode *);
   static unsigned short          BetaJoinCountDriver(Environment *,struct joinNode *);
   static void                    AlphaJoinsDriver(Environment *,struct joinNode *,unsigned short,struct joinInformation *);
   static void                    BetaJoinsDriver(Environment *,struct joinNode *,unsigned short,struct joinInformation *,struct betaMemory *,struct joinNode *);
   static int                     CountPatterns(Environment *,struct joinNode *,bool);
   static const char             *BetaHeaderString(Environment *,struct joinInformation *,long,long);
   static const char             *ActivityHeaderString(Environment *,struct joinInformation *,long,long);
   static void                    JoinActivityReset(Environment *,ConstructHeader *,void *);
#endif

/****************************************************************/
/* DefruleCommands: Initializes defrule commands and functions. */
/****************************************************************/
void DefruleCommands(
  Environment *theEnv)
  {
#if ! RUN_TIME
   AddUDF(theEnv,"run","v",0,1,"l",RunCommand,"RunCommand",NULL);
   AddUDF(theEnv,"halt","v",0,0,NULL,HaltCommand,"HaltCommand",NULL);
   AddUDF(theEnv,"focus","b",1,UNBOUNDED,"y",FocusCommand,"FocusCommand",NULL);
   AddUDF(theEnv,"clear-focus-stack","v",0,0,NULL,ClearFocusStackCommand,"ClearFocusStackCommand",NULL);
   AddUDF(theEnv,"get-focus-stack","m",0,0,NULL,GetFocusStackFunction,"GetFocusStackFunction",NULL);
   AddUDF(theEnv,"pop-focus","y",0,0,NULL,PopFocusFunction,"PopFocusFunction",NULL);
   AddUDF(theEnv,"get-focus","y",0,0,NULL,GetFocusFunction,"GetFocusFunction",NULL);
#if DEBUGGING_FUNCTIONS
   AddUDF(theEnv,"set-break","v",1,1,"y",SetBreakCommand,"SetBreakCommand",NULL);
   AddUDF(theEnv,"remove-break","v",0,1,"y",RemoveBreakCommand,"RemoveBreakCommand",NULL);
   AddUDF(theEnv,"show-breaks","v",0,1,"y",ShowBreaksCommand,"ShowBreaksCommand",NULL);
   AddUDF(theEnv,"matches","bm",1,2,"y",MatchesCommand,"MatchesCommand",NULL);
   AddUDF(theEnv,"join-activity","bm",1,2,"y",JoinActivityCommand,"JoinActivityCommand",NULL);
   AddUDF(theEnv,"join-activity-reset","v",0,0,NULL,JoinActivityResetCommand,"JoinActivityResetCommand",NULL);
   AddUDF(theEnv,"list-focus-stack","v",0,0,NULL,ListFocusStackCommand,"ListFocusStackCommand",NULL);
   AddUDF(theEnv,"dependencies","v",1,1,"infly",DependenciesCommand,"DependenciesCommand",NULL);
   AddUDF(theEnv,"dependents","v",1,1,"infly",DependentsCommand,"DependentsCommand",NULL);

   AddUDF(theEnv,"timetag","l",1,1,"infly" ,TimetagFunction,"TimetagFunction",NULL);
#endif /* DEBUGGING_FUNCTIONS */

   AddUDF(theEnv,"get-beta-memory-resizing","b",0,0,NULL,GetBetaMemoryResizingCommand,"GetBetaMemoryResizingCommand",NULL);
   AddUDF(theEnv,"set-beta-memory-resizing","b",1,1,NULL,SetBetaMemoryResizingCommand,"SetBetaMemoryResizingCommand",NULL);

   AddUDF(theEnv,"get-strategy","y",0,0,NULL,GetStrategyCommand,"GetStrategyCommand",NULL);
   AddUDF(theEnv,"set-strategy","y",1,1,"y",SetStrategyCommand,"SetStrategyCommand",NULL);

#if DEVELOPER && (! BLOAD_ONLY)
   AddUDF(theEnv,"rule-complexity","l",1,1,"y",RuleComplexityCommand,"RuleComplexityCommand",NULL);
   AddUDF(theEnv,"show-joins","v",1,1,"y",ShowJoinsCommand,"ShowJoinsCommand",NULL);
   AddUDF(theEnv,"show-aht","v",0,0,NULL,ShowAlphaHashTable,"ShowAlphaHashTable",NULL);
#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"rule-analysis",0,&DefruleData(theEnv)->WatchRuleAnalysis,0,NULL,NULL);
#endif
#endif /* DEVELOPER && (! BLOAD_ONLY) */

#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif /* ! RUN_TIME */
  }

/***********************************************/
/* GetBetaMemoryResizing: C access routine     */
/*   for the get-beta-memory-resizing command. */
/***********************************************/
bool GetBetaMemoryResizing(
  Environment *theEnv)
  {
   return DefruleData(theEnv)->BetaMemoryResizingFlag;
  }

/***********************************************/
/* SetBetaMemoryResizing: C access routine     */
/*   for the set-beta-memory-resizing command. */
/***********************************************/
bool SetBetaMemoryResizing(
  Environment *theEnv,
  bool value)
  {
   bool ov;

   ov = DefruleData(theEnv)->BetaMemoryResizingFlag;

   DefruleData(theEnv)->BetaMemoryResizingFlag = value;

   return(ov);
  }

/****************************************************/
/* SetBetaMemoryResizingCommand: H/L access routine */
/*   for the set-beta-memory-resizing command.      */
/****************************************************/
void SetBetaMemoryResizingCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;

   returnValue->lexemeValue = CreateBoolean(theEnv,GetBetaMemoryResizing(theEnv));

   /*=================================================*/
   /* The symbol FALSE disables beta memory resizing. */
   /* Any other value enables beta memory resizing.   */
   /*=================================================*/

   if (! UDFFirstArgument(context,ANY_TYPE_BITS,&theArg))
     { return; }

   if (theArg.value == FalseSymbol(theEnv))
     { SetBetaMemoryResizing(theEnv,false); }
   else
     { SetBetaMemoryResizing(theEnv,true); }
  }

/****************************************************/
/* GetBetaMemoryResizingCommand: H/L access routine */
/*   for the get-beta-memory-resizing command.      */
/****************************************************/
void GetBetaMemoryResizingCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   returnValue->lexemeValue = CreateBoolean(theEnv,GetBetaMemoryResizing(theEnv));
  }

/******************************************/
/* GetFocusFunction: H/L access routine   */
/*   for the get-focus function.          */
/******************************************/
void GetFocusFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Defmodule *rv;

   rv = GetFocus(theEnv);

   if (rv == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   returnValue->value = rv->header.name;
  }

/*********************************/
/* GetFocus: C access routine    */
/*   for the get-focus function. */
/*********************************/
Defmodule *GetFocus(
  Environment *theEnv)
  {
   if (EngineData(theEnv)->CurrentFocus == NULL) return NULL;

   return EngineData(theEnv)->CurrentFocus->theModule;
  }

#if DEBUGGING_FUNCTIONS

/****************************************/
/* MatchesCommand: H/L access routine   */
/*   for the matches command.           */
/****************************************/
void MatchesCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *ruleName, *argument;
   Defrule *rulePtr;
   UDFValue theArg;
   Verbosity output;
   CLIPSValue result;

   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg))
     { return; }

   ruleName = theArg.lexemeValue->contents;

   rulePtr = FindDefrule(theEnv,ruleName);
   if (rulePtr == NULL)
     {
      CantFindItemErrorMessage(theEnv,"defrule",ruleName,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   if (UDFHasNextArgument(context))
     {
      if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
        { return; }

      argument = theArg.lexemeValue->contents;
      if (strcmp(argument,"verbose") == 0)
        { output = VERBOSE; }
      else if (strcmp(argument,"succinct") == 0)
        { output = SUCCINCT; }
      else if (strcmp(argument,"terse") == 0)
        { output = TERSE; }
      else
        {
         UDFInvalidArgumentMessage(context,"symbol with value verbose, succinct, or terse");
         returnValue->lexemeValue = FalseSymbol(theEnv);
         return;
        }
     }
   else
     { output = VERBOSE; }

   Matches(rulePtr,output,&result);
   CLIPSToUDFValue(&result,returnValue);
  }

/******************************/
/* Matches: C access routine  */
/*   for the matches command. */
/******************************/
void Matches(
  Defrule *theDefrule,
  Verbosity output,
  CLIPSValue *returnValue)
  {
   Defrule *rulePtr;
   Defrule *topDisjunct = theDefrule;
   long joinIndex;
   unsigned short arraySize;
   struct joinInformation *theInfo;
   long long alphaMatchCount = 0;
   long long betaMatchCount = 0;
   long long activations = 0;
   Activation *agendaPtr;
   Environment *theEnv = theDefrule->header.env;

   /*==========================*/
   /* Set up the return value. */
   /*==========================*/

   returnValue->value = CreateMultifield(theEnv,3L);

   returnValue->multifieldValue->contents[0].integerValue = SymbolData(theEnv)->Zero;
   returnValue->multifieldValue->contents[1].integerValue = SymbolData(theEnv)->Zero;
   returnValue->multifieldValue->contents[2].integerValue = SymbolData(theEnv)->Zero;

   /*=================================================*/
   /* Loop through each of the disjuncts for the rule */
   /*=================================================*/

   for (rulePtr = topDisjunct; rulePtr != NULL; rulePtr = rulePtr->disjunct)
     {
      /*===============================================*/
      /* Create the array containing the list of alpha */
      /* join nodes (those connected to a pattern CE). */
      /*===============================================*/

      arraySize = AlphaJoinCount(theEnv,rulePtr);

      theInfo = CreateJoinArray(theEnv,arraySize);

      AlphaJoins(theEnv,rulePtr,arraySize,theInfo);

      /*=========================*/
      /* List the alpha matches. */
      /*=========================*/

      for (joinIndex = 0; joinIndex < arraySize; joinIndex++)
        {
         alphaMatchCount += ListAlphaMatches(theEnv,&theInfo[joinIndex],output);
         returnValue->multifieldValue->contents[0].integerValue = CreateInteger(theEnv,alphaMatchCount);
        }

      /*================================*/
      /* Free the array of alpha joins. */
      /*================================*/

      FreeJoinArray(theEnv,theInfo,arraySize);

      /*==============================================*/
      /* Create the array containing the list of beta */
      /* join nodes (joins from the right plus joins  */
      /* connected to a pattern CE).                  */
      /*==============================================*/

      arraySize = BetaJoinCount(theEnv,rulePtr);

      theInfo = CreateJoinArray(theEnv,arraySize);

      BetaJoins(theEnv,rulePtr,arraySize,theInfo);

      /*======================================*/
      /* List the beta matches (for all joins */
      /* except the first pattern CE).        */
      /*======================================*/

      for (joinIndex = 1; joinIndex < arraySize; joinIndex++)
        {
         betaMatchCount += ListBetaMatches(theEnv,theInfo,joinIndex,arraySize,output);
         returnValue->multifieldValue->contents[1].integerValue = CreateInteger(theEnv,betaMatchCount);
        }

      /*================================*/
      /* Free the array of alpha joins. */
      /*================================*/

      FreeJoinArray(theEnv,theInfo,arraySize);
     }

   /*===================*/
   /* List activations. */
   /*===================*/

   if (output == VERBOSE)
     { WriteString(theEnv,STDOUT,"Activations\n"); }

   for (agendaPtr = ((struct defruleModule *) topDisjunct->header.whichModule)->agenda;
        agendaPtr != NULL;
        agendaPtr = (struct activation *) GetNextActivation(theEnv,agendaPtr))
     {
      if (GetHaltExecution(theEnv) == true) return;

      if (((struct activation *) agendaPtr)->theRule->header.name == topDisjunct->header.name)
        {
         activations++;

         if (output == VERBOSE)
           {
            PrintPartialMatch(theEnv,STDOUT,GetActivationBasis(theEnv,agendaPtr));
            WriteString(theEnv,STDOUT,"\n");
           }
        }
     }

   if (output == SUCCINCT)
     {
      WriteString(theEnv,STDOUT,"Activations: ");
      WriteInteger(theEnv,STDOUT,activations);
      WriteString(theEnv,STDOUT,"\n");
     }

   if ((activations == 0) && (output == VERBOSE)) WriteString(theEnv,STDOUT," None\n");

   returnValue->multifieldValue->contents[2].integerValue = CreateInteger(theEnv,activations);
  }

/****************************************************/
/* AlphaJoinCountDriver: Driver routine to iterate  */
/*   over a rule's joins to determine the number of */
/*   alpha joins.                                   */
/****************************************************/
static unsigned short AlphaJoinCountDriver(
  Environment *theEnv,
  struct joinNode *theJoin)
  {
   unsigned short alphaCount = 0;

   if (theJoin == NULL)
     { return alphaCount; }

   if (theJoin->joinFromTheRight)
     { return AlphaJoinCountDriver(theEnv,(struct joinNode *) theJoin->rightSideEntryStructure); }
   else if (theJoin->lastLevel != NULL)
     { alphaCount += AlphaJoinCountDriver(theEnv,theJoin->lastLevel); }

   alphaCount++;

   return alphaCount;
  }

/***********************************************/
/* AlphaJoinCount: Returns the number of alpha */
/*   joins associated with the specified rule. */
/***********************************************/
unsigned short AlphaJoinCount(
  Environment *theEnv,
  Defrule *theDefrule)
  {
   return AlphaJoinCountDriver(theEnv,theDefrule->lastJoin->lastLevel);
  }

/***************************************/
/* AlphaJoinsDriver: Driver routine to */
/*   retrieve a rule's alpha joins.    */
/***************************************/
static void AlphaJoinsDriver(
  Environment *theEnv,
  struct joinNode *theJoin,
  unsigned short alphaIndex,
  struct joinInformation *theInfo)
  {
   if (theJoin == NULL)
     { return; }

   if (theJoin->joinFromTheRight)
     {
      AlphaJoinsDriver(theEnv,(struct joinNode *) theJoin->rightSideEntryStructure,alphaIndex,theInfo);
      return;
     }
   else if (theJoin->lastLevel != NULL)
     { AlphaJoinsDriver(theEnv,theJoin->lastLevel,alphaIndex-1,theInfo); }

   theInfo[alphaIndex-1].whichCE = alphaIndex;
   theInfo[alphaIndex-1].theJoin = theJoin;

   return;
  }

/*****************************************/
/* AlphaJoins: Retrieves the alpha joins */
/*   associated with the specified rule. */
/*****************************************/
void AlphaJoins(
  Environment *theEnv,
  Defrule *theDefrule,
  unsigned short alphaCount,
  struct joinInformation *theInfo)
  {
   AlphaJoinsDriver(theEnv,theDefrule->lastJoin->lastLevel,alphaCount,theInfo);
  }

/****************************************************/
/* BetaJoinCountDriver: Driver routine to iterate  */
/*   over a rule's joins to determine the number of */
/*   beta joins.                                   */
/****************************************************/
static unsigned short BetaJoinCountDriver(
  Environment *theEnv,
  struct joinNode *theJoin)
  {
   unsigned short betaCount = 0;

   if (theJoin == NULL)
     { return betaCount; }

   betaCount++;

   if (theJoin->joinFromTheRight)
     { betaCount += BetaJoinCountDriver(theEnv,(struct joinNode *) theJoin->rightSideEntryStructure); }
   else if (theJoin->lastLevel != NULL)
     { betaCount += BetaJoinCountDriver(theEnv,theJoin->lastLevel); }

   return betaCount;
  }

/***********************************************/
/* BetaJoinCount: Returns the number of beta   */
/*   joins associated with the specified rule. */
/***********************************************/
unsigned short BetaJoinCount(
  Environment *theEnv,
  Defrule *theDefrule)
  {
   return BetaJoinCountDriver(theEnv,theDefrule->lastJoin->lastLevel);
  }

/**************************************/
/* BetaJoinsDriver: Driver routine to */
/*   retrieve a rule's beta joins.    */
/**************************************/
static void BetaJoinsDriver(
  Environment *theEnv,
  struct joinNode *theJoin,
  unsigned short betaIndex,
  struct joinInformation *theJoinInfoArray,
  struct betaMemory *lastMemory,
  struct joinNode *nextJoin)
  {
   unsigned short theCE = 0;
   int theCount;
   struct joinNode *tmpPtr;

   if (theJoin == NULL)
     { return; }

   theJoinInfoArray[betaIndex-1].theJoin = theJoin;
   theJoinInfoArray[betaIndex-1].theMemory = lastMemory;
   theJoinInfoArray[betaIndex-1].nextJoin = nextJoin;

   /*===================================*/
   /* Determine the conditional element */
   /* index for this join.              */
   /*===================================*/

   for (tmpPtr = theJoin; tmpPtr != NULL; tmpPtr = tmpPtr->lastLevel)
      { theCE++; }

   theJoinInfoArray[betaIndex-1].whichCE = theCE;

   /*==============================================*/
   /* The end pattern in the range of patterns for */
   /* this join is always the number of patterns   */
   /* remaining to be encountered.                 */
   /*==============================================*/

   theCount = CountPatterns(theEnv,theJoin,true);
   theJoinInfoArray[betaIndex-1].patternEnd = theCount;

   /*========================================================*/
   /* Determine where the block of patterns for a CE begins. */
   /*========================================================*/


   theCount = CountPatterns(theEnv,theJoin,false);
   theJoinInfoArray[betaIndex-1].patternBegin = theCount;

   /*==========================*/
   /* Find the next beta join. */
   /*==========================*/

   if (theJoin->joinFromTheRight)
     { BetaJoinsDriver(theEnv,(struct joinNode *) theJoin->rightSideEntryStructure,betaIndex-1,theJoinInfoArray,theJoin->rightMemory,theJoin); }
   else if (theJoin->lastLevel != NULL)
     { BetaJoinsDriver(theEnv,theJoin->lastLevel,betaIndex-1,theJoinInfoArray,theJoin->leftMemory,theJoin); }

   return;
  }

/*****************************************/
/* BetaJoins: Retrieves the beta joins   */
/*   associated with the specified rule. */
/*****************************************/
void BetaJoins(
  Environment *theEnv,
  Defrule *theDefrule,
  unsigned short betaArraySize,
  struct joinInformation *theInfo)
  {
   BetaJoinsDriver(theEnv,theDefrule->lastJoin->lastLevel,betaArraySize,theInfo,theDefrule->lastJoin->leftMemory,theDefrule->lastJoin);
  }

/***********************************************/
/* CreateJoinArray: Creates a join information */
/*    array of the specified size.             */
/***********************************************/
struct joinInformation *CreateJoinArray(
   Environment *theEnv,
   unsigned short size)
   {
    if (size == 0) return NULL;

    return (struct joinInformation *) genalloc(theEnv,sizeof(struct joinInformation) * size);
   }

/*******************************************/
/* FreeJoinArray: Frees a join information */
/*    array of the specified size.         */
/*******************************************/
void FreeJoinArray(
   Environment *theEnv,
   struct joinInformation *theArray,
   unsigned short size)
   {
    if (size == 0) return;

    genfree(theEnv,theArray,sizeof(struct joinInformation) * size);
   }

/*********************/
/* ListAlphaMatches: */
/*********************/
static long long ListAlphaMatches(
  Environment *theEnv,
  struct joinInformation *theInfo,
  int output)
  {
   struct alphaMemoryHash *listOfHashNodes;
   struct partialMatch *listOfMatches;
   long long count;
   struct joinNode *theJoin;
   long long alphaCount = 0;

   if (GetHaltExecution(theEnv) == true)
     { return(alphaCount); }

   theJoin = theInfo->theJoin;

   if (output == VERBOSE)
     {
      WriteString(theEnv,STDOUT,"Matches for Pattern ");
      WriteInteger(theEnv,STDOUT,theInfo->whichCE);
      WriteString(theEnv,STDOUT,"\n");
     }

   if (theJoin->rightSideEntryStructure == NULL)
     {
      if (theJoin->rightMemory->beta[0]->children != NULL)
        { alphaCount += 1; }

      if (output == VERBOSE)
        {
         if (theJoin->rightMemory->beta[0]->children != NULL)
           { WriteString(theEnv,STDOUT,"*\n"); }
         else
           { WriteString(theEnv,STDOUT," None\n"); }
        }
      else if (output == SUCCINCT)
        {
         WriteString(theEnv,STDOUT,"Pattern ");
         WriteInteger(theEnv,STDOUT,theInfo->whichCE);
         WriteString(theEnv,STDOUT,": ");

         if (theJoin->rightMemory->beta[0]->children != NULL)
           { WriteString(theEnv,STDOUT,"1"); }
         else
           { WriteString(theEnv,STDOUT,"0"); }
         WriteString(theEnv,STDOUT,"\n");
        }

      return(alphaCount);
     }

   listOfHashNodes =  ((struct patternNodeHeader *) theJoin->rightSideEntryStructure)->firstHash;

   for (count = 0;
        listOfHashNodes != NULL;
        listOfHashNodes = listOfHashNodes->nextHash)
     {
      listOfMatches = listOfHashNodes->alphaMemory;

      while (listOfMatches != NULL)
        {
         if (GetHaltExecution(theEnv) == true)
           { return(alphaCount); }

         count++;
         if (output == VERBOSE)
           {
            PrintPartialMatch(theEnv,STDOUT,listOfMatches);
            WriteString(theEnv,STDOUT,"\n");
           }
         listOfMatches = listOfMatches->nextInMemory;
        }
     }

   alphaCount += count;

   if ((count == 0) && (output == VERBOSE)) WriteString(theEnv,STDOUT," None\n");

   if (output == SUCCINCT)
     {
      WriteString(theEnv,STDOUT,"Pattern ");
      WriteInteger(theEnv,STDOUT,theInfo->whichCE);
      WriteString(theEnv,STDOUT,": ");
      WriteInteger(theEnv,STDOUT,count);
      WriteString(theEnv,STDOUT,"\n");
     }

   return(alphaCount);
  }

/********************/
/* BetaHeaderString */
/********************/
static const char *BetaHeaderString(
  Environment *theEnv,
  struct joinInformation *infoArray,
  long joinIndex,
  long arraySize)
  {
   struct joinNode *theJoin;
   struct joinInformation *theInfo;
   long i, j, startPosition, endPosition, positionsToPrint = 0;
   bool nestedCEs = false;
   const char *returnString = "";
   long lastIndex;
   char buffer[32];

   /*=============================================*/
   /* Determine which joins need to be traversed. */
   /*=============================================*/

   for (i = 0; i < arraySize; i++)
     { infoArray[i].marked = false; }

   theInfo = &infoArray[joinIndex];
   theJoin = theInfo->theJoin;
   lastIndex = joinIndex;

   while (theJoin != NULL)
     {
      for (i = lastIndex; i >= 0; i--)
        {
         if (infoArray[i].theJoin == theJoin)
           {
            positionsToPrint++;
            infoArray[i].marked = true;
            if (infoArray[i].patternBegin != infoArray[i].patternEnd)
              { nestedCEs = true; }
            lastIndex = i - 1;
            break;
           }
        }
      theJoin = theJoin->lastLevel;
     }

   for (i = 0; i <= joinIndex; i++)
     {
      if (infoArray[i].marked == false) continue;

      positionsToPrint--;
      startPosition = i;
      endPosition = i;

      if (infoArray[i].patternBegin == infoArray[i].patternEnd)
        {
         for (j = i + 1; j <= joinIndex; j++)
           {
            if (infoArray[j].marked == false) continue;

            if (infoArray[j].patternBegin != infoArray[j].patternEnd) break;

            positionsToPrint--;
            i = j;
            endPosition = j;
           }
        }

      theInfo = &infoArray[startPosition];

      gensprintf(buffer,"%d",theInfo->whichCE);
      returnString = AppendStrings(theEnv,returnString,buffer);

      if (nestedCEs)
        {
         if (theInfo->patternBegin == theInfo->patternEnd)
           {
            returnString = AppendStrings(theEnv,returnString," (P");
            gensprintf(buffer,"%d",theInfo->patternBegin);
            returnString = AppendStrings(theEnv,returnString,buffer);
            returnString = AppendStrings(theEnv,returnString,")");
           }
         else
           {
            returnString = AppendStrings(theEnv,returnString," (P");
            gensprintf(buffer,"%d",theInfo->patternBegin);
            returnString = AppendStrings(theEnv,returnString,buffer);
            returnString = AppendStrings(theEnv,returnString," - P");
            gensprintf(buffer,"%d",theInfo->patternEnd);
            returnString = AppendStrings(theEnv,returnString,buffer);
            returnString = AppendStrings(theEnv,returnString,")");
           }
        }

      if (startPosition != endPosition)
        {
         theInfo = &infoArray[endPosition];

         returnString = AppendStrings(theEnv,returnString," - ");
         gensprintf(buffer,"%d",theInfo->whichCE);
         returnString = AppendStrings(theEnv,returnString,buffer);

         if (nestedCEs)
           {
            if (theInfo->patternBegin == theInfo->patternEnd)
              {
               returnString = AppendStrings(theEnv,returnString," (P");
               gensprintf(buffer,"%d",theInfo->patternBegin);
               returnString = AppendStrings(theEnv,returnString,buffer);
               returnString = AppendStrings(theEnv,returnString,")");
              }
            else
              {
               returnString = AppendStrings(theEnv,returnString," (P");
               gensprintf(buffer,"%d",theInfo->patternBegin);
               returnString = AppendStrings(theEnv,returnString,buffer);
               returnString = AppendStrings(theEnv,returnString," - P");
               gensprintf(buffer,"%d",theInfo->patternEnd);
               returnString = AppendStrings(theEnv,returnString,buffer);
               returnString = AppendStrings(theEnv,returnString,")");
              }
           }
        }

      if (positionsToPrint > 0)
        { returnString = AppendStrings(theEnv,returnString," , "); }
     }

   return returnString;
  }

/********************/
/* ListBetaMatches: */
/********************/
static long long ListBetaMatches(
  Environment *theEnv,
  struct joinInformation *infoArray,
  long joinIndex,
  unsigned short arraySize,
  int output)
  {
   long betaCount = 0;
   struct joinInformation *theInfo;
   unsigned long count;

   if (GetHaltExecution(theEnv) == true)
     { return(betaCount); }

   theInfo = &infoArray[joinIndex];

   if (output == VERBOSE)
     {
      WriteString(theEnv,STDOUT,"Partial matches for CEs ");
      WriteString(theEnv,STDOUT,
                     BetaHeaderString(theEnv,infoArray,joinIndex,arraySize));
      WriteString(theEnv,STDOUT,"\n");
     }

   count = PrintBetaMemory(theEnv,STDOUT,theInfo->theMemory,true,"",output);

   betaCount += count;

   if ((output == VERBOSE) && (count == 0))
     { WriteString(theEnv,STDOUT," None\n"); }
   else if (output == SUCCINCT)
     {
      WriteString(theEnv,STDOUT,"CEs ");
      WriteString(theEnv,STDOUT,
                     BetaHeaderString(theEnv,infoArray,joinIndex,arraySize));
      WriteString(theEnv,STDOUT,": ");
      WriteInteger(theEnv,STDOUT,betaCount);
      WriteString(theEnv,STDOUT,"\n");
     }

   return betaCount;
  }

/******************/
/* CountPatterns: */
/******************/
static int CountPatterns(
  Environment *theEnv,
  struct joinNode *theJoin,
  bool followRight)
  {
   int theCount = 0;

   if (theJoin == NULL) return theCount;

   if (theJoin->joinFromTheRight && (followRight == false))
     { theCount++; }

   while (theJoin != NULL)
     {
      if (theJoin->joinFromTheRight)
        {
         if (followRight)
           { theJoin = (struct joinNode *) theJoin->rightSideEntryStructure; }
         else
           { theJoin = theJoin->lastLevel; }
        }
      else
        {
         theCount++;
         theJoin = theJoin->lastLevel;
        }

      followRight = true;
     }

   return theCount;
  }

/*******************************************/
/* JoinActivityCommand: H/L access routine */
/*   for the join-activity command.        */
/*******************************************/
void JoinActivityCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *ruleName, *argument;
   Defrule *rulePtr;
   UDFValue theArg;
   int output;

   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg))
     { return; }

   ruleName = theArg.lexemeValue->contents;

   rulePtr = FindDefrule(theEnv,ruleName);
   if (rulePtr == NULL)
     {
      CantFindItemErrorMessage(theEnv,"defrule",ruleName,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   if (UDFHasNextArgument(context))
     {
      if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
        { return; }

      argument = theArg.lexemeValue->contents;
      if (strcmp(argument,"verbose") == 0)
        { output = VERBOSE; }
      else if (strcmp(argument,"succinct") == 0)
        { output = SUCCINCT; }
      else if (strcmp(argument,"terse") == 0)
        { output = TERSE; }
      else
        {
         UDFInvalidArgumentMessage(context,"symbol with value verbose, succinct, or terse");
         returnValue->lexemeValue = FalseSymbol(theEnv);
         return;
        }
     }
   else
     { output = VERBOSE; }

   JoinActivity(theEnv,rulePtr,output,returnValue);
  }

/************************************/
/* JoinActivity: C access routine   */
/*   for the join-activity command. */
/************************************/
void JoinActivity(
  Environment *theEnv,
  Defrule *theRule,
  int output,
  UDFValue *returnValue)
  {
   Defrule *rulePtr;
   long disjunctCount, disjunctIndex, joinIndex;
   unsigned short arraySize;
   struct joinInformation *theInfo;

   /*==========================*/
   /* Set up the return value. */
   /*==========================*/

   returnValue->begin = 0;
   returnValue->range = 3;
   returnValue->value = CreateMultifield(theEnv,3L);

   returnValue->multifieldValue->contents[0].integerValue = SymbolData(theEnv)->Zero;
   returnValue->multifieldValue->contents[1].integerValue = SymbolData(theEnv)->Zero;
   returnValue->multifieldValue->contents[2].integerValue = SymbolData(theEnv)->Zero;

   /*=================================================*/
   /* Loop through each of the disjuncts for the rule */
   /*=================================================*/

   disjunctCount = GetDisjunctCount(theEnv,theRule);

   for (disjunctIndex = 1; disjunctIndex <= disjunctCount; disjunctIndex++)
     {
      rulePtr = GetNthDisjunct(theEnv,theRule,disjunctIndex);

      /*==============================================*/
      /* Create the array containing the list of beta */
      /* join nodes (joins from the right plus joins  */
      /* connected to a pattern CE).                  */
      /*==============================================*/

      arraySize = BetaJoinCount(theEnv,rulePtr);

      theInfo = CreateJoinArray(theEnv,arraySize);

      BetaJoins(theEnv,rulePtr,arraySize,theInfo);

      /*======================================*/
      /* List the beta matches (for all joins */
      /* except the first pattern CE).        */
      /*======================================*/

      for (joinIndex = 0; joinIndex < arraySize; joinIndex++)
        { ListBetaJoinActivity(theEnv,theInfo,joinIndex,arraySize,output,returnValue); }

      /*================================*/
      /* Free the array of alpha joins. */
      /*================================*/

      FreeJoinArray(theEnv,theInfo,arraySize);
     }
  }

/************************/
/* ActivityHeaderString */
/************************/
static const char *ActivityHeaderString(
  Environment *theEnv,
  struct joinInformation *infoArray,
  long joinIndex,
  long arraySize)
  {
   struct joinNode *theJoin;
   struct joinInformation *theInfo;
   long i;
   bool nestedCEs = false;
   const char *returnString = "";
   long lastIndex;
   char buffer[32];

   /*=============================================*/
   /* Determine which joins need to be traversed. */
   /*=============================================*/

   for (i = 0; i < arraySize; i++)
     { infoArray[i].marked = false; }

   theInfo = &infoArray[joinIndex];
   theJoin = theInfo->theJoin;
   lastIndex = joinIndex;

   while (theJoin != NULL)
     {
      for (i = lastIndex; i >= 0; i--)
        {
         if (infoArray[i].theJoin == theJoin)
           {
            if (infoArray[i].patternBegin != infoArray[i].patternEnd)
              { nestedCEs = true; }
            lastIndex = i - 1;
            break;
           }
        }
      theJoin = theJoin->lastLevel;
     }

   gensprintf(buffer,"%d",theInfo->whichCE);
   returnString = AppendStrings(theEnv,returnString,buffer);
   if (nestedCEs == false)
     { return returnString; }

   if (theInfo->patternBegin == theInfo->patternEnd)
     {
      returnString = AppendStrings(theEnv,returnString," (P");
      gensprintf(buffer,"%d",theInfo->patternBegin);
      returnString = AppendStrings(theEnv,returnString,buffer);

      returnString = AppendStrings(theEnv,returnString,")");
     }
   else
     {
      returnString = AppendStrings(theEnv,returnString," (P");

      gensprintf(buffer,"%d",theInfo->patternBegin);
      returnString = AppendStrings(theEnv,returnString,buffer);

      returnString = AppendStrings(theEnv,returnString," - P");

      gensprintf(buffer,"%d",theInfo->patternEnd);
      returnString = AppendStrings(theEnv,returnString,buffer);

      returnString = AppendStrings(theEnv,returnString,")");
     }

   return returnString;
  }

/*************************/
/* ListBetaJoinActivity: */
/*************************/
static void ListBetaJoinActivity(
  Environment *theEnv,
  struct joinInformation *infoArray,
  long joinIndex,
  long arraySize,
  int output,
  UDFValue *returnValue)
  {
   long long activity = 0;
   long long compares, adds, deletes;
   struct joinNode *theJoin, *nextJoin;
   struct joinInformation *theInfo;

   if (GetHaltExecution(theEnv) == true)
     { return; }

   theInfo = &infoArray[joinIndex];

   theJoin = theInfo->theJoin;
   nextJoin = theInfo->nextJoin;

   compares = theJoin->memoryCompares;
   if (theInfo->nextJoin->joinFromTheRight)
     {
      adds = nextJoin->memoryRightAdds;
      deletes = nextJoin->memoryRightDeletes;
     }
   else
     {
      adds = nextJoin->memoryLeftAdds;
      deletes = nextJoin->memoryLeftDeletes;
     }

   activity = compares + adds + deletes;

   if (output == VERBOSE)
     {
      char buffer[100];

      WriteString(theEnv,STDOUT,"Activity for CE ");
      WriteString(theEnv,STDOUT,
                     ActivityHeaderString(theEnv,infoArray,joinIndex,arraySize));
      WriteString(theEnv,STDOUT,"\n");

      sprintf(buffer,"   Compares: %10lld\n",compares);
      WriteString(theEnv,STDOUT,buffer);
      sprintf(buffer,"   Adds:     %10lld\n",adds);
      WriteString(theEnv,STDOUT,buffer);
      sprintf(buffer,"   Deletes:  %10lld\n",deletes);
      WriteString(theEnv,STDOUT,buffer);
     }
   else if (output == SUCCINCT)
     {
      WriteString(theEnv,STDOUT,"CE ");
      WriteString(theEnv,STDOUT,
                     ActivityHeaderString(theEnv,infoArray,joinIndex,arraySize));
      WriteString(theEnv,STDOUT,": ");
      WriteInteger(theEnv,STDOUT,activity);
      WriteString(theEnv,STDOUT,"\n");
     }

   compares += returnValue->multifieldValue->contents[0].integerValue->contents;
   adds += returnValue->multifieldValue->contents[1].integerValue->contents;
   deletes += returnValue->multifieldValue->contents[2].integerValue->contents;

   returnValue->multifieldValue->contents[0].integerValue = CreateInteger(theEnv,compares);
   returnValue->multifieldValue->contents[1].integerValue = CreateInteger(theEnv,adds);
   returnValue->multifieldValue->contents[2].integerValue = CreateInteger(theEnv,deletes);
  }

/*********************************************/
/* JoinActivityReset: Sets the join activity */
/*   counts for each rule back to 0.         */
/*********************************************/
static void JoinActivityReset(
  Environment *theEnv,
  ConstructHeader *theConstruct,
  void *buffer)
  {
#if MAC_XCD
#pragma unused(buffer)
#endif
   Defrule *theDefrule = (Defrule *) theConstruct;
   struct joinNode *theJoin = theDefrule->lastJoin;

   while (theJoin != NULL)
     {
      theJoin->memoryCompares = 0;
      theJoin->memoryLeftAdds = 0;
      theJoin->memoryRightAdds = 0;
      theJoin->memoryLeftDeletes = 0;
      theJoin->memoryRightDeletes = 0;

      if (theJoin->joinFromTheRight)
        { theJoin = (struct joinNode *) theJoin->rightSideEntryStructure; }
      else
        { theJoin = theJoin->lastLevel; }
     }
  }

/************************************************/
/* JoinActivityResetCommand: H/L access routine */
/*   for the reset-join-activity command.       */
/************************************************/
void JoinActivityResetCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   DoForAllConstructs(theEnv,
                      JoinActivityReset,
                      DefruleData(theEnv)->DefruleModuleIndex,true,NULL);
  }

/***************************************/
/* TimetagFunction: H/L access routine */
/*   for the timetag function.         */
/***************************************/
void TimetagFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;
   void *ptr;

   ptr = GetFactOrInstanceArgument(context,1,&theArg);

   if (ptr == NULL)
     {
      returnValue->integerValue = CreateInteger(theEnv,-1LL);
      return;
     }

   returnValue->integerValue = CreateInteger(theEnv,(long long) ((struct patternEntity *) ptr)->timeTag);
  }

#endif /* DEBUGGING_FUNCTIONS */

#if DEVELOPER
/***********************************************/
/* RuleComplexityCommand: H/L access routine   */
/*   for the rule-complexity function.         */
/***********************************************/
void RuleComplexityCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *ruleName;
   Defrule *rulePtr;

   ruleName = GetConstructName(context,"rule-complexity","rule name");
   if (ruleName == NULL)
     {
      returnValue->integerValue = CreateInteger(theEnv,-1);
      return;
     }

   rulePtr = FindDefrule(theEnv,ruleName);
   if (rulePtr == NULL)
     {
      CantFindItemErrorMessage(theEnv,"defrule",ruleName,true);
      returnValue->integerValue = CreateInteger(theEnv,-1);
      return;
     }

   returnValue->integerValue = CreateInteger(theEnv,rulePtr->complexity);
  }

/******************************************/
/* ShowJoinsCommand: H/L access routine   */
/*   for the show-joins command.          */
/******************************************/
void ShowJoinsCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *ruleName;
   Defrule *rulePtr;

   ruleName = GetConstructName(context,"show-joins","rule name");
   if (ruleName == NULL) return;

   rulePtr = FindDefrule(theEnv,ruleName);
   if (rulePtr == NULL)
     {
      CantFindItemErrorMessage(theEnv,"defrule",ruleName,true);
      return;
     }

   ShowJoins(theEnv,rulePtr);

   return;
  }

/*********************************/
/* ShowJoins: C access routine   */
/*   for the show-joins command. */
/*********************************/
static void ShowJoins(
  Environment *theEnv,
  Defrule *theRule)
  {
   Defrule *rulePtr;
   struct joinNode *theJoin;
   struct joinNode *joinList[MAXIMUM_NUMBER_OF_PATTERNS];
   int numberOfJoins;
   char rhsType;
   unsigned int disjunct = 0;
   unsigned long count = 0;

   rulePtr = theRule;

   if ((rulePtr != NULL) && (rulePtr->disjunct != NULL))
     { disjunct = 1; }

   /*=================================================*/
   /* Loop through each of the disjuncts for the rule */
   /*=================================================*/

   while (rulePtr != NULL)
     {
      if (disjunct > 0)
        {
         WriteString(theEnv,STDOUT,"Disjunct #");
         PrintUnsignedInteger(theEnv,STDOUT,disjunct++);
         WriteString(theEnv,STDOUT,"\n");
        }

      /*=====================================*/
      /* Determine the number of join nodes. */
      /*=====================================*/

      numberOfJoins = -1;
      theJoin = rulePtr->lastJoin;
      while (theJoin != NULL)
        {
         if (theJoin->joinFromTheRight)
           {
            numberOfJoins++;
            joinList[numberOfJoins] = theJoin;
            theJoin = (struct joinNode *) theJoin->rightSideEntryStructure;
           }
         else
           {
            numberOfJoins++;
            joinList[numberOfJoins] = theJoin;
            theJoin = theJoin->lastLevel;
           }
        }

      /*====================*/
      /* Display the joins. */
      /*====================*/

      while (numberOfJoins >= 0)
        {
         char buffer[20];

         if (joinList[numberOfJoins]->patternIsNegated)
           { rhsType = 'n'; }
         else if (joinList[numberOfJoins]->patternIsExists)
           { rhsType = 'x'; }
         else
           { rhsType = ' '; }

         gensprintf(buffer,"%2hu%c%c%c%c : ",joinList[numberOfJoins]->depth,
                                     (joinList[numberOfJoins]->firstJoin) ? 'f' : ' ',
                                     rhsType,
                                     (joinList[numberOfJoins]->joinFromTheRight) ? 'j' : ' ',
                                     (joinList[numberOfJoins]->logicalJoin) ? 'l' : ' ');
         WriteString(theEnv,STDOUT,buffer);
         PrintExpression(theEnv,STDOUT,joinList[numberOfJoins]->networkTest);
         WriteString(theEnv,STDOUT,"\n");

         if (joinList[numberOfJoins]->ruleToActivate != NULL)
           {
            WriteString(theEnv,STDOUT,"    RA : ");
            WriteString(theEnv,STDOUT,DefruleName(joinList[numberOfJoins]->ruleToActivate));
            WriteString(theEnv,STDOUT,"\n");
           }

         if (joinList[numberOfJoins]->secondaryNetworkTest != NULL)
           {
            WriteString(theEnv,STDOUT,"    SNT : ");
            PrintExpression(theEnv,STDOUT,joinList[numberOfJoins]->secondaryNetworkTest);
            WriteString(theEnv,STDOUT,"\n");
           }

         if (joinList[numberOfJoins]->leftHash != NULL)
           {
            WriteString(theEnv,STDOUT,"    LH : ");
            PrintExpression(theEnv,STDOUT,joinList[numberOfJoins]->leftHash);
            WriteString(theEnv,STDOUT,"\n");
           }

         if (joinList[numberOfJoins]->rightHash != NULL)
           {
            WriteString(theEnv,STDOUT,"    RH : ");
            PrintExpression(theEnv,STDOUT,joinList[numberOfJoins]->rightHash);
            WriteString(theEnv,STDOUT,"\n");
           }

         if (! joinList[numberOfJoins]->firstJoin)
           {
            WriteString(theEnv,STDOUT,"    LM : ");
            count = PrintBetaMemory(theEnv,STDOUT,joinList[numberOfJoins]->leftMemory,false,"",SUCCINCT);
            if (count == 0)
              { WriteString(theEnv,STDOUT,"None\n"); }
            else
              {
               sprintf(buffer,"%lu\n",count);
               WriteString(theEnv,STDOUT,buffer);
              }
           }

         if (joinList[numberOfJoins]->joinFromTheRight)
           {
            WriteString(theEnv,STDOUT,"    RM : ");
            count = PrintBetaMemory(theEnv,STDOUT,joinList[numberOfJoins]->rightMemory,false,"",SUCCINCT);
            if (count == 0)
              { WriteString(theEnv,STDOUT,"None\n"); }
            else
              {
               sprintf(buffer,"%lu\n",count);
               WriteString(theEnv,STDOUT,buffer);
              }
           }

         numberOfJoins--;
        };

      /*===============================*/
      /* Proceed to the next disjunct. */
      /*===============================*/

      rulePtr = rulePtr->disjunct;
      if (rulePtr != NULL) WriteString(theEnv,STDOUT,"\n");
     }
  }

/******************************************************/
/* ShowAlphaHashTable: Displays the number of entries */
/*   in each slot of the alpha hash table.            */
/******************************************************/
void ShowAlphaHashTable(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
   {
    int i, count;
    long totalCount = 0;
    struct alphaMemoryHash *theEntry;
    struct partialMatch *theMatch;
    char buffer[40];

    for (i = 0; i < ALPHA_MEMORY_HASH_SIZE; i++)
      {
       for (theEntry =  DefruleData(theEnv)->AlphaMemoryTable[i], count = 0;
            theEntry != NULL;
            theEntry = theEntry->next)
         { count++; }

       if (count != 0)
         {
          totalCount += count;
          gensprintf(buffer,"%4d: %4d ->",i,count);
          WriteString(theEnv,STDOUT,buffer);

          for (theEntry =  DefruleData(theEnv)->AlphaMemoryTable[i], count = 0;
               theEntry != NULL;
               theEntry = theEntry->next)
            {
             for (theMatch = theEntry->alphaMemory;
                  theMatch != NULL;
                  theMatch = theMatch->nextInMemory)
               { count++; }

             gensprintf(buffer," %4d",count);
             WriteString(theEnv,STDOUT,buffer);
             if (theEntry->owner->rightHash == NULL)
               { WriteString(theEnv,STDOUT,"*"); }
            }

          WriteString(theEnv,STDOUT,"\n");
         }
      }
    gensprintf(buffer,"Total Count: %ld\n",totalCount);
    WriteString(theEnv,STDOUT,buffer);
   }

#endif /* DEVELOPER */

#endif /* DEFRULE_CONSTRUCT */
