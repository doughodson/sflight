   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  11/01/16             */
   /*                                                     */
   /*          FACT RETE ACCESS FUNCTIONS MODULE          */
   /*******************************************************/

/*************************************************************/
/* Purpose: Rete access functions for fact pattern matching. */
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
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Support for hashing optimizations.             */
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
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT

#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "factgen.h"
#include "factmch.h"
#include "incrrset.h"
#include "memalloc.h"
#include "multifld.h"
#include "reteutil.h"
#include "router.h"

#include "factrete.h"

/***************************************************************/
/* FactPNGetVar1: Fact pattern network function for extracting */
/*   a variable's value. This is the most generalized routine. */
/***************************************************************/
bool FactPNGetVar1(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   size_t adjustedField;
   unsigned short theField, theSlot;
   Fact *factPtr;
   CLIPSValue *fieldPtr;
   struct multifieldMarker *marks;
   Multifield *segmentPtr;
   size_t extent;
   struct factGetVarPN1Call *hack;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factGetVarPN1Call *) ((CLIPSBitMap *) theValue)->contents;

   /*=====================================================*/
   /* Get the pointer to the fact from the partial match. */
   /*=====================================================*/

   factPtr = FactData(theEnv)->CurrentPatternFact;
   marks = FactData(theEnv)->CurrentPatternMarks;

   /*==========================================================*/
   /* Determine if we want to retrieve the fact address of the */
   /* fact, rather than retrieving a field from the fact.      */
   /*==========================================================*/

   if (hack->factAddress)
     {
      returnValue->value = factPtr;
      return true;
     }

   /*=========================================================*/
   /* Determine if we want to retrieve the entire slot value. */
   /*=========================================================*/

   if (hack->allFields)
     {
      theSlot = hack->whichSlot;
      fieldPtr = &factPtr->theProposition.contents[theSlot];
      returnValue->value = fieldPtr->value;
      if (returnValue->header->type == MULTIFIELD_TYPE)
        {
         returnValue->begin = 0;
         returnValue->range = fieldPtr->multifieldValue->length;
        }

      return true;
     }

   /*====================================================*/
   /* If the slot being accessed is a single field slot, */
   /* then just return the single value found in that    */
   /* slot. The multifieldMarker data structures do not  */
   /* have to be considered since access to a single     */
   /* field slot is not affected by variable bindings    */
   /* from multifield slots.                             */
   /*====================================================*/

   theField = hack->whichField;
   theSlot = hack->whichSlot;
   fieldPtr = &factPtr->theProposition.contents[theSlot];

   /*==========================================================*/
   /* Retrieve a value from a multifield slot. First determine */
   /* the range of fields for the variable being retrieved.    */
   /*==========================================================*/

   extent = SIZE_MAX;
   adjustedField = AdjustFieldPosition(theEnv,marks,theField,theSlot,&extent);

   /*=============================================================*/
   /* If a range of values are being retrieved (i.e. a multifield */
   /* variable), then return the values as a multifield.          */
   /*=============================================================*/

   if (extent != SIZE_MAX)
     {
      returnValue->value = fieldPtr->value;
      returnValue->begin = adjustedField;
      returnValue->range = extent;
      return true;
     }

   /*========================================================*/
   /* Otherwise a single field value is being retrieved from */
   /* a multifield slot. Just return the type and value.     */
   /*========================================================*/

   segmentPtr = fieldPtr->multifieldValue;
   fieldPtr = &segmentPtr->contents[adjustedField];

   returnValue->value = fieldPtr->value;

   return true;
  }

/**************************************************/
/* FactPNGetVar2: Fact pattern network function   */
/*   for extracting a variable's value. The value */
/*   extracted is from a single field slot.       */
/**************************************************/
bool FactPNGetVar2(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   Fact *factPtr;
   struct factGetVarPN2Call *hack;
   CLIPSValue *fieldPtr;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factGetVarPN2Call *) ((CLIPSBitMap *) theValue)->contents;

   /*==============================*/
   /* Get the pointer to the fact. */
   /*==============================*/

   factPtr = FactData(theEnv)->CurrentPatternFact;

   /*============================================*/
   /* Extract the value from the specified slot. */
   /*============================================*/

   fieldPtr = &factPtr->theProposition.contents[hack->whichSlot];

   returnValue->value = fieldPtr->value;

   return true;
  }

/*****************************************************************/
/* FactPNGetVar3: Fact pattern network function for extracting a */
/*   variable's value. The value extracted is from a multifield  */
/*   slot that contains at most one multifield variable.         */
/*****************************************************************/
bool FactPNGetVar3(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   Fact *factPtr;
   Multifield *segmentPtr;
   CLIPSValue *fieldPtr;
   struct factGetVarPN3Call *hack;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factGetVarPN3Call *) ((CLIPSBitMap *) theValue)->contents;

   /*==============================*/
   /* Get the pointer to the fact. */
   /*==============================*/

   factPtr = FactData(theEnv)->CurrentPatternFact;

   /*============================================================*/
   /* Get the multifield value from which the data is retrieved. */
   /*============================================================*/

   segmentPtr = factPtr->theProposition.contents[hack->whichSlot].multifieldValue;

   /*=========================================*/
   /* If the beginning and end flags are set, */
   /* then retrieve a multifield value.       */
   /*=========================================*/

   if (hack->fromBeginning && hack->fromEnd)
     {
      returnValue->value = segmentPtr;
      returnValue->begin = hack->beginOffset;
      returnValue->range = segmentPtr->length - (hack->endOffset + hack->beginOffset);
      return true;
     }

   /*=====================================================*/
   /* Return a single field value from a multifield slot. */
   /*=====================================================*/

   if (hack->fromBeginning)
     { fieldPtr = &segmentPtr->contents[hack->beginOffset]; }
   else
     { fieldPtr = &segmentPtr->contents[segmentPtr->length - (hack->endOffset + 1)]; }

   returnValue->value = fieldPtr->value;

   return true;
  }

/******************************************************/
/* FactPNConstant1: Fact pattern network function for */
/*   comparing a value stored in a single field slot  */
/*   to a constant for either equality or inequality. */
/******************************************************/
bool FactPNConstant1(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
#if MAC_XCD
#pragma unused(returnValue)
#endif
   struct factConstantPN1Call *hack;
   CLIPSValue *fieldPtr;
   struct expr *theConstant;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factConstantPN1Call *) ((CLIPSBitMap *) theValue)->contents;

   /*============================================*/
   /* Extract the value from the specified slot. */
   /*============================================*/

   fieldPtr = &FactData(theEnv)->CurrentPatternFact->theProposition.contents[hack->whichSlot];

   /*====================================*/
   /* Compare the value to the constant. */
   /*====================================*/

   theConstant = GetFirstArgument();
   if (theConstant->value != fieldPtr->value)
     {
      if (1 - hack->testForEquality)
        { return true; }
      else
        { return false; }
     }

   if (hack->testForEquality)
     { return true; }
   else
     { return false; }
  }

/****************************************************************/
/* FactPNConstant2: Fact pattern network function for comparing */
/*   a value stored in a slot to a constant for either equality */
/*   or inequality. The value being retrieved from the slot has */
/*   no multifields to its right (thus it can be retrieved      */
/*   relative to the beginning).                                */
/****************************************************************/
bool FactPNConstant2(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
#if MAC_XCD
#pragma unused(returnValue)
#endif
   struct factConstantPN2Call *hack;
   CLIPSValue *fieldPtr;
   struct expr *theConstant;
   Multifield *segmentPtr;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factConstantPN2Call *) ((CLIPSBitMap *) theValue)->contents;

   /*==========================================================*/
   /* Extract the value from the specified slot. Note that the */
   /* test to determine the slot's type (multifield) should be */
   /* unnecessary since this routine should only be used for   */
   /* multifield slots.                                        */
   /*==========================================================*/

   fieldPtr = &FactData(theEnv)->CurrentPatternFact->theProposition.contents[hack->whichSlot];

   if (fieldPtr->header->type == MULTIFIELD_TYPE)
     {
      segmentPtr = fieldPtr->multifieldValue;

      if (hack->fromBeginning)
        { fieldPtr = &segmentPtr->contents[hack->offset]; }
      else
        {
         fieldPtr = &segmentPtr->contents[segmentPtr->length -
                    (hack->offset + 1)];
        }
     }

   /*====================================*/
   /* Compare the value to the constant. */
   /*====================================*/

   theConstant = GetFirstArgument();
   if (theConstant->value != fieldPtr->value)
     {
      if (1 - hack->testForEquality)
        { return true; }
      else
        { return false; }
     }
   
   if (hack->testForEquality)
     { return true; }
   else
     { return false; }
  }

/**************************************************************/
/* FactJNGetVar1: Fact join network function for extracting a */
/*   variable's value. This is the most generalized routine.  */
/**************************************************************/
bool FactJNGetVar1(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   size_t adjustedField;
   unsigned short theField, theSlot;
   Fact *factPtr;
   CLIPSValue *fieldPtr;
   struct multifieldMarker *marks;
   Multifield *segmentPtr;
   size_t extent;
   struct factGetVarJN1Call *hack;
   Multifield *theSlots = NULL;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factGetVarJN1Call *) ((CLIPSBitMap *) theValue)->contents;

   /*=====================================================*/
   /* Get the pointer to the fact from the partial match. */
   /*=====================================================*/

   if (hack->lhs)
     {
      factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem;
      marks = get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->markers;
     }
   else if (hack->rhs)
     {
      factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,hack->whichPattern)->matchingItem;
      marks = get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,hack->whichPattern)->markers;
     }
   else if (EngineData(theEnv)->GlobalRHSBinds == NULL)
     {
      factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem;
      marks = get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->markers;
     }
   else if ((EngineData(theEnv)->GlobalJoin->depth - 1) == hack->whichPattern)
     {
      factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,0)->matchingItem;
      marks = get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,0)->markers;
     }
   else
     {
      factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem;
      marks = get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->markers;
     }

   /*==========================================================*/
   /* Determine if we want to retrieve the fact address of the */
   /* fact, rather than retrieving a field from the fact.      */
   /*==========================================================*/

   if (hack->factAddress)
     {
      returnValue->value = factPtr;
      return true;
     }

   if ((factPtr->basisSlots != NULL) &&
       (! EngineData(theEnv)->JoinOperationInProgress))
     { theSlots = factPtr->basisSlots; }
   else
     { theSlots = &factPtr->theProposition; }

   /*=========================================================*/
   /* Determine if we want to retrieve the entire slot value. */
   /*=========================================================*/

   if (hack->allFields)
     {
      theSlot = hack->whichSlot;
      fieldPtr = &theSlots->contents[theSlot];
      returnValue->value = fieldPtr->value;
      if (returnValue->header->type == MULTIFIELD_TYPE)
        {
         returnValue->begin = 0;
         returnValue->range = fieldPtr->multifieldValue->length;
        }

      return true;
     }

   /*====================================================*/
   /* If the slot being accessed is a single field slot, */
   /* then just return the single value found in that    */
   /* slot. The multifieldMarker data structures do not  */
   /* have to be considered since access to a single     */
   /* field slot is not affected by variable bindings    */
   /* from multifield slots.                             */
   /*====================================================*/

   theField = hack->whichField;
   theSlot = hack->whichSlot;
   fieldPtr = &theSlots->contents[theSlot];

   if (fieldPtr->header->type != MULTIFIELD_TYPE)
     {
      returnValue->value = fieldPtr->value;
      return true;
     }

   /*==========================================================*/
   /* Retrieve a value from a multifield slot. First determine */
   /* the range of fields for the variable being retrieved.    */
   /*==========================================================*/

   extent = SIZE_MAX;
   adjustedField = AdjustFieldPosition(theEnv,marks,theField,theSlot,&extent);

   /*=============================================================*/
   /* If a range of values are being retrieved (i.e. a multifield */
   /* variable), then return the values as a multifield.          */
   /*=============================================================*/

   if (extent != SIZE_MAX)
     {
      returnValue->value = fieldPtr->value;
      returnValue->begin = adjustedField;
      returnValue->range = extent;
      return true;
     }

   /*========================================================*/
   /* Otherwise a single field value is being retrieved from */
   /* a multifield slot. Just return the type and value.     */
   /*========================================================*/

   segmentPtr = theSlots->contents[theSlot].multifieldValue;
   fieldPtr = &segmentPtr->contents[adjustedField];

   returnValue->value = fieldPtr->value;

   return true;
  }

/*************************************************/
/* FactJNGetVar2: Fact join network function for */
/*   extracting a variable's value. The value    */
/*   extracted is from a single field slot.      */
/*************************************************/
bool FactJNGetVar2(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   Fact *factPtr;
   struct factGetVarJN2Call *hack;
   CLIPSValue *fieldPtr;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factGetVarJN2Call *) ((CLIPSBitMap *) theValue)->contents;

   /*=====================================================*/
   /* Get the pointer to the fact from the partial match. */
   /*=====================================================*/

   if (hack->lhs)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem; }
   else if (hack->rhs)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,hack->whichPattern)->matchingItem; }
   else if (EngineData(theEnv)->GlobalRHSBinds == NULL)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem; }
   else if ((EngineData(theEnv)->GlobalJoin->depth - 1) == hack->whichPattern)
	 { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,0)->matchingItem; }
   else
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem; }

   /*============================================*/
   /* Extract the value from the specified slot. */
   /*============================================*/

   if ((factPtr->basisSlots != NULL) &&
       (! EngineData(theEnv)->JoinOperationInProgress))
     { fieldPtr = &factPtr->basisSlots->contents[hack->whichSlot]; }
   else
     { fieldPtr = &factPtr->theProposition.contents[hack->whichSlot]; }

   returnValue->value = fieldPtr->value;

   return true;
  }

/****************************************************************/
/* FactJNGetVar3: Fact join network function for extracting a   */
/*   variable's value. The value extracted is from a multifield */
/*   slot that contains at most one multifield variable.        */
/****************************************************************/
bool FactJNGetVar3(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   Fact *factPtr;
   Multifield *segmentPtr;
   CLIPSValue *fieldPtr;
   struct factGetVarJN3Call *hack;

   /*==========================================*/
   /* Retrieve the arguments for the function. */
   /*==========================================*/

   hack = (struct factGetVarJN3Call *) ((CLIPSBitMap *) theValue)->contents;

   /*=====================================================*/
   /* Get the pointer to the fact from the partial match. */
   /*=====================================================*/

   if (hack->lhs)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem; }
   else if (hack->rhs)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,hack->whichPattern)->matchingItem; }
   else if (EngineData(theEnv)->GlobalRHSBinds == NULL)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem; }
   else if ((EngineData(theEnv)->GlobalJoin->depth - 1) == hack->whichPattern)
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,0)->matchingItem; }
   else
     { factPtr = (Fact *) get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,hack->whichPattern)->matchingItem; }

   /*============================================================*/
   /* Get the multifield value from which the data is retrieved. */
   /*============================================================*/

   if ((factPtr->basisSlots != NULL) &&
       (! EngineData(theEnv)->JoinOperationInProgress))
     { segmentPtr = factPtr->basisSlots->contents[hack->whichSlot].multifieldValue; }
   else
     { segmentPtr = factPtr->theProposition.contents[hack->whichSlot].multifieldValue; }

   /*=========================================*/
   /* If the beginning and end flags are set, */
   /* then retrieve a multifield value.       */
   /*=========================================*/

   if (hack->fromBeginning && hack->fromEnd)
     {
      returnValue->value = segmentPtr;
      returnValue->begin = hack->beginOffset;
      returnValue->range = segmentPtr->length - (hack->endOffset + hack->beginOffset);
      return true;
     }

   /*=====================================================*/
   /* Return a single field value from a multifield slot. */
   /*=====================================================*/

   if (hack->fromBeginning)
     { fieldPtr = &segmentPtr->contents[hack->beginOffset]; }
   else
     { fieldPtr = &segmentPtr->contents[segmentPtr->length - (hack->endOffset + 1)]; }

   returnValue->value = fieldPtr->value;

   return true;
  }

/****************************************************/
/* FactSlotLength: Determines if the length of a    */
/*  multifield slot falls within a specified range. */
/****************************************************/
bool FactSlotLength(
  Environment *theEnv,
  void *theValue,
  UDFValue *returnValue)
  {
   struct factCheckLengthPNCall *hack;
   Multifield *segmentPtr;
   size_t extraOffset = 0;
   struct multifieldMarker *tempMark;

   returnValue->value = FalseSymbol(theEnv);

   hack = (struct factCheckLengthPNCall *) ((CLIPSBitMap *) theValue)->contents;

   for (tempMark = FactData(theEnv)->CurrentPatternMarks;
        tempMark != NULL;
        tempMark = tempMark->next)
     {
      if (tempMark->where.whichSlotNumber != hack->whichSlot) continue;
      extraOffset += tempMark->range;
     }

   segmentPtr = FactData(theEnv)->CurrentPatternFact->theProposition.contents[hack->whichSlot].multifieldValue;

   if (segmentPtr->length < (hack->minLength + extraOffset))
     { return false; }

   if (hack->exactly && (segmentPtr->length > (hack->minLength + extraOffset)))
     { return false; }

   returnValue->value = TrueSymbol(theEnv);
   return true;
  }

/************************************************************/
/* FactJNCompVars1: Fact join network routine for comparing */
/*   the values of two single field slots.                  */
/************************************************************/
bool FactJNCompVars1(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
#if MAC_XCD
#pragma unused(theResult)
#endif
   unsigned short p1, e1, p2, e2;
   Fact *fact1, *fact2;
   struct factCompVarsJN1Call *hack;

   /*=========================================*/
   /* Retrieve the arguments to the function. */
   /*=========================================*/

   hack = (struct factCompVarsJN1Call *) ((CLIPSBitMap *) theValue)->contents;

   /*=================================================*/
   /* Extract the fact pointers for the two patterns. */
   /*=================================================*/

   p1 = hack->pattern1;
   p2 = hack->pattern2;

   fact1 = (Fact *) EngineData(theEnv)->GlobalRHSBinds->binds[p1].gm.theMatch->matchingItem;

   if (hack->p2rhs)
     { fact2 = (Fact *) EngineData(theEnv)->GlobalRHSBinds->binds[p2].gm.theMatch->matchingItem; }
   else
     { fact2 = (Fact *) EngineData(theEnv)->GlobalLHSBinds->binds[p2].gm.theMatch->matchingItem; }

   /*=====================*/
   /* Compare the values. */
   /*=====================*/

   e1 = hack->slot1;
   e2 = hack->slot2;

   if (fact1->theProposition.contents[e1].value !=
       fact2->theProposition.contents[e2].value)
     { return hack->fail; }

   return hack->pass;
  }

/*****************************************************************/
/* FactJNCompVars2:  Fact join network routine for comparing the */
/*   two single field value that are found in the first slot     */
/*   (which must also be a multifield slot) of a deftemplate.    */
/*   This function is provided so that variable comparisons of   */
/*   implied deftemplates will be faster.                        */
/*****************************************************************/
bool FactJNCompVars2(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
#if MAC_XCD
#pragma unused(theResult)
#endif
   unsigned short p1, s1, p2, s2;
   Fact *fact1, *fact2;
   struct factCompVarsJN2Call *hack;
   Multifield *segment;
   CLIPSValue *fieldPtr1, *fieldPtr2;

   /*=========================================*/
   /* Retrieve the arguments to the function. */
   /*=========================================*/

   hack = (struct factCompVarsJN2Call *) ((CLIPSBitMap *) theValue)->contents;

   /*=================================================*/
   /* Extract the fact pointers for the two patterns. */
   /*=================================================*/

   p1 = hack->pattern1;
   p2 = hack->pattern2;
   s1 = hack->slot1;
   s2 = hack->slot2;

   fact1 = (Fact *) EngineData(theEnv)->GlobalRHSBinds->binds[p1].gm.theMatch->matchingItem;

   if (hack->p2rhs)
     { fact2 = (Fact *) EngineData(theEnv)->GlobalRHSBinds->binds[p2].gm.theMatch->matchingItem; }
   else
     { fact2 = (Fact *) EngineData(theEnv)->GlobalLHSBinds->binds[p2].gm.theMatch->matchingItem; }

   /*======================*/
   /* Retrieve the values. */
   /*======================*/

   if (fact1->theProposition.contents[s1].header->type != MULTIFIELD_TYPE)
     { fieldPtr1 = &fact1->theProposition.contents[s1]; }
   else
     {
      segment = fact1->theProposition.contents[s1].multifieldValue;

      if (hack->fromBeginning1)
        { fieldPtr1 = &segment->contents[hack->offset1]; }
      else
        { fieldPtr1 = &segment->contents[segment->length - (hack->offset1 + 1)]; }
     }

   if (fact2->theProposition.contents[s2].header->type != MULTIFIELD_TYPE)
     { fieldPtr2 = &fact2->theProposition.contents[s2]; }
   else
     {
      segment = fact2->theProposition.contents[s2].multifieldValue;

      if (hack->fromBeginning2)
        { fieldPtr2 = &segment->contents[hack->offset2]; }
      else
        { fieldPtr2 = &segment->contents[segment->length - (hack->offset2 + 1)]; }
     }

   /*=====================*/
   /* Compare the values. */
   /*=====================*/

   if (fieldPtr1->value != fieldPtr2->value)
     { return hack->fail; }

   return hack->pass;
  }

/*****************************************************/
/* FactPNCompVars1: Fact pattern network routine for */
/*   comparing the values of two single field slots. */
/*****************************************************/
bool FactPNCompVars1(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   bool rv;
   CLIPSValue *fieldPtr1, *fieldPtr2;
   struct factCompVarsPN1Call *hack;

   /*========================================*/
   /* Extract the arguments to the function. */
   /*========================================*/

   hack = (struct factCompVarsPN1Call *) ((CLIPSBitMap *) theValue)->contents;
   fieldPtr1 = &FactData(theEnv)->CurrentPatternFact->theProposition.contents[hack->field1];
   fieldPtr2 = &FactData(theEnv)->CurrentPatternFact->theProposition.contents[hack->field2];

   /*=====================*/
   /* Compare the values. */
   /*=====================*/

   if (fieldPtr1->value != fieldPtr2->value) rv = (bool) hack->fail;
   else rv = (bool) hack->pass;

   if (rv) theResult->value = TrueSymbol(theEnv);
   else theResult->value = FalseSymbol(theEnv);

   return rv;
  }

/*************************************************************************/
/* AdjustFieldPosition: Given a list of multifield markers and the index */
/*   to a variable in a slot, this function computes the index to the    */
/*   field in the slot where the variable begins. In the case of         */
/*   multifield variables, it also computes the extent (or length) of    */
/*   the multifield. Note that the extent should be given a default      */
/*   value of either -1 or 1 for variables other than multifield         */
/*   variables before calling this routine.  An extent of -1 for these   */
/*   variables will distinguish their extent as being different when it  */
/*   is necessary to note their difference from a multifield variable    */
/*   with an extent of 1. For example, given the slot pattern            */
/*   (data $?x c $?y ?z) and the slot value (data a b c d e f x), the    */
/*   actual index in the fact for the 5th item in the pattern (the       */
/*   variable ?z) would be 8 since $?x binds to 2 fields and $?y binds   */
/*   to 3 fields.                                                        */
/*************************************************************************/
size_t AdjustFieldPosition(
  Environment *theEnv,
  struct multifieldMarker *markList,
  unsigned short whichField,
  unsigned short whichSlot,
  size_t *extent)
  {
   size_t actualIndex;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   actualIndex = whichField;
   for (;
        markList != NULL;
        markList = markList->next)
     {
      /*===============================================*/
      /* Skip over multifield markers for other slots. */
      /*===============================================*/

      if (markList->where.whichSlotNumber != whichSlot) continue;

      /*=========================================================*/
      /* If the multifield marker occurs exactly at the field in */
      /* question, then the actual index needs to be adjusted    */
      /* and the extent needs to be computed since the value is  */
      /* a multifield value.                                     */
      /*=========================================================*/

      if (markList->whichField == whichField)
        {
         *extent = markList->range;
         return actualIndex;
        }

      /*=====================================================*/
      /* Otherwise if the multifield marker occurs after the */
      /* field in question, then the actual index has been   */
      /* completely computed and can be returned.            */
      /*=====================================================*/

      else if (markList->whichField > whichField)
        { return actualIndex; }

      /*==========================================================*/
      /* Adjust the actual index to the field based on the number */
      /* of fields taken up by the preceding multifield variable. */
      /*==========================================================*/

      actualIndex = actualIndex + markList->range - 1;
     }

   /*=======================================*/
   /* Return the actual index to the field. */
   /*=======================================*/

   return actualIndex;
  }

/*****************************************************/
/* FactStoreMultifield: This primitive is used by a  */
/*   number of multifield functions for grouping a   */
/*   series of valuesinto a single multifield value. */
/*****************************************************/
bool FactStoreMultifield(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
#if MAC_XCD
#pragma unused(theValue)
#endif

   StoreInMultifield(theEnv,theResult,GetFirstArgument(),false);
   return true;
  }

#endif /* DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT */

