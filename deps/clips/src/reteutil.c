   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  08/28/17             */
   /*                                                     */
   /*                 RETE UTILITY MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides a set of utility functions useful to    */
/*   other modules.                                          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed INCREMENTAL_RESET compilation flag.    */
/*                                                           */
/*            Rule with exists CE has incorrect activation.  */
/*            DR0867                                         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Support for join network changes.              */
/*                                                           */
/*            Support for using an asterick (*) to indicate  */
/*            that existential patterns are matched.         */
/*                                                           */
/*            Support for partial match changes.             */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Added support for hashed memories.             */
/*                                                           */
/*            Removed pseudo-facts used in not CEs.          */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.31: Bug fix to prevent rule activations for        */
/*            partial matches being deleted.                 */
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
/*            Incremental reset is always enabled.           */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "incrrset.h"
#include "match.h"
#include "memalloc.h"
#include "moduldef.h"
#include "pattern.h"
#include "prntutil.h"
#include "retract.h"
#include "router.h"
#include "rulecom.h"

#include "reteutil.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                        TraceErrorToRuleDriver(Environment *,struct joinNode *,const char *,int,bool);
   static struct alphaMemoryHash     *FindAlphaMemory(Environment *,struct patternNodeHeader *,unsigned long);
   static unsigned long               AlphaMemoryHashValue(struct patternNodeHeader *,unsigned long);
   static void                        UnlinkAlphaMemory(Environment *,struct patternNodeHeader *,struct alphaMemoryHash *);
   static void                        UnlinkAlphaMemoryBucketSiblings(Environment *,struct alphaMemoryHash *);
   static void                        InitializePMLinks(struct partialMatch *);
   static void                        UnlinkBetaPartialMatchfromAlphaAndBetaLineage(struct partialMatch *);
   static int                         CountPriorPatterns(struct joinNode *);
   static void                        ResizeBetaMemory(Environment *,struct betaMemory *);
   static void                        ResetBetaMemory(Environment *,struct betaMemory *);
#if (CONSTRUCT_COMPILER || BLOAD_AND_BSAVE) && (! RUN_TIME)
   static void                        TagNetworkTraverseJoins(Environment *,unsigned long *,unsigned long *,struct joinNode *);
#endif

/***********************************************************/
/* PrintPartialMatch: Prints out the list of fact indices  */
/*   and/or instance names associated with a partial match */
/*   or rule instantiation.                                */
/***********************************************************/
void PrintPartialMatch(
  Environment *theEnv,
  const char *logicalName,
  struct partialMatch *list)
  {
   struct patternEntity *matchingItem;
   unsigned short i;

   for (i = 0; i < list->bcount;)
     {
      if ((get_nth_pm_match(list,i) != NULL) &&
          (get_nth_pm_match(list,i)->matchingItem != NULL))
        {
         matchingItem = get_nth_pm_match(list,i)->matchingItem;
         (*matchingItem->theInfo->base.shortPrintFunction)(theEnv,logicalName,matchingItem);
        }
      else
        { WriteString(theEnv,logicalName,"*"); }
      i++;
      if (i < list->bcount) WriteString(theEnv,logicalName,",");
     }
  }

/**********************************************/
/* CopyPartialMatch:  Copies a partial match. */
/**********************************************/
struct partialMatch *CopyPartialMatch(
  Environment *theEnv,
  struct partialMatch *list)
  {
   struct partialMatch *linker;
   unsigned short i;

   linker = get_var_struct(theEnv,partialMatch,sizeof(struct genericMatch) *
                                        (list->bcount - 1));

   InitializePMLinks(linker);
   linker->betaMemory = true;
   linker->busy = false;
   linker->rhsMemory = false;
   linker->deleting = false;
   linker->bcount = list->bcount;
   linker->hashValue = 0;

   for (i = 0; i < linker->bcount; i++) linker->binds[i] = list->binds[i];

   return(linker);
  }

/****************************/
/* CreateEmptyPartialMatch: */
/****************************/
struct partialMatch *CreateEmptyPartialMatch(
  Environment *theEnv)
  {
   struct partialMatch *linker;

   linker = get_struct(theEnv,partialMatch);

   InitializePMLinks(linker);
   linker->betaMemory = true;
   linker->busy = false;
   linker->rhsMemory = false;
   linker->deleting = false;
   linker->bcount = 1;
   linker->hashValue = 0;
   linker->binds[0].gm.theValue = NULL;

   return(linker);
  }

/**********************/
/* InitializePMLinks: */
/**********************/
static void InitializePMLinks(
  struct partialMatch *theMatch)
  {
   theMatch->nextInMemory = NULL;
   theMatch->prevInMemory = NULL;
   theMatch->nextRightChild = NULL;
   theMatch->prevRightChild = NULL;
   theMatch->nextLeftChild = NULL;
   theMatch->prevLeftChild = NULL;
   theMatch->children = NULL;
   theMatch->rightParent = NULL;
   theMatch->leftParent = NULL;
   theMatch->blockList = NULL;
   theMatch->nextBlocked = NULL;
   theMatch->prevBlocked = NULL;
   theMatch->marker = NULL;
   theMatch->dependents = NULL;
  }

/**********************/
/* UpdateBetaPMLinks: */
/**********************/
void UpdateBetaPMLinks(
  Environment *theEnv,
  struct partialMatch *thePM,
  struct partialMatch *lhsBinds,
  struct partialMatch *rhsBinds,
  struct joinNode *join,
  unsigned long hashValue,
  int side)
  {
   unsigned long betaLocation;
   struct betaMemory *theMemory;

   if (side == LHS)
     {
      theMemory = join->leftMemory;
      thePM->rhsMemory = false;
     }
   else
     {
      theMemory = join->rightMemory;
      thePM->rhsMemory = true;
     }

   thePM->hashValue = hashValue;

   /*================================*/
   /* Update the node's linked list. */
   /*================================*/

   betaLocation = hashValue % theMemory->size;

   if (side == LHS)
     {
      thePM->nextInMemory = theMemory->beta[betaLocation];
      if (theMemory->beta[betaLocation] != NULL)
        { theMemory->beta[betaLocation]->prevInMemory = thePM; }
      theMemory->beta[betaLocation] = thePM;
     }
   else
     {
      if (theMemory->last[betaLocation] != NULL)
        {
         theMemory->last[betaLocation]->nextInMemory = thePM;
         thePM->prevInMemory = theMemory->last[betaLocation];
        }
      else
        { theMemory->beta[betaLocation] = thePM; }

      theMemory->last[betaLocation] = thePM;
     }

   theMemory->count++;
   if (side == LHS)
    { join->memoryLeftAdds++; }
   else
    { join->memoryRightAdds++; }

   thePM->owner = join;

   /*======================================*/
   /* Update the alpha memory linked list. */
   /*======================================*/

   if (rhsBinds != NULL)
     {
      thePM->nextRightChild = rhsBinds->children;
      if (rhsBinds->children != NULL)
        { rhsBinds->children->prevRightChild = thePM; }
      rhsBinds->children = thePM;
      thePM->rightParent = rhsBinds;
    }

   /*=====================================*/
   /* Update the beta memory linked list. */
   /*=====================================*/

   if (lhsBinds != NULL)
     {
      thePM->nextLeftChild = lhsBinds->children;
      if (lhsBinds->children != NULL)
        { lhsBinds->children->prevLeftChild = thePM; }
      lhsBinds->children = thePM;
      thePM->leftParent = lhsBinds;
     }

   if (! DefruleData(theEnv)->BetaMemoryResizingFlag)
     { return; }

   if ((theMemory->size > 1) &&
       (theMemory->count > (theMemory->size * 11)))
     { ResizeBetaMemory(theEnv,theMemory); }
  }

/**********************************************************/
/* AddBlockedLink: Adds a link between a partial match in */
/*   the beta memory of a join (with a negated RHS) and a */
/*   partial match in its right memory that prevents the  */
/*   partial match from being satisfied and propagated to */
/*   the next join in the rule.                           */
/**********************************************************/
void AddBlockedLink(
  struct partialMatch *thePM,
  struct partialMatch *rhsBinds)
  {
   thePM->marker = rhsBinds;
   thePM->nextBlocked = rhsBinds->blockList;
   if (rhsBinds->blockList != NULL)
     { rhsBinds->blockList->prevBlocked = thePM; }
   rhsBinds->blockList = thePM;
  }

/*************************************************************/
/* RemoveBlockedLink: Removes a link between a partial match */
/*   in the beta memory of a join (with a negated RHS) and a */
/*   partial match in its right memory that prevents the     */
/*   partial match from being satisfied and propagated to    */
/*   the next join in the rule.                              */
/*************************************************************/
void RemoveBlockedLink(
  struct partialMatch *thePM)
  {
   struct partialMatch *blocker;

   if (thePM->prevBlocked == NULL)
     {
      blocker = (struct partialMatch *) thePM->marker;
      blocker->blockList = thePM->nextBlocked;
     }
   else
     { thePM->prevBlocked->nextBlocked = thePM->nextBlocked; }

   if (thePM->nextBlocked != NULL)
     { thePM->nextBlocked->prevBlocked = thePM->prevBlocked; }

   thePM->nextBlocked = NULL;
   thePM->prevBlocked = NULL;
   thePM->marker = NULL;
  }

/***********************************/
/* UnlinkBetaPMFromNodeAndLineage: */
/***********************************/
void UnlinkBetaPMFromNodeAndLineage(
  Environment *theEnv,
  struct joinNode *join,
  struct partialMatch *thePM,
  int side)
  {
   unsigned long betaLocation;
   struct betaMemory *theMemory;

   if (side == LHS)
     { theMemory = join->leftMemory; }
   else
     { theMemory = join->rightMemory; }

   /*=============================================*/
   /* Update the nextInMemory/prevInMemory links. */
   /*=============================================*/

   theMemory->count--;

   if (side == LHS)
    { join->memoryLeftDeletes++; }
   else
    { join->memoryRightDeletes++; }

   betaLocation = thePM->hashValue % theMemory->size;

   if ((side == RHS) &&
       (theMemory->last[betaLocation] == thePM))
     { theMemory->last[betaLocation] = thePM->prevInMemory; }

   if (thePM->prevInMemory == NULL)
     {
      betaLocation = thePM->hashValue % theMemory->size;
      theMemory->beta[betaLocation] = thePM->nextInMemory;
     }
   else
     { thePM->prevInMemory->nextInMemory = thePM->nextInMemory; }

   if (thePM->nextInMemory != NULL)
     { thePM->nextInMemory->prevInMemory = thePM->prevInMemory; }

   thePM->nextInMemory = NULL;
   thePM->prevInMemory = NULL;

   UnlinkBetaPartialMatchfromAlphaAndBetaLineage(thePM);

   if (! DefruleData(theEnv)->BetaMemoryResizingFlag)
     { return; }

   if ((theMemory->count == 0) && (theMemory->size > 1))
     { ResetBetaMemory(theEnv,theMemory); }
  }

/*************************/
/* UnlinkNonLeftLineage: */
/*************************/
void UnlinkNonLeftLineage(
  Environment *theEnv,
  struct joinNode *join,
  struct partialMatch *thePM,
  int side)
  {
   unsigned long betaLocation;
   struct betaMemory *theMemory;
   struct partialMatch *tempPM;

   if (side == LHS)
     { theMemory = join->leftMemory; }
   else
     { theMemory = join->rightMemory; }

   /*=============================================*/
   /* Update the nextInMemory/prevInMemory links. */
   /*=============================================*/

   theMemory->count--;

   if (side == LHS)
    { join->memoryLeftDeletes++; }
   else
    { join->memoryRightDeletes++; }

   betaLocation = thePM->hashValue % theMemory->size;

   if ((side == RHS) &&
       (theMemory->last[betaLocation] == thePM))
     { theMemory->last[betaLocation] = thePM->prevInMemory; }

   if (thePM->prevInMemory == NULL)
     {
      betaLocation = thePM->hashValue % theMemory->size;
      theMemory->beta[betaLocation] = thePM->nextInMemory;
     }
   else
     { thePM->prevInMemory->nextInMemory = thePM->nextInMemory; }

   if (thePM->nextInMemory != NULL)
     { thePM->nextInMemory->prevInMemory = thePM->prevInMemory; }

   /*=========================*/
   /* Update the alpha lists. */
   /*=========================*/

   if (thePM->prevRightChild == NULL)
     {
      if (thePM->rightParent != NULL)
        {
         thePM->rightParent->children = thePM->nextRightChild;
         if (thePM->nextRightChild != NULL)
           {
            thePM->rightParent->children = thePM->nextRightChild;
            thePM->nextRightChild->rightParent = thePM->rightParent;
           }
        }
     }
   else
     { thePM->prevRightChild->nextRightChild = thePM->nextRightChild; }

   if (thePM->nextRightChild != NULL)
     { thePM->nextRightChild->prevRightChild = thePM->prevRightChild; }

   /*===========================*/
   /* Update the blocked lists. */
   /*===========================*/

   if (thePM->prevBlocked == NULL)
     {
      tempPM = (struct partialMatch *) thePM->marker;

      if (tempPM != NULL)
        { tempPM->blockList = thePM->nextBlocked; }
     }
   else
     { thePM->prevBlocked->nextBlocked = thePM->nextBlocked; }

   if (thePM->nextBlocked != NULL)
     { thePM->nextBlocked->prevBlocked = thePM->prevBlocked; }

   if (! DefruleData(theEnv)->BetaMemoryResizingFlag)
     { return; }

   if ((theMemory->count == 0) && (theMemory->size > 1))
     { ResetBetaMemory(theEnv,theMemory); }
  }

/*******************************************************************/
/* UnlinkBetaPartialMatchfromAlphaAndBetaLineage: Removes the      */
/*   lineage links from a beta memory partial match. This removes  */
/*   the links between this partial match and its left and right   */
/*   memory parents. It also removes the links between this        */
/*   partial match and any of its children in other beta memories. */
/*******************************************************************/
static void UnlinkBetaPartialMatchfromAlphaAndBetaLineage(
  struct partialMatch *thePM)
  {
   struct partialMatch *tempPM;

   /*=========================*/
   /* Update the alpha lists. */
   /*=========================*/

   if (thePM->prevRightChild == NULL)
     {
      if (thePM->rightParent != NULL)
        { thePM->rightParent->children = thePM->nextRightChild; }
     }
   else
     { thePM->prevRightChild->nextRightChild = thePM->nextRightChild; }

   if (thePM->nextRightChild != NULL)
     { thePM->nextRightChild->prevRightChild = thePM->prevRightChild; }

   thePM->rightParent = NULL;
   thePM->nextRightChild = NULL;
   thePM->prevRightChild = NULL;

   /*========================*/
   /* Update the beta lists. */
   /*========================*/

   if (thePM->prevLeftChild == NULL)
     {
      if (thePM->leftParent != NULL)
        { thePM->leftParent->children = thePM->nextLeftChild; }
     }
   else
     { thePM->prevLeftChild->nextLeftChild = thePM->nextLeftChild; }

   if (thePM->nextLeftChild != NULL)
     { thePM->nextLeftChild->prevLeftChild = thePM->prevLeftChild; }

   thePM->leftParent = NULL;
   thePM->nextLeftChild = NULL;
   thePM->prevLeftChild = NULL;

   /*===========================*/
   /* Update the blocked lists. */
   /*===========================*/

   if (thePM->prevBlocked == NULL)
     {
      tempPM = (struct partialMatch *) thePM->marker;

      if (tempPM != NULL)
        { tempPM->blockList = thePM->nextBlocked; }
     }
   else
     { thePM->prevBlocked->nextBlocked = thePM->nextBlocked; }

   if (thePM->nextBlocked != NULL)
     { thePM->nextBlocked->prevBlocked = thePM->prevBlocked; }

   thePM->marker = NULL;
   thePM->nextBlocked = NULL;
   thePM->prevBlocked = NULL;

   /*===============================================*/
   /* Remove parent reference from the child links. */
   /*===============================================*/

   if (thePM->children != NULL)
     {
      if (thePM->rhsMemory)
        {
         for (tempPM = thePM->children; tempPM != NULL; tempPM = tempPM->nextRightChild)
           { tempPM->rightParent = NULL; }
        }
      else
        {
         for (tempPM = thePM->children; tempPM != NULL; tempPM = tempPM->nextLeftChild)
           { tempPM->leftParent = NULL; }
        }

      thePM->children = NULL;
     }
  }

/********************************************************/
/* MergePartialMatches: Merges two partial matches. The */
/*   second match should either be NULL (indicating a   */
/*   negated CE) or contain a single match.             */
/********************************************************/
struct partialMatch *MergePartialMatches(
  Environment *theEnv,
  struct partialMatch *lhsBind,
  struct partialMatch *rhsBind)
  {
   struct partialMatch *linker;
   static struct partialMatch mergeTemplate = { 1 }; /* betaMemory is true, remainder are 0 or NULL */

   /*=================================*/
   /* Allocate the new partial match. */
   /*=================================*/

   linker = get_var_struct(theEnv,partialMatch,sizeof(struct genericMatch) * lhsBind->bcount);

   /*============================================*/
   /* Set the flags to their appropriate values. */
   /*============================================*/

   memcpy(linker,&mergeTemplate,sizeof(struct partialMatch) - sizeof(struct genericMatch));

   linker->deleting = false;
   linker->bcount = lhsBind->bcount + 1;

   /*========================================================*/
   /* Copy the bindings of the partial match being extended. */
   /*========================================================*/

   memcpy(linker->binds,lhsBind->binds,sizeof(struct genericMatch) * lhsBind->bcount);

   /*===================================*/
   /* Add the binding of the rhs match. */
   /*===================================*/

   if (rhsBind == NULL)
     { linker->binds[lhsBind->bcount].gm.theValue = NULL; }
   else
     { linker->binds[lhsBind->bcount].gm.theValue = rhsBind->binds[0].gm.theValue; }

   return linker;
  }

/*******************************************************************/
/* InitializePatternHeader: Initializes a pattern header structure */
/*   (used by the fact and instance pattern matchers).             */
/*******************************************************************/
void InitializePatternHeader(
  Environment *theEnv,
  struct patternNodeHeader *theHeader)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
   theHeader->firstHash = NULL;
   theHeader->lastHash = NULL;
   theHeader->entryJoin = NULL;
   theHeader->rightHash = NULL;
   theHeader->singlefieldNode = false;
   theHeader->multifieldNode = false;
   theHeader->stopNode = false;
#if (! RUN_TIME)
   theHeader->initialize = true;
#else
   theHeader->initialize = false;
#endif
   theHeader->marked = false;
   theHeader->beginSlot = false;
   theHeader->endSlot = false;
   theHeader->selector = false;
  }

/******************************************************************/
/* CreateAlphaMatch: Given a pointer to an entity (such as a fact */
/*   or instance) which matched a pattern, this function creates  */
/*   a partial match suitable for storing in the alpha memory of  */
/*   the pattern network. Note that the multifield markers which  */
/*   are passed as a calling argument are copied (thus the caller */
/*   is still responsible for freeing these data structures).     */
/******************************************************************/
struct partialMatch *CreateAlphaMatch(
  Environment *theEnv,
  void *theEntity,
  struct multifieldMarker *markers,
  struct patternNodeHeader *theHeader,
  unsigned long hashOffset)
  {
   struct partialMatch *theMatch;
   struct alphaMatch *afbtemp;
   unsigned long hashValue;
   struct alphaMemoryHash *theAlphaMemory;

   /*==================================================*/
   /* Create the alpha match and intialize its values. */
   /*==================================================*/

   theMatch = get_struct(theEnv,partialMatch);
   InitializePMLinks(theMatch);
   theMatch->betaMemory = false;
   theMatch->busy = false;
   theMatch->deleting = false;
   theMatch->bcount = 1;
   theMatch->hashValue = hashOffset;

   afbtemp = get_struct(theEnv,alphaMatch);
   afbtemp->next = NULL;
   afbtemp->matchingItem = (struct patternEntity *) theEntity;

   if (markers != NULL)
     { afbtemp->markers = CopyMultifieldMarkers(theEnv,markers); }
   else
     { afbtemp->markers = NULL; }

   theMatch->binds[0].gm.theMatch = afbtemp;

   /*============================================*/
   /* Find the alpha memory of the pattern node. */
   /*============================================*/

   hashValue = AlphaMemoryHashValue(theHeader,hashOffset);
   theAlphaMemory = FindAlphaMemory(theEnv,theHeader,hashValue);
   afbtemp->bucket = hashValue;

   /*============================================*/
   /* Create an alpha memory if it wasn't found. */
   /*============================================*/

   if (theAlphaMemory == NULL)
     {
      theAlphaMemory = get_struct(theEnv,alphaMemoryHash);
      theAlphaMemory->bucket = hashValue;
      theAlphaMemory->owner = theHeader;
      theAlphaMemory->alphaMemory = NULL;
      theAlphaMemory->endOfQueue = NULL;
      theAlphaMemory->nextHash = NULL;

      theAlphaMemory->next = DefruleData(theEnv)->AlphaMemoryTable[hashValue];
      if (theAlphaMemory->next != NULL)
        { theAlphaMemory->next->prev = theAlphaMemory; }

      theAlphaMemory->prev = NULL;
      DefruleData(theEnv)->AlphaMemoryTable[hashValue] = theAlphaMemory;

      if (theHeader->firstHash == NULL)
        {
         theHeader->firstHash = theAlphaMemory;
         theHeader->lastHash = theAlphaMemory;
         theAlphaMemory->prevHash = NULL;
        }
      else
        {
         theHeader->lastHash->nextHash = theAlphaMemory;
         theAlphaMemory->prevHash = theHeader->lastHash;
         theHeader->lastHash = theAlphaMemory;
        }
     }

   /*====================================*/
   /* Store the alpha match in the alpha */
   /* memory of the pattern node.        */
   /*====================================*/

    theMatch->prevInMemory = theAlphaMemory->endOfQueue;
    if (theAlphaMemory->endOfQueue == NULL)
     {
      theAlphaMemory->alphaMemory = theMatch;
      theAlphaMemory->endOfQueue = theMatch;
     }
   else
     {
      theAlphaMemory->endOfQueue->nextInMemory = theMatch;
      theAlphaMemory->endOfQueue = theMatch;
     }

   /*===================================================*/
   /* Return a pointer to the newly create alpha match. */
   /*===================================================*/

   return(theMatch);
  }

/*******************************************/
/* CopyMultifieldMarkers: Copies a list of */
/*   multifieldMarker data structures.     */
/*******************************************/
struct multifieldMarker *CopyMultifieldMarkers(
  Environment *theEnv,
  struct multifieldMarker *theMarkers)
  {
   struct multifieldMarker *head = NULL, *lastMark = NULL, *newMark;

   while (theMarkers != NULL)
     {
      newMark = get_struct(theEnv,multifieldMarker);
      newMark->next = NULL;
      newMark->whichField = theMarkers->whichField;
      newMark->where = theMarkers->where;
      newMark->startPosition = theMarkers->startPosition;
      newMark->range = theMarkers->range;

      if (lastMark == NULL)
        { head = newMark; }
      else
        { lastMark->next = newMark; }
      lastMark = newMark;

      theMarkers = theMarkers->next;
     }

   return(head);
  }

/***************************************************************/
/* FlushAlphaBetaMemory: Returns all partial matches in a list */
/*   of partial matches either directly to the pool of free    */
/*   memory or to the list of GarbagePartialMatches. Partial   */
/*   matches stored in alpha memories must be placed on the    */
/*   list of GarbagePartialMatches.                            */
/***************************************************************/
void FlushAlphaBetaMemory(
  Environment *theEnv,
  struct partialMatch *pfl)
  {
   struct partialMatch *pfltemp;

   while (pfl != NULL)
     {
      pfltemp = pfl->nextInMemory;

      UnlinkBetaPartialMatchfromAlphaAndBetaLineage(pfl);
      ReturnPartialMatch(theEnv,pfl);

      pfl = pfltemp;
     }
  }

/*****************************************************************/
/* DestroyAlphaBetaMemory: Returns all partial matches in a list */
/*   of partial matches directly to the pool of free memory.     */
/*****************************************************************/
void DestroyAlphaBetaMemory(
  Environment *theEnv,
  struct partialMatch *pfl)
  {
   struct partialMatch *pfltemp;

   while (pfl != NULL)
     {
      pfltemp = pfl->nextInMemory;
      DestroyPartialMatch(theEnv,pfl);
      pfl = pfltemp;
     }
  }

/******************************************************/
/* FindEntityInPartialMatch: Searches for a specified */
/*   data entity in a partial match.                  */
/******************************************************/
bool FindEntityInPartialMatch(
  struct patternEntity *theEntity,
  struct partialMatch *thePartialMatch)
  {
   unsigned short i;

   for (i = 0 ; i < thePartialMatch->bcount; i++)
     {
      if (thePartialMatch->binds[i].gm.theMatch == NULL) continue;
      if (thePartialMatch->binds[i].gm.theMatch->matchingItem == theEntity)
        { return true; }
     }

   return false;
  }

/***********************************************************************/
/* GetPatternNumberFromJoin: Given a pointer to a join associated with */
/*   a pattern CE, returns an integer representing the position of the */
/*   pattern CE in the rule (e.g. first, second, third).               */
/***********************************************************************/
int GetPatternNumberFromJoin(
  struct joinNode *joinPtr)
  {
   int whichOne = 0;

   while (joinPtr != NULL)
     {
      if (joinPtr->joinFromTheRight)
        { joinPtr = (struct joinNode *) joinPtr->rightSideEntryStructure; }
      else
        {
         whichOne++;
         joinPtr = joinPtr->lastLevel;
        }
     }

   return(whichOne);
  }

/************************************************************************/
/* TraceErrorToRule: Prints an error message when a error occurs as the */
/*   result of evaluating an expression in the pattern network. Used to */
/*   indicate which rule caused the problem.                            */
/************************************************************************/
void TraceErrorToRule(
  Environment *theEnv,
  struct joinNode *joinPtr,
  const char *indentSpaces)
  {
   int patternCount;

   MarkRuleNetwork(theEnv,0);

   patternCount = CountPriorPatterns(joinPtr->lastLevel) + 1;

   TraceErrorToRuleDriver(theEnv,joinPtr,indentSpaces,patternCount,false);

   MarkRuleNetwork(theEnv,0);
  }

/**************************************************************/
/* TraceErrorToRuleDriver: Driver code for printing out which */
/*   rule caused a pattern or join network error.             */
/**************************************************************/
static void TraceErrorToRuleDriver(
  Environment *theEnv,
  struct joinNode *joinPtr,
  const char *indentSpaces,
  int priorRightJoinPatterns,
  bool enteredJoinFromRight)
  {
   const char *name;
   int priorPatternCount;
   struct joinLink *theLinks;

   if ((joinPtr->joinFromTheRight) && enteredJoinFromRight)
     { priorPatternCount = CountPriorPatterns(joinPtr->lastLevel); }
   else
     { priorPatternCount = 0; }

   if (joinPtr->marked)
     { /* Do Nothing */ }
   else if (joinPtr->ruleToActivate != NULL)
     {
      joinPtr->marked = 1;
      name = DefruleName(joinPtr->ruleToActivate);
      WriteString(theEnv,STDERR,indentSpaces);

      WriteString(theEnv,STDERR,"Of pattern #");
      WriteInteger(theEnv,STDERR,priorRightJoinPatterns+priorPatternCount);
      WriteString(theEnv,STDERR," in rule ");
      WriteString(theEnv,STDERR,name);
      WriteString(theEnv,STDERR,"\n");
     }
   else
     {
      joinPtr->marked = 1;

      theLinks = joinPtr->nextLinks;
      while (theLinks != NULL)
        {
         TraceErrorToRuleDriver(theEnv,theLinks->join,indentSpaces,
                                priorRightJoinPatterns+priorPatternCount,
                                (theLinks->enterDirection == RHS));
         theLinks = theLinks->next;
        }
     }
  }

/***********************/
/* CountPriorPatterns: */
/***********************/
static int CountPriorPatterns(
  struct joinNode *joinPtr)
  {
   int count = 0;

   while (joinPtr != NULL)
     {
      if (joinPtr->joinFromTheRight)
        { count += CountPriorPatterns((struct joinNode *) joinPtr->rightSideEntryStructure); }
      else
        { count++; }

      joinPtr = joinPtr->lastLevel;
     }

   return(count);
  }

/********************************************************/
/* MarkRuleNetwork: Sets the marked flag in each of the */
/*   joins in the join network to the specified value.  */
/********************************************************/
void MarkRuleNetwork(
  Environment *theEnv,
  bool value)
  {
   Defrule *rulePtr, *disjunctPtr;
   struct joinNode *joinPtr;
   Defmodule *modulePtr;

   /*===========================*/
   /* Loop through each module. */
   /*===========================*/

   SaveCurrentModule(theEnv);
   for (modulePtr = GetNextDefmodule(theEnv,NULL);
        modulePtr != NULL;
        modulePtr = GetNextDefmodule(theEnv,modulePtr))
     {
      SetCurrentModule(theEnv,modulePtr);

      /*=========================*/
      /* Loop through each rule. */
      /*=========================*/

      rulePtr = GetNextDefrule(theEnv,NULL);
      while (rulePtr != NULL)
        {
         /*=============================*/
         /* Mark each join for the rule */
         /* with the specified value.   */
         /*=============================*/

         for (disjunctPtr = rulePtr; disjunctPtr != NULL; disjunctPtr = disjunctPtr->disjunct)
           {
            joinPtr = disjunctPtr->lastJoin;
            MarkRuleJoins(joinPtr,value);
           }

         /*===========================*/
         /* Move on to the next rule. */
         /*===========================*/

         rulePtr = GetNextDefrule(theEnv,rulePtr);
        }

     }

   RestoreCurrentModule(theEnv);
  }

/******************/
/* MarkRuleJoins: */
/******************/
void MarkRuleJoins(
  struct joinNode *joinPtr,
  bool value)
  {
   while (joinPtr != NULL)
     {
      if (joinPtr->joinFromTheRight)
        { MarkRuleJoins((struct joinNode *) joinPtr->rightSideEntryStructure,value); }

      joinPtr->marked = value;
      joinPtr = joinPtr->lastLevel;
     }
  }

/*****************************************/
/* GetAlphaMemory: Retrieves the list of */
/*   matches from an alpha memory.       */
/*****************************************/
struct partialMatch *GetAlphaMemory(
  Environment *theEnv,
  struct patternNodeHeader *theHeader,
  unsigned long hashOffset)
  {
   struct alphaMemoryHash *theAlphaMemory;
   unsigned long hashValue;

   hashValue = AlphaMemoryHashValue(theHeader,hashOffset);
   theAlphaMemory = FindAlphaMemory(theEnv,theHeader,hashValue);

   if (theAlphaMemory == NULL)
     { return NULL; }

   return theAlphaMemory->alphaMemory;
  }

/*****************************************/
/* GetLeftBetaMemory: Retrieves the list */
/*   of matches from a beta memory.      */
/*****************************************/
struct partialMatch *GetLeftBetaMemory(
  struct joinNode *theJoin,
  unsigned long hashValue)
  {
   unsigned long betaLocation;

   betaLocation = hashValue % theJoin->leftMemory->size;

   return theJoin->leftMemory->beta[betaLocation];
  }

/******************************************/
/* GetRightBetaMemory: Retrieves the list */
/*   of matches from a beta memory.       */
/******************************************/
struct partialMatch *GetRightBetaMemory(
  struct joinNode *theJoin,
  unsigned long hashValue)
  {
   unsigned long betaLocation;

   betaLocation = hashValue % theJoin->rightMemory->size;

   return theJoin->rightMemory->beta[betaLocation];
  }

/***************************************/
/* ReturnLeftMemory: Sets the contents */
/*   of a beta memory to NULL.         */
/***************************************/
void ReturnLeftMemory(
  Environment *theEnv,
  struct joinNode *theJoin)
  {
   if (theJoin->leftMemory == NULL) return;
   genfree(theEnv,theJoin->leftMemory->beta,sizeof(struct partialMatch *) * theJoin->leftMemory->size);
   rtn_struct(theEnv,betaMemory,theJoin->leftMemory);
   theJoin->leftMemory = NULL;
  }

/***************************************/
/* ReturnRightMemory: Sets the contents */
/*   of a beta memory to NULL.         */
/***************************************/
void ReturnRightMemory(
  Environment *theEnv,
  struct joinNode *theJoin)
  {
   if (theJoin->rightMemory == NULL) return;
   genfree(theEnv,theJoin->rightMemory->beta,sizeof(struct partialMatch *) * theJoin->rightMemory->size);
   genfree(theEnv,theJoin->rightMemory->last,sizeof(struct partialMatch *) * theJoin->rightMemory->size);
   rtn_struct(theEnv,betaMemory,theJoin->rightMemory);
   theJoin->rightMemory = NULL;
  }

/****************************************************************/
/* DestroyBetaMemory: Destroys the contents of a beta memory in */
/*   preperation for the deallocation of a join. Destroying is  */
/*   performed when the environment is being deallocated and it */
/*   is not necessary to leave the environment in a consistent  */
/*   state (as it would be if just a single rule were being     */
/*   deleted).                                                  */
/****************************************************************/
void DestroyBetaMemory(
  Environment *theEnv,
  struct joinNode *theJoin,
  int side)
  {
   unsigned long i;

   if (side == LHS)
     {
      if (theJoin->leftMemory == NULL) return;

      for (i = 0; i < theJoin->leftMemory->size; i++)
        { DestroyAlphaBetaMemory(theEnv,theJoin->leftMemory->beta[i]); }
     }
   else
     {
      if (theJoin->rightMemory == NULL) return;

      for (i = 0; i < theJoin->rightMemory->size; i++)
        { DestroyAlphaBetaMemory(theEnv,theJoin->rightMemory->beta[i]); }
     }
  }

/*************************************************************/
/* FlushBetaMemory: Flushes the contents of a beta memory in */
/*   preperation for the deallocation of a join. Flushing    */
/*   is performed when the partial matches in the beta       */
/*   memory may still be in use because the environment will */
/*   remain active.                                          */
/*************************************************************/
void FlushBetaMemory(
  Environment *theEnv,
  struct joinNode *theJoin,
  int side)
  {
   unsigned long i;

   if (side == LHS)
     {
      if (theJoin->leftMemory == NULL) return;

      for (i = 0; i < theJoin->leftMemory->size; i++)
        { FlushAlphaBetaMemory(theEnv,theJoin->leftMemory->beta[i]); }
     }
   else
     {
      if (theJoin->rightMemory == NULL) return;

      for (i = 0; i < theJoin->rightMemory->size; i++)
        { FlushAlphaBetaMemory(theEnv,theJoin->rightMemory->beta[i]); }
     }
 }

/***********************/
/* BetaMemoryNotEmpty: */
/***********************/
bool BetaMemoryNotEmpty(
  struct joinNode *theJoin)
  {
   if (theJoin->leftMemory != NULL)
     {
      if (theJoin->leftMemory->count > 0)
        { return true; }
     }

   if (theJoin->rightMemory != NULL)
     {
      if (theJoin->rightMemory->count > 0)
        { return true; }
     }

   return false;
  }

/*********************************************/
/* RemoveAlphaMemoryMatches: Removes matches */
/*   from an alpha memory.                   */
/*********************************************/
void RemoveAlphaMemoryMatches(
  Environment *theEnv,
  struct patternNodeHeader *theHeader,
  struct partialMatch *theMatch,
  struct alphaMatch *theAlphaMatch)
  {
   struct alphaMemoryHash *theAlphaMemory = NULL;
   unsigned long hashValue;

   if ((theMatch->prevInMemory == NULL) || (theMatch->nextInMemory == NULL))
     {
      hashValue = theAlphaMatch->bucket;
      theAlphaMemory = FindAlphaMemory(theEnv,theHeader,hashValue);
     }

   if (theMatch->prevInMemory != NULL)
     { theMatch->prevInMemory->nextInMemory = theMatch->nextInMemory; }
   else
     { theAlphaMemory->alphaMemory = theMatch->nextInMemory; }

   if (theMatch->nextInMemory != NULL)
     { theMatch->nextInMemory->prevInMemory = theMatch->prevInMemory; }
   else
     { theAlphaMemory->endOfQueue = theMatch->prevInMemory; }

   /*====================================*/
   /* Add the match to the garbage list. */
   /*====================================*/

   theMatch->nextInMemory = EngineData(theEnv)->GarbagePartialMatches;
   EngineData(theEnv)->GarbagePartialMatches = theMatch;

   if ((theAlphaMemory != NULL) && (theAlphaMemory->alphaMemory == NULL))
     { UnlinkAlphaMemory(theEnv,theHeader,theAlphaMemory); }
  }

/***********************/
/* DestroyAlphaMemory: */
/***********************/
void DestroyAlphaMemory(
  Environment *theEnv,
  struct patternNodeHeader *theHeader,
  bool unlink)
  {
   struct alphaMemoryHash *theAlphaMemory, *tempMemory;

   theAlphaMemory = theHeader->firstHash;

   while (theAlphaMemory != NULL)
     {
      tempMemory = theAlphaMemory->nextHash;
      DestroyAlphaBetaMemory(theEnv,theAlphaMemory->alphaMemory);
      if (unlink)
        { UnlinkAlphaMemoryBucketSiblings(theEnv,theAlphaMemory); }
      rtn_struct(theEnv,alphaMemoryHash,theAlphaMemory);
      theAlphaMemory = tempMemory;
     }

   theHeader->firstHash = NULL;
   theHeader->lastHash = NULL;
  }

/*********************/
/* FlushAlphaMemory: */
/*********************/
void FlushAlphaMemory(
  Environment *theEnv,
  struct patternNodeHeader *theHeader)
  {
   struct alphaMemoryHash *theAlphaMemory, *tempMemory;

   theAlphaMemory = theHeader->firstHash;

   while (theAlphaMemory != NULL)
     {
      tempMemory = theAlphaMemory->nextHash;
      FlushAlphaBetaMemory(theEnv,theAlphaMemory->alphaMemory);
      UnlinkAlphaMemoryBucketSiblings(theEnv,theAlphaMemory);
      rtn_struct(theEnv,alphaMemoryHash,theAlphaMemory);
      theAlphaMemory = tempMemory;
     }

   theHeader->firstHash = NULL;
   theHeader->lastHash = NULL;
  }

/********************/
/* FindAlphaMemory: */
/********************/
static struct alphaMemoryHash *FindAlphaMemory(
  Environment *theEnv,
  struct patternNodeHeader *theHeader,
  unsigned long hashValue)
  {
   struct alphaMemoryHash *theAlphaMemory;

   theAlphaMemory = DefruleData(theEnv)->AlphaMemoryTable[hashValue];

   if (theAlphaMemory != NULL)
     {
      while ((theAlphaMemory != NULL) && (theAlphaMemory->owner != theHeader))
        { theAlphaMemory = theAlphaMemory->next; }
     }

   return theAlphaMemory;
  }

/*************************/
/* AlphaMemoryHashValue: */
/*************************/
static unsigned long AlphaMemoryHashValue(
  struct patternNodeHeader *theHeader,
  unsigned long hashOffset)
  {
   unsigned long hashValue;
   union
     {
      void *vv;
      unsigned uv;
     } fis;

   fis.uv = 0;
   fis.vv = theHeader;

   hashValue = fis.uv + hashOffset;
   hashValue = hashValue % ALPHA_MEMORY_HASH_SIZE;

   return hashValue;
  }

/**********************/
/* UnlinkAlphaMemory: */
/**********************/
static void UnlinkAlphaMemory(
  Environment *theEnv,
  struct patternNodeHeader *theHeader,
  struct alphaMemoryHash *theAlphaMemory)
  {
   /*======================*/
   /* Unlink the siblings. */
   /*======================*/

   UnlinkAlphaMemoryBucketSiblings(theEnv,theAlphaMemory);

   /*================================*/
   /* Update firstHash and lastHash. */
   /*================================*/

   if (theAlphaMemory == theHeader->firstHash)
     { theHeader->firstHash = theAlphaMemory->nextHash; }

   if (theAlphaMemory == theHeader->lastHash)
     { theHeader->lastHash = theAlphaMemory->prevHash; }

   /*===============================*/
   /* Update nextHash and prevHash. */
   /*===============================*/

   if (theAlphaMemory->prevHash != NULL)
     { theAlphaMemory->prevHash->nextHash = theAlphaMemory->nextHash; }

   if (theAlphaMemory->nextHash != NULL)
     { theAlphaMemory->nextHash->prevHash = theAlphaMemory->prevHash; }

   rtn_struct(theEnv,alphaMemoryHash,theAlphaMemory);
  }

/************************************/
/* UnlinkAlphaMemoryBucketSiblings: */
/************************************/
static void UnlinkAlphaMemoryBucketSiblings(
  Environment *theEnv,
  struct alphaMemoryHash *theAlphaMemory)
  {
   if (theAlphaMemory->prev == NULL)
     { DefruleData(theEnv)->AlphaMemoryTable[theAlphaMemory->bucket] = theAlphaMemory->next; }
   else
     { theAlphaMemory->prev->next = theAlphaMemory->next; }

   if (theAlphaMemory->next != NULL)
     { theAlphaMemory->next->prev = theAlphaMemory->prev; }
  }

/**************************/
/* ComputeRightHashValue: */
/**************************/
unsigned long ComputeRightHashValue(
  Environment *theEnv,
  struct patternNodeHeader *theHeader)
  {
   struct expr *tempExpr;
   unsigned long hashValue = 0;
   unsigned long multiplier = 1;
   union
     {
      void *vv;
      unsigned long liv;
     } fis;

   if (theHeader->rightHash == NULL)
     { return hashValue; }

   for (tempExpr = theHeader->rightHash;
        tempExpr != NULL;
        tempExpr = tempExpr->nextArg, multiplier = multiplier * 509)
      {
       UDFValue theResult;
       struct expr *oldArgument;

       oldArgument = EvaluationData(theEnv)->CurrentExpression;
       EvaluationData(theEnv)->CurrentExpression = tempExpr;
       (*EvaluationData(theEnv)->PrimitivesArray[tempExpr->type]->evaluateFunction)(theEnv,tempExpr->value,&theResult);
       EvaluationData(theEnv)->CurrentExpression = oldArgument;

       switch (theResult.header->type)
         {
          case STRING_TYPE:
          case SYMBOL_TYPE:
          case INSTANCE_NAME_TYPE:
            hashValue += (theResult.lexemeValue->bucket * multiplier);
            break;

          case INTEGER_TYPE:
            hashValue += (theResult.integerValue->bucket * multiplier);
            break;

          case FLOAT_TYPE:
            hashValue += (theResult.floatValue->bucket * multiplier);
            break;

          case FACT_ADDRESS_TYPE:
#if OBJECT_SYSTEM
          case INSTANCE_ADDRESS_TYPE:
#endif
            fis.liv = 0;
            fis.vv = theResult.value;
            hashValue += fis.liv * multiplier;
            break;

          case EXTERNAL_ADDRESS_TYPE:
            fis.liv = 0;
            fis.vv = theResult.externalAddressValue->contents;
            hashValue += fis.liv * multiplier;
            break;
          }
       }

     return hashValue;
    }

/*********************/
/* ResizeBetaMemory: */
/*********************/
void ResizeBetaMemory(
  Environment *theEnv,
  struct betaMemory *theMemory)
  {
   struct partialMatch **oldArray, **lastAdd, *thePM, *nextPM;
   unsigned long i, oldSize, betaLocation;

   oldSize = theMemory->size;
   oldArray = theMemory->beta;

   theMemory->size = oldSize * 11;
   theMemory->beta = (struct partialMatch **) genalloc(theEnv,sizeof(struct partialMatch *) * theMemory->size);

   lastAdd = (struct partialMatch **) genalloc(theEnv,sizeof(struct partialMatch *) * theMemory->size);
   memset(theMemory->beta,0,sizeof(struct partialMatch *) * theMemory->size);
   memset(lastAdd,0,sizeof(struct partialMatch *) * theMemory->size);

   for (i = 0; i < oldSize; i++)
     {
      thePM = oldArray[i];
      while (thePM != NULL)
        {
         nextPM = thePM->nextInMemory;

         thePM->nextInMemory = NULL;

         betaLocation = thePM->hashValue % theMemory->size;
         thePM->prevInMemory = lastAdd[betaLocation];

         if (lastAdd[betaLocation] != NULL)
           { lastAdd[betaLocation]->nextInMemory = thePM; }
         else
           { theMemory->beta[betaLocation] = thePM; }

         lastAdd[betaLocation] = thePM;

         thePM = nextPM;
        }
     }

   if (theMemory->last != NULL)
     {
      genfree(theEnv,theMemory->last,sizeof(struct partialMatch *) * oldSize);
      theMemory->last = lastAdd;
     }
   else
     { genfree(theEnv,lastAdd,sizeof(struct partialMatch *) * theMemory->size); }

   genfree(theEnv,oldArray,sizeof(struct partialMatch *) * oldSize);
  }

/********************/
/* ResetBetaMemory: */
/********************/
static void ResetBetaMemory(
  Environment *theEnv,
  struct betaMemory *theMemory)
  {
   struct partialMatch **oldArray, **lastAdd;
   unsigned long oldSize;

   if ((theMemory->size == 1) ||
       (theMemory->size == INITIAL_BETA_HASH_SIZE))
     { return; }

   oldSize = theMemory->size;
   oldArray = theMemory->beta;

   theMemory->size = INITIAL_BETA_HASH_SIZE;
   theMemory->beta = (struct partialMatch **) genalloc(theEnv,sizeof(struct partialMatch *) * theMemory->size);
   memset(theMemory->beta,0,sizeof(struct partialMatch *) * theMemory->size);
   genfree(theEnv,oldArray,sizeof(struct partialMatch *) * oldSize);

   if (theMemory->last != NULL)
     {
      lastAdd = (struct partialMatch **) genalloc(theEnv,sizeof(struct partialMatch *) * theMemory->size);
      memset(lastAdd,0,sizeof(struct partialMatch *) * theMemory->size);
      genfree(theEnv,theMemory->last,sizeof(struct partialMatch *) * oldSize);
      theMemory->last = lastAdd;
     }
  }

/********************/
/* PrintBetaMemory: */
/********************/
unsigned long PrintBetaMemory(
  Environment *theEnv,
  const char *logName,
  struct betaMemory *theMemory,
  bool indentFirst,
  const char *indentString,
  int output)
  {
   struct partialMatch *listOfMatches;
   unsigned long b, count = 0;

   if (GetHaltExecution(theEnv) == true)
     { return count; }

   for (b = 0; b < theMemory->size; b++)
     {
      listOfMatches = theMemory->beta[b];

      while (listOfMatches != NULL)
        {
         /*=========================================*/
         /* Check to see if the user is attempting  */
         /* to stop the display of partial matches. */
         /*=========================================*/

         if (GetHaltExecution(theEnv) == true)
           { return count; }

         /*=========================================================*/
         /* The first partial match may have already been indented. */
         /* Subsequent partial matches will always be indented with */
         /* the indentation string.                                 */
         /*=========================================================*/

         if (output == VERBOSE)
           {
            if (indentFirst)
              { WriteString(theEnv,logName,indentString); }
            else
              { indentFirst = true; }
           }

         /*==========================*/
         /* Print the partial match. */
         /*==========================*/

         if (output == VERBOSE)
           {
            PrintPartialMatch(theEnv,logName,listOfMatches);
            WriteString(theEnv,logName,"\n");
           }

         count++;

         /*============================*/
         /* Move on to the next match. */
         /*============================*/

         listOfMatches = listOfMatches->nextInMemory;
        }
     }

   return count;
  }

#if (CONSTRUCT_COMPILER || BLOAD_AND_BSAVE) && (! RUN_TIME)

/*************************************************************/
/* TagRuleNetwork: Assigns each join in the join network and */
/*   each defrule data structure with a unique integer ID.   */
/*   Also counts the number of defrule and joinNode data     */
/*   structures currently in use.                            */
/*************************************************************/
void TagRuleNetwork(
  Environment *theEnv,
  unsigned long *moduleCount,
  unsigned long *ruleCount,
  unsigned long *joinCount,
  unsigned long *linkCount)
  {
   Defmodule *modulePtr;
   Defrule *rulePtr, *disjunctPtr;
   struct joinLink *theLink;

   *moduleCount = 0;
   *ruleCount = 0;
   *joinCount = 0;
   *linkCount = 0;

   MarkRuleNetwork(theEnv,0);

   for (theLink = DefruleData(theEnv)->LeftPrimeJoins;
        theLink != NULL;
        theLink = theLink->next)
     {
      theLink->bsaveID = *linkCount;
      (*linkCount)++;
     }

   for (theLink = DefruleData(theEnv)->RightPrimeJoins;
        theLink != NULL;
        theLink = theLink->next)
     {
      theLink->bsaveID = *linkCount;
      (*linkCount)++;
     }

   /*===========================*/
   /* Loop through each module. */
   /*===========================*/

   for (modulePtr = GetNextDefmodule(theEnv,NULL);
        modulePtr != NULL;
        modulePtr = GetNextDefmodule(theEnv,modulePtr))
     {
      (*moduleCount)++;
      SetCurrentModule(theEnv,modulePtr);

      /*=========================*/
      /* Loop through each rule. */
      /*=========================*/

      rulePtr = GetNextDefrule(theEnv,NULL);

      while (rulePtr != NULL)
        {
         /*=============================*/
         /* Loop through each disjunct. */
         /*=============================*/

         for (disjunctPtr = rulePtr; disjunctPtr != NULL; disjunctPtr = disjunctPtr->disjunct)
           {
            disjunctPtr->header.bsaveID = *ruleCount;
            (*ruleCount)++;
            TagNetworkTraverseJoins(theEnv,joinCount,linkCount,disjunctPtr->lastJoin);
           }

         rulePtr = GetNextDefrule(theEnv,rulePtr);
        }
     }
  }

/*******************************************************************/
/* TagNetworkTraverseJoins: Traverses the join network for a rule. */
/*******************************************************************/
static void TagNetworkTraverseJoins(
  Environment *theEnv,
  unsigned long *joinCount,
  unsigned long *linkCount,
  struct joinNode *joinPtr)
  {
   struct joinLink *theLink;
   for (;
        joinPtr != NULL;
        joinPtr = joinPtr->lastLevel)
     {
      if (joinPtr->marked == 0)
        {
         joinPtr->marked = 1;
         joinPtr->bsaveID = *joinCount;
         (*joinCount)++;
         for (theLink = joinPtr->nextLinks;
              theLink != NULL;
              theLink = theLink->next)
           {
            theLink->bsaveID = *linkCount;
            (*linkCount)++;
           }
        }

      if (joinPtr->joinFromTheRight)
        { TagNetworkTraverseJoins(theEnv,joinCount,linkCount,(struct joinNode *) joinPtr->rightSideEntryStructure); }
     }
  }

#endif /* (CONSTRUCT_COMPILER || BLOAD_AND_BSAVE) && (! RUN_TIME) */

#endif /* DEFRULE_CONSTRUCT */





