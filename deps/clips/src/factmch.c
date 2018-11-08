   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  08/25/16             */
   /*                                                     */
   /*                 FACT MATCH MODULE                   */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements the algorithm for pattern matching in */
/*   the fact pattern network.                               */
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
/*      6.24: Removed INCREMENTAL_RESET compilation flag.    */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Added support for hashed alpha memories.       */
/*                                                           */
/*            Fix for DR0880. 2008-01-24                     */
/*                                                           */
/*            Added support for hashed comparisons to        */
/*            constants.                                     */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Watch facts for modify command only prints     */
/*            changed slots.                                 */
/*                                                           */
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT

#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "factgen.h"
#include "factrete.h"
#include "incrrset.h"
#include "memalloc.h"
#include "prntutil.h"
#include "reteutil.h"
#include "router.h"
#include "sysdep.h"
#include "tmpltdef.h"

#include "factmch.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static bool                     EvaluatePatternExpression(Environment *,struct factPatternNode *,struct expr *);
   static void                     TraceErrorToJoin(Environment *,struct factPatternNode *,bool);
   static void                     ProcessFactAlphaMatch(Environment *,Fact *,struct multifieldMarker *,struct factPatternNode *);
   static struct factPatternNode  *GetNextFactPatternNode(Environment *,bool,struct factPatternNode *);
   static bool                     SkipFactPatternNode(Environment *,struct factPatternNode *);
   static void                     ProcessMultifieldNode(Environment *,
                                                         struct factPatternNode *,
                                                         struct multifieldMarker *,
                                                         struct multifieldMarker *,size_t,size_t);
   static void                     PatternNetErrorMessage(Environment *,struct factPatternNode *);

/*************************************************************************/
/* FactPatternMatch: Implements the core loop for fact pattern matching. */
/*************************************************************************/
void FactPatternMatch(
  Environment *theEnv,
  Fact *theFact,
  struct factPatternNode *patternPtr,
  size_t offset,
  size_t multifieldsProcessed,
  struct multifieldMarker *markers,
  struct multifieldMarker *endMark)
  {
   size_t theSlotField;
   unsigned short offsetSlot;
   UDFValue theResult;
   struct factPatternNode *tempPtr;
     
   /*=========================================================*/
   /* If there's nothing left in the pattern network to match */
   /* against, then the current traversal of the pattern      */
   /* network needs to back up.                               */
   /*=========================================================*/

   if (patternPtr == NULL) return;

   /*=======================================================*/
   /* The offsetSlot variable indicates the current offset  */
   /* within the multifield slot being pattern matched.     */
   /* (Recall that a multifield wildcard or variable        */
   /* recursively iterates through all possible  bindings.) */
   /* Once a new slot starts being pattern matched, the     */
   /* offset is reset to zero.                              */
   /*=======================================================*/

   offsetSlot = patternPtr->whichSlot;

   /*================================================*/
   /* Set up some global parameters for use by the   */
   /* Rete access functions and general convenience. */
   /*================================================*/

   FactData(theEnv)->CurrentPatternFact = theFact;
   FactData(theEnv)->CurrentPatternMarks = markers;

   /*============================================*/
   /* Loop through each node in pattern network. */
   /*============================================*/

   while (patternPtr != NULL)
     {
      /*=============================================================*/
      /* Determine the position of the field we're going to pattern  */
      /* match. If this routine has been entered recursively because */
      /* of multifield wildcards or variables, then add in the       */
      /* additional offset caused by the values which match these    */
      /* multifields. This offset may be negative (if for example a  */
      /* a multifield matched a zero length value).                  */
      /*=============================================================*/

      theSlotField = patternPtr->whichField;
      if (offsetSlot == patternPtr->whichSlot)
        { theSlotField = theSlotField + offset - multifieldsProcessed; }
        
      /*===================================*/
      /* Determine if we want to skip this */
      /* node during an incremental reset. */
      /*===================================*/

      if (SkipFactPatternNode(theEnv,patternPtr))
        { patternPtr = GetNextFactPatternNode(theEnv,true,patternPtr); }

      /*=========================================================*/
      /* If this is a single field pattern node, then determine  */
      /* if the constraints for the node have been satisfied for */
      /* the current field in the slot being examined.           */
      /*=========================================================*/

      else if (patternPtr->header.singlefieldNode)
        {
         /*==================================================*/
         /* If we're at the last slot in the pattern, make   */
         /* sure the number of fields in the fact correspond */
         /* to the number of fields required by the pattern  */
         /* based on the binding of multifield variables.    */
         /*==================================================*/

         bool skipit = false;
         if (patternPtr->header.endSlot &&
             ((FactData(theEnv)->CurrentPatternMarks == NULL) ?
              false :
              (FactData(theEnv)->CurrentPatternMarks->where.whichSlotNumber == patternPtr->whichSlot)) &&
             (FactData(theEnv)->CurrentPatternFact->theProposition.contents
                  [patternPtr->whichSlot].header->type == MULTIFIELD_TYPE))
           {
            if ((patternPtr->leaveFields + theSlotField) !=
                  FactData(theEnv)->CurrentPatternFact->theProposition.contents
                                      [patternPtr->whichSlot].multifieldValue->length)
              { skipit = true; }
           }

         if (skipit)
           { patternPtr = GetNextFactPatternNode(theEnv,true,patternPtr); }
         else

         if (patternPtr->header.selector)
           {
            if (EvaluatePatternExpression(theEnv,patternPtr,patternPtr->networkTest->nextArg))
              {
               EvaluateExpression(theEnv,patternPtr->networkTest,&theResult);

               tempPtr = (struct factPatternNode *) FindHashedPatternNode(theEnv,patternPtr,theResult.header->type,theResult.value);
              }
            else
              { tempPtr = NULL; }

            if (tempPtr != NULL)
              {
               if (SkipFactPatternNode(theEnv,tempPtr))
                 { patternPtr = GetNextFactPatternNode(theEnv,true,patternPtr); }
               else
                 {
                  if (tempPtr->header.stopNode)
                    { ProcessFactAlphaMatch(theEnv,theFact,markers,tempPtr); }

                  patternPtr = GetNextFactPatternNode(theEnv,false,tempPtr);
                 }
              }
            else
              { patternPtr = GetNextFactPatternNode(theEnv,true,patternPtr); }
           }

         /*=============================================*/
         /* If the constraints are satisified, then ... */
         /*=============================================*/

         else if (EvaluatePatternExpression(theEnv,patternPtr,patternPtr->networkTest))
           {
            /*=======================================================*/
            /* If a leaf pattern node has been successfully reached, */
            /* then the pattern has been satisified. Generate an     */
            /* alpha match to store in the pattern node.             */
            /*=======================================================*/

            if (patternPtr->header.stopNode)
              { ProcessFactAlphaMatch(theEnv,theFact,markers,patternPtr); }

            /*===================================*/
            /* Move on to the next pattern node. */
            /*===================================*/

            patternPtr = GetNextFactPatternNode(theEnv,false,patternPtr);
           }

         /*==============================================*/
         /* Otherwise, move on to the next pattern node. */
         /*==============================================*/

         else
           { patternPtr = GetNextFactPatternNode(theEnv,true,patternPtr); }
        }

      /*======================================================*/
      /* If this is a multifield pattern node, then determine */
      /* if the constraints for the node have been satisfied  */
      /* for the current field in the slot being examined.    */
      /*======================================================*/

      else if (patternPtr->header.multifieldNode)
        {
         /*========================================================*/
         /* Determine if the multifield pattern node's constraints */
         /* are satisfied. If we've traversed to a different slot  */
         /* than the one we started this routine with, then the    */
         /* offset into the slot is reset to zero.                 */
         /*========================================================*/

         if (offsetSlot == patternPtr->whichSlot)
           { ProcessMultifieldNode(theEnv,patternPtr,markers,endMark,offset,multifieldsProcessed); }
         else
           { ProcessMultifieldNode(theEnv,patternPtr,markers,endMark,0,0); }

         /*===================================================*/
         /* Move on to the next pattern node. Since the lower */
         /* branches of the pattern network have already been */
         /* recursively processed by ProcessMultifieldNode,   */
         /* we get the next pattern node by treating this     */
         /* multifield pattern node as if it were a single    */
         /* field pattern node that failed its constraint.    */
         /*===================================================*/

         patternPtr = GetNextFactPatternNode(theEnv,true,patternPtr);
        }
     }
  }

/**************************************************************/
/* ProcessMultifieldNode: Handles recursive pattern matching  */
/*  when a multifield wildcard or variable is encountered as  */
/*  a slot constraint. The pattern matching routine is called */
/*  iteratively for each possible binding of the multifield   */
/*  wildcard or variable.                                     */
/**************************************************************/
static void ProcessMultifieldNode(
  Environment *theEnv,
  struct factPatternNode *thePattern,
  struct multifieldMarker *markers,
  struct multifieldMarker *endMark,
  size_t offset,
  size_t multifieldsProcessed)
  {
   struct multifieldMarker *newMark, *oldMark;
   size_t fieldsRemaining;
   size_t i;
   size_t repeatCount;
   Multifield *theSlotValue;
   UDFValue theResult;
   struct factPatternNode *tempPtr;
   bool success;

   /*========================================*/
   /* Get a pointer to the slot value of the */
   /* multifield slot being pattern matched. */
   /*========================================*/

   theSlotValue =
     FactData(theEnv)->CurrentPatternFact->theProposition.contents[thePattern->whichSlot].multifieldValue;

   /*===============================================*/
   /* Save the value of the markers already stored. */
   /*===============================================*/

   oldMark = markers;

   /*===========================================*/
   /* Create a new multifield marker and append */
   /* it to the end of the current list.        */
   /*===========================================*/

   newMark = get_struct(theEnv,multifieldMarker);
   
   newMark->whichField = thePattern->whichField - 1;
   newMark->where.whichSlotNumber = thePattern->whichSlot;
   newMark->startPosition = (thePattern->whichField - 1) + offset - multifieldsProcessed;
   newMark->next = NULL;

   if (endMark == NULL)
     {
      markers = newMark;
      FactData(theEnv)->CurrentPatternMarks = markers;
     }
   else
     { endMark->next = newMark; }

   /*============================================*/
   /* Handle a multifield constraint as the last */
   /* constraint of a slot as a special case.    */
   /*============================================*/

   if (thePattern->header.endSlot)
     {
      if (theSlotValue->length < (newMark->startPosition + thePattern->leaveFields))
        { newMark->range = 0; }
      else
        { newMark->range = theSlotValue->length - (newMark->startPosition + thePattern->leaveFields); }

      /*===========================================*/
      /* Determine if the constraint is satisfied. */
      /*===========================================*/

      if (thePattern->header.selector)
        {
         if (EvaluatePatternExpression(theEnv,thePattern,thePattern->networkTest->nextArg))
           {
            EvaluateExpression(theEnv,thePattern->networkTest,&theResult);

            thePattern = (struct factPatternNode *) FindHashedPatternNode(theEnv,thePattern,theResult.header->type,theResult.value);
            if (thePattern != NULL)
              { success = true; }
            else
              { success = false; }
           }
         else
           { success = false; }
        }
      else if ((thePattern->networkTest == NULL) ?
          true :
          (EvaluatePatternExpression(theEnv,thePattern,thePattern->networkTest)))
        { success = true; }
      else
        { success = false; }

      if (success)
        {
         /*=======================================================*/
         /* If a leaf pattern node has been successfully reached, */
         /* then the pattern has been satisified. Generate an     */
         /* alpha match to store in the pattern node.             */
         /*=======================================================*/

         if (thePattern->header.stopNode)
           { ProcessFactAlphaMatch(theEnv,FactData(theEnv)->CurrentPatternFact,FactData(theEnv)->CurrentPatternMarks,thePattern); }

         /*=============================================*/
         /* Recursively continue pattern matching based */
         /* on the multifield binding just generated.   */
         /*=============================================*/

         FactPatternMatch(theEnv,FactData(theEnv)->CurrentPatternFact,
                          thePattern->nextLevel,0,0,FactData(theEnv)->CurrentPatternMarks,newMark);
        }

      /*================================================*/
      /* Discard the multifield marker since we've done */
      /* all the pattern matching for this binding of   */
      /* the multifield slot constraint.                */
      /*================================================*/

      rtn_struct(theEnv,multifieldMarker,newMark);
      if (endMark != NULL) endMark->next = NULL;
      FactData(theEnv)->CurrentPatternMarks = oldMark;
      return;
     }

   /*===============================================================*/
   /* Stop matching if there aren't enough values left in the fact. */
   /*===============================================================*/

   if (theSlotValue->length < (newMark->startPosition + thePattern->leaveFields))
     {
      rtn_struct(theEnv,multifieldMarker,newMark);
      if (endMark != NULL) endMark->next = NULL;
      FactData(theEnv)->CurrentPatternMarks = oldMark;
      return;
     }
     
   /*==============================================*/
   /* Perform matching for nodes beneath this one. */
   /*==============================================*/
   
   fieldsRemaining = theSlotValue->length - (newMark->startPosition + thePattern->leaveFields);
     
   for (i = 0; i <= fieldsRemaining; i++)
     {
      repeatCount = fieldsRemaining - i;
      
      newMark->range = repeatCount;

      if (thePattern->header.selector)
        {
         if (EvaluatePatternExpression(theEnv,thePattern,thePattern->networkTest->nextArg))
           {
            EvaluateExpression(theEnv,thePattern->networkTest,&theResult);

            tempPtr = (struct factPatternNode *) FindHashedPatternNode(theEnv,thePattern,theResult.header->type,theResult.value);
            if (tempPtr != NULL)
              {
               FactPatternMatch(theEnv,FactData(theEnv)->CurrentPatternFact,
                                tempPtr->nextLevel,offset + repeatCount,multifieldsProcessed+1,
                                FactData(theEnv)->CurrentPatternMarks,newMark);
              }
           }
        }
      else if ((thePattern->networkTest == NULL) ?
               true :
               (EvaluatePatternExpression(theEnv,thePattern,thePattern->networkTest)))
        {
         FactPatternMatch(theEnv,FactData(theEnv)->CurrentPatternFact,
                          thePattern->nextLevel,offset + repeatCount,multifieldsProcessed+1,
                          FactData(theEnv)->CurrentPatternMarks,newMark);
        }
     }

    /*======================================================*/
    /* Get rid of the marker created for a multifield node. */
    /*======================================================*/

    rtn_struct(theEnv,multifieldMarker,newMark);
    if (endMark != NULL) endMark->next = NULL;
    FactData(theEnv)->CurrentPatternMarks = oldMark;
   }

/******************************************************/
/* GetNextFactPatternNode: Returns the next node in a */
/*   pattern network tree to be traversed. The next   */
/*   node is computed using a depth first traversal.  */
/******************************************************/
static struct factPatternNode *GetNextFactPatternNode(
  Environment *theEnv,
  bool finishedMatching,
  struct factPatternNode *thePattern)
  {
   EvaluationData(theEnv)->EvaluationError = false;

   /*===================================================*/
   /* If pattern matching was successful at the current */
   /* node in the tree and it's possible to go deeper   */
   /* into the tree, then move down to the next level.  */
   /*===================================================*/

   if (finishedMatching == false)
     { if (thePattern->nextLevel != NULL) return(thePattern->nextLevel); }

   /*================================================*/
   /* Keep backing up toward the root of the pattern */
   /* network until a side branch can be taken.      */
   /*================================================*/

   while ((thePattern->rightNode == NULL) ||
          ((thePattern->lastLevel != NULL) &&
           (thePattern->lastLevel->header.selector)))
     {
      /*========================================*/
      /* Back up to check the next side branch. */
      /*========================================*/

      thePattern = thePattern->lastLevel;

      /*======================================*/
      /* If we branched up from the root, the */
      /* entire tree has been traversed.      */
      /*======================================*/

      if (thePattern == NULL) return NULL;

      /*======================================*/
      /* Skip selector constants and pop back */
      /* back to the selector node.           */
      /*======================================*/

      if ((thePattern->lastLevel != NULL) &&
          (thePattern->lastLevel->header.selector))
        { thePattern = thePattern->lastLevel; }

      /*===================================================*/
      /* If we branched up to a multifield node, then stop */
      /* since these nodes are handled recursively. The    */
      /* previous call to the pattern matching algorithm   */
      /* on the stack will handle backing up to the nodes  */
      /* above the multifield node in the pattern network. */
      /*===================================================*/

      if (thePattern->header.multifieldNode) return NULL;
     }

   /*==================================*/
   /* Move on to the next side branch. */
   /*==================================*/

   return(thePattern->rightNode);
  }

/*******************************************************/
/* ProcessFactAlphaMatch: When a fact pattern has been */
/*   satisfied, this routine creates an alpha match to */
/*   store in the pattern network and then sends the   */
/*   new alpha match through the join network.         */
/*******************************************************/
static void ProcessFactAlphaMatch(
  Environment *theEnv,
  Fact *theFact,
  struct multifieldMarker *theMarks,
  struct factPatternNode *thePattern)
  {
   struct partialMatch *theMatch;
   struct patternMatch *listOfMatches;
   struct joinNode *listOfJoins;
   unsigned long hashValue;

  /*============================================*/
  /* Create the hash value for the alpha match. */
  /*============================================*/

  hashValue = ComputeRightHashValue(theEnv,&thePattern->header);

  /*===========================================*/
  /* Create the partial match for the pattern. */
  /*===========================================*/

  theMatch = CreateAlphaMatch(theEnv,theFact,theMarks,(struct patternNodeHeader *) &thePattern->header,hashValue);
  theMatch->owner = &thePattern->header;

  /*=======================================================*/
  /* Add the pattern to the list of matches for this fact. */
  /*=======================================================*/

  listOfMatches = (struct patternMatch *) theFact->list;
  theFact->list = get_struct(theEnv,patternMatch);
  ((struct patternMatch *) theFact->list)->next = listOfMatches;
  ((struct patternMatch *) theFact->list)->matchingPattern = (struct patternNodeHeader *) thePattern;
  ((struct patternMatch *) theFact->list)->theMatch = theMatch;

  /*================================================================*/
  /* Send the partial match to the joins connected to this pattern. */
  /*================================================================*/

  for (listOfJoins = thePattern->header.entryJoin;
       listOfJoins != NULL;
       listOfJoins = listOfJoins->rightMatchNode)
     { NetworkAssert(theEnv,theMatch,listOfJoins); }
  }

/*****************************************************************/
/* EvaluatePatternExpression: Performs a faster evaluation for   */
/*   fact pattern network expressions than if EvaluateExpression */
/*   were used directly.                                         */
/*****************************************************************/
static bool EvaluatePatternExpression(
  Environment *theEnv,
  struct factPatternNode *patternPtr,
  struct expr *theTest)
  {
   UDFValue theResult;
   struct expr *oldArgument;
   bool rv;

   /*=====================================*/
   /* A pattern node without a constraint */
   /* is always satisfied.                */
   /*=====================================*/

   if (theTest == NULL) return true;

   /*======================================*/
   /* Evaluate pattern network primitives. */
   /*======================================*/

   switch(theTest->type)
     {
      /*==============================================*/
      /* This primitive compares the value stored in  */
      /* a single field slot to a constant for either */
      /* equality or inequality.                      */
      /*==============================================*/

      case FACT_PN_CONSTANT1:
        oldArgument = EvaluationData(theEnv)->CurrentExpression;
        EvaluationData(theEnv)->CurrentExpression = theTest;
        rv = FactPNConstant1(theEnv,theTest->value,&theResult);
        EvaluationData(theEnv)->CurrentExpression = oldArgument;
        return(rv);

      /*=============================================*/
      /* This primitive compares the value stored in */
      /* a multifield slot to a constant for either  */
      /* equality or inequality.                     */
      /*=============================================*/

      case FACT_PN_CONSTANT2:
        oldArgument = EvaluationData(theEnv)->CurrentExpression;
        EvaluationData(theEnv)->CurrentExpression = theTest;
        rv = FactPNConstant2(theEnv,theTest->value,&theResult);
        EvaluationData(theEnv)->CurrentExpression = oldArgument;
        return(rv);

      /*================================================*/
      /* This primitive determines if a multifield slot */
      /* contains at least a certain number of fields.  */
      /*================================================*/

      case FACT_SLOT_LENGTH:
        oldArgument = EvaluationData(theEnv)->CurrentExpression;
        EvaluationData(theEnv)->CurrentExpression = theTest;
        rv = FactSlotLength(theEnv,theTest->value,&theResult);
        EvaluationData(theEnv)->CurrentExpression = oldArgument;
        return(rv);
     }

   /*==============================================*/
   /* Evaluate "or" expressions by evaluating each */
   /* argument and return true if any of them      */
   /* evaluated to true, otherwise return false.   */
   /*==============================================*/

   if (theTest->value == ExpressionData(theEnv)->PTR_OR)
     {
      for (theTest = theTest->argList;
           theTest != NULL;
           theTest = theTest->nextArg)
        {
         if (EvaluatePatternExpression(theEnv,patternPtr,theTest) == true)
           {
            if (EvaluationData(theEnv)->EvaluationError) return false;
            return true;
           }
         if (EvaluationData(theEnv)->EvaluationError) return false;
        }

      return false;
     }

   /*===============================================*/
   /* Evaluate "and" expressions by evaluating each */
   /* argument and return false if any of them      */
   /* evaluated to false, otherwise return true.    */
   /*===============================================*/

   else if (theTest->value == ExpressionData(theEnv)->PTR_AND)
     {
      for (theTest = theTest->argList;
           theTest != NULL;
           theTest = theTest->nextArg)
        {
         if (EvaluatePatternExpression(theEnv,patternPtr,theTest) == false)
           { return false; }
         if (EvaluationData(theEnv)->EvaluationError) return false;
        }

      return true;
     }

   /*==========================================================*/
   /* Evaluate all other expressions using EvaluateExpression. */
   /*==========================================================*/

   if (EvaluateExpression(theEnv,theTest,&theResult))
     {
      PatternNetErrorMessage(theEnv,patternPtr);
      return false;
     }

   if (theResult.value == FalseSymbol(theEnv))
     { return false; }

   return true;
  }

/************************************************************************/
/* PatternNetErrorMessage: Prints the informational header to the error */
/*   message that occurs when a error occurs as the  result of          */
/*   evaluating an expression in the fact pattern network. Prints the   */
/*   fact currently being pattern matched and the field number or slot  */
/*   name in the pattern from which the error originated. The error is  */
/*   then trace to the point where the pattern enters the join network  */
/*   so that the names of the rule which utilize the pattern can also   */
/*   be printed.                                                        */
/************************************************************************/
static void PatternNetErrorMessage(
  Environment *theEnv,
  struct factPatternNode *patternPtr)
  {
   char buffer[60];
   struct templateSlot *theSlots;
   unsigned short i;

   /*=======================================*/
   /* Print the fact being pattern matched. */
   /*=======================================*/

   PrintErrorID(theEnv,"FACTMCH",1,true);
   WriteString(theEnv,STDERR,"This error occurred in the fact pattern network.\n");
   WriteString(theEnv,STDERR,"   Currently active fact: ");
   PrintFact(theEnv,STDERR,FactData(theEnv)->CurrentPatternFact,false,false,NULL);
   WriteString(theEnv,STDERR,"\n");

   /*==============================================*/
   /* Print the field position or slot name of the */
   /* pattern from which the error originated.     */
   /*==============================================*/

   if (FactData(theEnv)->CurrentPatternFact->whichDeftemplate->implied)
     { gensprintf(buffer,"   Problem resides in field #%d\n",patternPtr->whichField); }
   else
     {
      theSlots = FactData(theEnv)->CurrentPatternFact->whichDeftemplate->slotList;
      for (i = 0; i < patternPtr->whichSlot; i++) theSlots = theSlots->next;
      gensprintf(buffer,"   Problem resides in slot %s\n",theSlots->slotName->contents);
     }

   WriteString(theEnv,STDERR,buffer);

   /*==========================================================*/
   /* Trace the pattern to its entry point to the join network */
   /* (which then traces to the defrule data structure so that */
   /* the name(s) of the rule(s) utilizing the patterns can be */
   /* printed).                                                */
   /*==========================================================*/

   TraceErrorToJoin(theEnv,patternPtr,false);
   WriteString(theEnv,STDERR,"\n");
  }

/***************************************************************************/
/* TraceErrorToJoin: Traces the cause of an evaluation error which occured */
/*   in the fact pattern network to the entry join in the join network for */
/*   the pattern from which the error originated. Once the entry join is   */
/*   reached, the error is then traced to the defrule data structures so   */
/*   that the name of the rule(s) containing the pattern can be printed.   */
/***************************************************************************/
static void TraceErrorToJoin(
  Environment *theEnv,
  struct factPatternNode *patternPtr,
  bool traceRight)
  {
   struct joinNode *joinPtr;

   while (patternPtr != NULL)
     {
      if (patternPtr->header.stopNode)
        {
         for (joinPtr = patternPtr->header.entryJoin;
              joinPtr != NULL;
              joinPtr = joinPtr->rightMatchNode)
           { TraceErrorToRule(theEnv,joinPtr,"      "); }
        }
      else
        { TraceErrorToJoin(theEnv,patternPtr->nextLevel,true); }

      if (traceRight) patternPtr = patternPtr->rightNode;
      else patternPtr = NULL;
     }
  }

/***********************************************************************/
/* SkipFactPatternNode: During an incremental reset, only fact pattern */
/*   nodes associated with new patterns are traversed. Given a pattern */
/*   node, this routine will return true if the pattern node should be */
/*   traversed during incremental reset pattern matching or false if   */
/*   the node should be skipped.                                       */
/***********************************************************************/
static bool SkipFactPatternNode(
  Environment *theEnv,
  struct factPatternNode *thePattern)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)
   if (EngineData(theEnv)->IncrementalResetInProgress &&
       (thePattern->header.initialize == false))
     { return true; }
#endif

   return false;
  }

/***************************************************************/
/* MarkFactPatternForIncrementalReset: Sets the initialization */
/*  field of a fact pattern for use with incremental reset.    */
/*  This is called before an incremental reset for newly added */
/*  patterns to indicate that the pattern nodes should be      */
/*  traversed and then after an incremental reset to indicate  */
/*  that the nodes were traversed ("initialized") by the       */
/*  incremental reset.                                         */
/***************************************************************/
void MarkFactPatternForIncrementalReset(
  Environment *theEnv,
  struct patternNodeHeader *thePattern,
  bool value)
  {
   struct factPatternNode *patternPtr = (struct factPatternNode *) thePattern;
   struct joinNode *theJoin;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   /*=====================================*/
   /* We should be passed a valid pointer */
   /* to a fact pattern network node.     */
   /*=====================================*/

   Bogus(patternPtr == NULL);

   /*===============================================================*/
   /* If the pattern was previously initialized,  then don't bother */
   /* with it unless the pattern was subsumed by another pattern    */
   /* and associated with a join that hasn't been initialized.      */
   /* DR0880 2008-01-24                                             */
   /*===============================================================*/

   if (patternPtr->header.initialize == false)
     {
      for (theJoin = patternPtr->header.entryJoin;
           theJoin != NULL;
           theJoin = theJoin->rightMatchNode)
        {
         if (theJoin->initialize == false)
           { return; }
        }
     }

   /*======================================================*/
   /* Set the initialization field of this pattern network */
   /* node and all pattern network nodes which preceed it. */
   /*======================================================*/

   while (patternPtr != NULL)
     {
      patternPtr->header.initialize = value;
      patternPtr = patternPtr->lastLevel;
     }
  }

/**************************************************************/
/* FactsIncrementalReset: Incremental reset function for the  */
/*   fact pattern network. Asserts all facts in the fact-list */
/*   so that they repeat the pattern matching process. During */
/*   an incremental reset, newly added patterns should be the */
/*   only active patterns in the fact pattern network.        */
/**************************************************************/
void FactsIncrementalReset(
  Environment *theEnv)
  {
   Fact *factPtr;

   for (factPtr = GetNextFact(theEnv,NULL);
        factPtr != NULL;
        factPtr = GetNextFact(theEnv,factPtr))
     {
      EngineData(theEnv)->JoinOperationInProgress = true;
      FactPatternMatch(theEnv,factPtr,
                       factPtr->whichDeftemplate->patternNetwork,
                       0,0,NULL,NULL);
      EngineData(theEnv)->JoinOperationInProgress = false;
     }
  }

#endif /* DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT */

