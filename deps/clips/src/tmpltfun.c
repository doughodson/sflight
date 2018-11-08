   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  09/20/17             */
   /*                                                     */
   /*             DEFTEMPLATE FUNCTIONS MODULE            */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements the modify and duplicate functions.   */
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
/*      6.24: Added deftemplate-slot-names,                  */
/*            deftemplate-slot-default-value,                */
/*            deftemplate-slot-cardinality,                  */
/*            deftemplate-slot-allowed-values,               */
/*            deftemplate-slot-range,                        */
/*            deftemplate-slot-types,                        */
/*            deftemplate-slot-multip,                       */
/*            deftemplate-slot-singlep,                      */
/*            deftemplate-slot-existp, and                   */
/*            deftemplate-slot-defaultp functions.           */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Support for deftemplate slot facets.           */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Added deftemplate-slot-facet-existp and        */
/*            deftemplate-slot-facet-value functions.        */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Used gensprintf instead of sprintf.            */
/*                                                           */
/*            Support for modify callback function.          */
/*                                                           */
/*            Added additional argument to function          */
/*            CheckDeftemplateAndSlotArguments to specify    */
/*            the expected number of arguments.              */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Added code to prevent a clear command from     */
/*            being executed during fact assertions via      */
/*            Increment/DecrementClearReadyLocks API.        */
/*                                                           */
/*      6.31: Error messages are now generated when modify   */
/*            and duplicate functions are given a retracted  */
/*            fact.                                          */
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
/*            Eval support for run time and bload only.      */
/*                                                           */
/*            Watch facts for modify command only prints     */
/*            changed slots.                                 */
/*                                                           */
/*            Modify command preserves fact id and address.  */
/*                                                           */
/*            Assert returns duplicate fact. FALSE is now    */
/*            returned only if an error occurs.              */
/*                                                           */
/*            For the modify command, specifying the fact    */
/*            using a fact-index is no longer limited to     */
/*            top-level commands.                            */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT

#include <stdio.h>
#include <string.h>

#include "argacces.h"
#include "commline.h"
#include "constant.h"
#include "cstrnchk.h"
#include "default.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "factmngr.h"
#include "factrhs.h"
#include "memalloc.h"
#include "modulutl.h"
#include "multifld.h"
#include "pprint.h"
#include "prcdrpsr.h"
#include "prntutil.h"
#include "reorder.h"
#include "router.h"
#include "scanner.h"
#include "symbol.h"
#include "sysdep.h"
#include "tmpltdef.h"
#include "tmpltlhs.h"
#include "tmpltrhs.h"
#include "tmpltutl.h"
#include "utility.h"

#include "tmpltfun.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static CLIPSLexeme            *CheckDeftemplateAndSlotArguments(UDFContext *,Deftemplate **);
   static void                    FreeTemplateValueArray(Environment *,CLIPSValue *,Deftemplate *);
   static struct expr            *ModAndDupParse(Environment *,struct expr *,const char *,const char *);
#if (! RUN_TIME) && (! BLOAD_ONLY)
   static CLIPSLexeme            *FindTemplateForFactAddress(CLIPSLexeme *,struct lhsParseNode *);
#endif

/****************************************************************/
/* DeftemplateFunctions: Initializes the deftemplate functions. */
/****************************************************************/
void DeftemplateFunctions(
  Environment *theEnv)
  {
#if ! RUN_TIME
   AddUDF(theEnv,"modify","bf",0,UNBOUNDED,"*;lf",ModifyCommand,"ModifyCommand",NULL);
   AddUDF(theEnv,"duplicate","bf",0,UNBOUNDED,"*;lf",DuplicateCommand,"DuplicateCommand",NULL);

   AddUDF(theEnv,"deftemplate-slot-names","bm",1,1,"y",DeftemplateSlotNamesFunction,"DeftemplateSlotNamesFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-default-value","*",2,2,"y",DeftemplateSlotDefaultValueFunction,"DeftemplateSlotDefaultValueFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-cardinality","*",2,2,"y",DeftemplateSlotCardinalityFunction,"DeftemplateSlotCardinalityFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-allowed-values","*",2,2,"y",DeftemplateSlotAllowedValuesFunction,"DeftemplateSlotAllowedValuesFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-range","*",2,2,"y",DeftemplateSlotRangeFunction,"DeftemplateSlotRangeFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-types","*",2,2,"y",DeftemplateSlotTypesFunction,"DeftemplateSlotTypesFunction",NULL);

   AddUDF(theEnv,"deftemplate-slot-multip","b",2,2,"y",DeftemplateSlotMultiPFunction,"DeftemplateSlotMultiPFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-singlep","b",2,2,"y",DeftemplateSlotSinglePFunction,"DeftemplateSlotSinglePFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-existp","b",2,2,"y",DeftemplateSlotExistPFunction,"DeftemplateSlotExistPFunction",NULL);
   AddUDF(theEnv,"deftemplate-slot-defaultp","y",2,2,"y",DeftemplateSlotDefaultPFunction,"DeftemplateSlotDefaultPFunction",NULL);

   AddUDF(theEnv,"deftemplate-slot-facet-existp","b",3,3,"y",DeftemplateSlotFacetExistPFunction,"DeftemplateSlotFacetExistPFunction",NULL);

   AddUDF(theEnv,"deftemplate-slot-facet-value","*",3,3,"y",DeftemplateSlotFacetValueFunction,"DeftemplateSlotFacetValueFunction",NULL);

   FuncSeqOvlFlags(theEnv,"modify",false,false);
   FuncSeqOvlFlags(theEnv,"duplicate",false,false);
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif

   AddFunctionParser(theEnv,"modify",ModifyParse);
   AddFunctionParser(theEnv,"duplicate",DuplicateParse);
  }

/***************************/
/* FreeTemplateValueArray: */
/***************************/
static void FreeTemplateValueArray(
  Environment *theEnv,
  CLIPSValue *theValueArray,
  Deftemplate *templatePtr)
  {
   unsigned short i;

   if (theValueArray == NULL) return;

   for (i = 0; i < templatePtr->numberOfSlots; i++)
     {
      if (theValueArray[i].header->type == MULTIFIELD_TYPE)
        { ReturnMultifield(theEnv,theValueArray[i].multifieldValue); }
     }

   rm(theEnv,theValueArray,sizeof(CLIPSValue) * templatePtr->numberOfSlots);
  }

/*************************************************************/
/* ModifyCommand: H/L access routine for the modify command. */
/*************************************************************/
void ModifyCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   long long factNum;
   Fact *oldFact;
   struct expr *testPtr;
   UDFValue computeResult;
   Deftemplate *templatePtr;
   struct templateSlot *slotPtr;
   size_t i;
   long long position;
   int replacementCount = 0;
   bool found;
   CLIPSValue *theValueArray;
   char *changeMap;

   /*===================================================*/
   /* Set the default return value to the symbol FALSE. */
   /*===================================================*/

   returnValue->lexemeValue = FalseSymbol(theEnv);

   /*==================================================*/
   /* Evaluate the first argument which is used to get */
   /* a pointer to the fact to be modified/duplicated. */
   /*==================================================*/

   testPtr = GetFirstArgument();
   IncrementClearReadyLocks(theEnv);
   EvaluateExpression(theEnv,testPtr,&computeResult);
   DecrementClearReadyLocks(theEnv);

   /*==============================================================*/
   /* If an integer is supplied, then treat it as a fact-index and */
   /* search the fact-list for the fact with that fact-index.      */
   /*==============================================================*/

   if (computeResult.header->type == INTEGER_TYPE)
     {
      factNum = computeResult.integerValue->contents;
      if (factNum < 0)
        {
         ExpectedTypeError2(theEnv,"modify",1);
         SetEvaluationError(theEnv,true);
         return;
        }

      oldFact = GetNextFact(theEnv,NULL);
      while (oldFact != NULL)
        {
         if (oldFact->factIndex == factNum)
           { break; }
         else
           { oldFact = oldFact->nextFact; }
        }

      if (oldFact == NULL)
        {
         char tempBuffer[20];
         gensprintf(tempBuffer,"f-%lld",factNum);
         CantFindItemErrorMessage(theEnv,"fact",tempBuffer,false);
         return;
        }
     }

   /*==========================================*/
   /* Otherwise, if a pointer is supplied then */
   /* no lookup is required.                   */
   /*==========================================*/

   else if (computeResult.header->type == FACT_ADDRESS_TYPE)
     { oldFact = computeResult.factValue; }

   /*===========================================*/
   /* Otherwise, the first argument is invalid. */
   /*===========================================*/

   else
     {
      ExpectedTypeError2(theEnv,"modify",1);
      SetEvaluationError(theEnv,true);
      return;
     }

   /*=====================================*/
   /* Retracted facts cannot be modified. */
   /*=====================================*/
   
   if (oldFact->garbage)
     {
      FactRetractedErrorMessage(theEnv,oldFact);
      return;
     }
     
   /*==================================*/
   /* See if it is a deftemplate fact. */
   /*==================================*/

   templatePtr = oldFact->whichDeftemplate;

   if (templatePtr->implied) return;

   /*========================================================*/
   /* Create a data object array to hold the updated values. */
   /*========================================================*/
   
   if (templatePtr->numberOfSlots == 0)
     {
      theValueArray = NULL;
      changeMap = NULL;
     }
   else
     {
      theValueArray = (CLIPSValue *) gm2(theEnv,sizeof(void *) * templatePtr->numberOfSlots);
      changeMap = (char *) gm2(theEnv,CountToBitMapSize(templatePtr->numberOfSlots));
      ClearBitString((void *) changeMap,CountToBitMapSize(templatePtr->numberOfSlots));
     }

   /*================================================================*/
   /* Duplicate the values from the old fact (skipping multifields). */
   /*================================================================*/

   for (i = 0; i < oldFact->theProposition.length; i++)
     { theValueArray[i].voidValue = VoidConstant(theEnv); }

   /*========================*/
   /* Start replacing slots. */
   /*========================*/

   testPtr = testPtr->nextArg;
   while (testPtr != NULL)
     {
      /*============================================================*/
      /* If the slot identifier is an integer, then the slot was    */
      /* previously identified and its position within the template */
      /* was stored. Otherwise, the position of the slot within the */
      /* deftemplate has to be determined by comparing the name of  */
      /* the slot against the list of slots for the deftemplate.    */
      /*============================================================*/

      if (testPtr->type == INTEGER_TYPE)
        { position = testPtr->integerValue->contents; }
      else
        {
         found = false;
         position = 0;
         slotPtr = templatePtr->slotList;
         while (slotPtr != NULL)
           {
            if (slotPtr->slotName == testPtr->lexemeValue)
              {
               found = true;
               slotPtr = NULL;
              }
            else
              {
               slotPtr = slotPtr->next;
               position++;
              }
           }

         if (! found)
           {
            InvalidDeftemplateSlotMessage(theEnv,testPtr->lexemeValue->contents,
                                          templatePtr->header.name->contents,true);
            SetEvaluationError(theEnv,true);
            FreeTemplateValueArray(theEnv,theValueArray,templatePtr);
            if (changeMap != NULL)
              { rm(theEnv,(void *) changeMap,CountToBitMapSize(templatePtr->numberOfSlots)); }
            return;
           }
        }

      /*===================================================*/
      /* If a single field slot is being replaced, then... */
      /*===================================================*/

      if (oldFact->theProposition.contents[position].header->type != MULTIFIELD_TYPE)
        {
         /*======================================================*/
         /* If the list of values to store in the slot is empty  */
         /* or contains more than one member than an error has   */
         /* occured because a single field slot can only contain */
         /* a single value.                                      */
         /*======================================================*/

         if ((testPtr->argList == NULL) ? true : (testPtr->argList->nextArg != NULL))
           {
            MultiIntoSingleFieldSlotError(theEnv,GetNthSlot(templatePtr,position),templatePtr);
            FreeTemplateValueArray(theEnv,theValueArray,templatePtr);
            if (changeMap != NULL)
              { rm(theEnv,(void *) changeMap,CountToBitMapSize(templatePtr->numberOfSlots)); }
            return;
           }

         /*===================================================*/
         /* Evaluate the expression to be stored in the slot. */
         /*===================================================*/

         IncrementClearReadyLocks(theEnv);
         EvaluateExpression(theEnv,testPtr->argList,&computeResult);
         SetEvaluationError(theEnv,false);
         DecrementClearReadyLocks(theEnv);

         /*====================================================*/
         /* If the expression evaluated to a multifield value, */
         /* then an error occured since a multifield value can */
         /* not be stored in a single field slot.              */
         /*====================================================*/

         if (computeResult.header->type == MULTIFIELD_TYPE)
           {
            MultiIntoSingleFieldSlotError(theEnv,GetNthSlot(templatePtr,position),templatePtr);
            FreeTemplateValueArray(theEnv,theValueArray,templatePtr);
            if (changeMap != NULL)
              { rm(theEnv,(void *) changeMap,CountToBitMapSize(templatePtr->numberOfSlots)); }
            return;
           }

         /*=============================*/
         /* Store the value in the slot */
         /*=============================*/

         if (oldFact->theProposition.contents[position].value != computeResult.value)
           {
            replacementCount++;
            theValueArray[position].value = computeResult.value;
            if (changeMap != NULL)
              { SetBitMap(changeMap,position); }
           }
        }

      /*=================================*/
      /* Else replace a multifield slot. */
      /*=================================*/

      else
        {
         /*======================================*/
         /* Determine the new value of the slot. */
         /*======================================*/

         IncrementClearReadyLocks(theEnv);
         StoreInMultifield(theEnv,&computeResult,testPtr->argList,false);
         SetEvaluationError(theEnv,false);
         DecrementClearReadyLocks(theEnv);

         /*=============================*/
         /* Store the value in the slot */
         /*=============================*/

         if ((oldFact->theProposition.contents[position].header->type != computeResult.header->type) ||
             (! MultifieldsEqual((Multifield *) oldFact->theProposition.contents[position].value,(Multifield *) computeResult.value)))
           {
            theValueArray[position].value = computeResult.value;
            replacementCount++;
            if (changeMap != NULL)
              { SetBitMap(changeMap,position); }
           }
         else
           { ReturnMultifield(theEnv,computeResult.multifieldValue); }
        }

      testPtr = testPtr->nextArg;
     }

   /*==================================*/
   /* If no slots have changed, then a */
   /* retract/assert is not performed. */
   /*==================================*/

   if (replacementCount == 0)
     {
      if (theValueArray != NULL)
        { rm(theEnv,theValueArray,sizeof(void *) * templatePtr->numberOfSlots); }
      if (changeMap != NULL)
        { rm(theEnv,(void *) changeMap,CountToBitMapSize(templatePtr->numberOfSlots)); }
      
      returnValue->value = oldFact;
      return;
     }
    
   /*=========================================*/
   /* Replace the old values with the values. */
   /*=========================================*/
   
   if ((oldFact = ReplaceFact(theEnv,oldFact,theValueArray,changeMap)) != NULL)
     { returnValue->factValue = oldFact; }

   /*=============================*/
   /* Free the data object array. */
   /*=============================*/

   if (theValueArray != NULL)
     { rm(theEnv,theValueArray,sizeof(void *) * templatePtr->numberOfSlots); }

   if (changeMap != NULL)
     { rm(theEnv,(void *) changeMap,CountToBitMapSize(templatePtr->numberOfSlots)); }

   return;
  }

/****************/
/* ReplaceFact: */
/****************/
Fact *ReplaceFact(
  Environment *theEnv,
  Fact *oldFact,
  CLIPSValue *theValueArray,
  char *changeMap)
  {
   size_t i;
   Fact *theFact;
   Fact *factListPosition, *templatePosition;
   
   /*===============================================*/
   /* Call registered modify notification functions */
   /* for the existing version of the fact.         */
   /*===============================================*/

   if (FactData(theEnv)->ListOfModifyFunctions != NULL)
     {
      ModifyCallFunctionItem *theModifyFunction;
      
      for (theModifyFunction = FactData(theEnv)->ListOfModifyFunctions;
           theModifyFunction != NULL;
           theModifyFunction = theModifyFunction->next)
        {
         (*theModifyFunction->func)(theEnv,oldFact,NULL,theModifyFunction->context);
        }
     }

   /*==========================================*/
   /* Remember the position of the fact before */
   /* it is retracted so this can be restored  */
   /* when the modified fact is asserted.      */
   /*==========================================*/

   factListPosition = oldFact->previousFact;
   templatePosition = oldFact->previousTemplateFact;
   
   /*===================*/
   /* Retract the fact. */
   /*===================*/

   RetractDriver(theEnv,oldFact,true,changeMap);
   oldFact->garbage = false;

   /*======================================*/
   /* Copy the new values to the old fact. */
   /*======================================*/

   for (i = 0; i < oldFact->theProposition.length; i++)
     {
      if (theValueArray[i].voidValue != VoidConstant(theEnv))
        {
         AtomDeinstall(theEnv,oldFact->theProposition.contents[i].header->type,oldFact->theProposition.contents[i].value);
         
         if (oldFact->theProposition.contents[i].header->type == MULTIFIELD_TYPE)
           {
            Multifield *theSegment = oldFact->theProposition.contents[i].multifieldValue;
            if (theSegment->busyCount == 0)
              { ReturnMultifield(theEnv,theSegment); }
            else
              { AddToMultifieldList(theEnv,theSegment); }
           }

         oldFact->theProposition.contents[i].value = theValueArray[i].value;
         
         AtomInstall(theEnv,oldFact->theProposition.contents[i].header->type,oldFact->theProposition.contents[i].value);
        }
     }

   /*======================*/
   /* Assert the new fact. */
   /*======================*/

   theFact = AssertDriver(oldFact,oldFact->factIndex,factListPosition,templatePosition,changeMap);

   /*===============================================*/
   /* Call registered modify notification functions */
   /* for the new version of the fact.              */
   /*===============================================*/

   if (FactData(theEnv)->ListOfModifyFunctions != NULL)
     {
      ModifyCallFunctionItem *theModifyFunction;

      for (theModifyFunction = FactData(theEnv)->ListOfModifyFunctions;
           theModifyFunction != NULL;
           theModifyFunction = theModifyFunction->next)
        {
         (*theModifyFunction->func)(theEnv,NULL,theFact,theModifyFunction->context);
        }
     }
     
   return theFact;
  }

/*******************************************************************/
/* DuplicateCommand: H/L access routine for the duplicate command. */
/*******************************************************************/
void DuplicateCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   long long factNum;
   Fact *oldFact, *newFact, *theFact;
   struct expr *testPtr;
   UDFValue computeResult;
   Deftemplate *templatePtr;
   struct templateSlot *slotPtr;
   size_t i;
   long long position;
   bool found;

   /*===================================================*/
   /* Set the default return value to the symbol FALSE. */
   /*===================================================*/

   returnValue->lexemeValue = FalseSymbol(theEnv);

   /*==================================================*/
   /* Evaluate the first argument which is used to get */
   /* a pointer to the fact to be modified/duplicated. */
   /*==================================================*/

   testPtr = GetFirstArgument();
   IncrementClearReadyLocks(theEnv);
   EvaluateExpression(theEnv,testPtr,&computeResult);
   DecrementClearReadyLocks(theEnv);

   /*==============================================================*/
   /* If an integer is supplied, then treat it as a fact-index and */
   /* search the fact-list for the fact with that fact-index.      */
   /*==============================================================*/

   if (computeResult.header->type == INTEGER_TYPE)
     {
      factNum = computeResult.integerValue->contents;
      if (factNum < 0)
        {
         ExpectedTypeError2(theEnv,"duplicate",1);
         SetEvaluationError(theEnv,true);
         return;
        }

      oldFact = GetNextFact(theEnv,NULL);
      while (oldFact != NULL)
        {
         if (oldFact->factIndex == factNum)
           { break; }
         else
           { oldFact = oldFact->nextFact; }
        }

      if (oldFact == NULL)
        {
         char tempBuffer[20];
         gensprintf(tempBuffer,"f-%lld",factNum);
         CantFindItemErrorMessage(theEnv,"fact",tempBuffer,false);
         return;
        }
     }

   /*==========================================*/
   /* Otherwise, if a pointer is supplied then */
   /* no lookup is required.                   */
   /*==========================================*/

   else if (computeResult.header->type == FACT_ADDRESS_TYPE)
     { oldFact = computeResult.factValue; }

   /*===========================================*/
   /* Otherwise, the first argument is invalid. */
   /*===========================================*/

   else
     {
      ExpectedTypeError2(theEnv,"duplicate",1);
      SetEvaluationError(theEnv,true);
      return;
     }

   /*=======================================*/
   /* Retracted facts cannot be duplicated. */
   /*=======================================*/
   
   if (oldFact->garbage)
     {
      FactRetractedErrorMessage(theEnv,oldFact);
      return;
     }

   /*==================================*/
   /* See if it is a deftemplate fact. */
   /*==================================*/

   templatePtr = oldFact->whichDeftemplate;

   if (templatePtr->implied) return;

   /*================================================================*/
   /* Duplicate the values from the old fact (skipping multifields). */
   /*================================================================*/

   newFact = CreateFactBySize(theEnv,oldFact->theProposition.length);
   newFact->whichDeftemplate = templatePtr;
   for (i = 0; i < oldFact->theProposition.length; i++)
     {
      if (oldFact->theProposition.contents[i].header->type != MULTIFIELD_TYPE)
        { newFact->theProposition.contents[i].value = oldFact->theProposition.contents[i].value; }
      else
        { newFact->theProposition.contents[i].value = NULL; }
     }

   /*========================*/
   /* Start replacing slots. */
   /*========================*/

   testPtr = testPtr->nextArg;
   while (testPtr != NULL)
     {
      /*============================================================*/
      /* If the slot identifier is an integer, then the slot was    */
      /* previously identified and its position within the template */
      /* was stored. Otherwise, the position of the slot within the */
      /* deftemplate has to be determined by comparing the name of  */
      /* the slot against the list of slots for the deftemplate.    */
      /*============================================================*/

      if (testPtr->type == INTEGER_TYPE)
        { position = testPtr->integerValue->contents; }
      else
        {
         found = false;
         position = 0;
         slotPtr = templatePtr->slotList;
         while (slotPtr != NULL)
           {
            if (slotPtr->slotName == testPtr->lexemeValue)
              {
               found = true;
               slotPtr = NULL;
              }
            else
              {
               slotPtr = slotPtr->next;
               position++;
              }
           }

         if (! found)
           {
            InvalidDeftemplateSlotMessage(theEnv,testPtr->lexemeValue->contents,
                                          templatePtr->header.name->contents,true);
            SetEvaluationError(theEnv,true);
            ReturnFact(theEnv,newFact);
            return;
           }
        }

      /*===================================================*/
      /* If a single field slot is being replaced, then... */
      /*===================================================*/

      if (newFact->theProposition.contents[position].value != NULL)
        {
         /*======================================================*/
         /* If the list of values to store in the slot is empty  */
         /* or contains more than one member than an error has   */
         /* occured because a single field slot can only contain */
         /* a single value.                                      */
         /*======================================================*/

         if ((testPtr->argList == NULL) ? true : (testPtr->argList->nextArg != NULL))
           {
            MultiIntoSingleFieldSlotError(theEnv,GetNthSlot(templatePtr,position),templatePtr);
            ReturnFact(theEnv,newFact);
            return;
           }

         /*===================================================*/
         /* Evaluate the expression to be stored in the slot. */
         /*===================================================*/

         IncrementClearReadyLocks(theEnv);
         EvaluateExpression(theEnv,testPtr->argList,&computeResult);
         SetEvaluationError(theEnv,false);
         DecrementClearReadyLocks(theEnv);

         /*====================================================*/
         /* If the expression evaluated to a multifield value, */
         /* then an error occured since a multifield value can */
         /* not be stored in a single field slot.              */
         /*====================================================*/

         if (computeResult.header->type == MULTIFIELD_TYPE)
           {
            ReturnFact(theEnv,newFact);
            MultiIntoSingleFieldSlotError(theEnv,GetNthSlot(templatePtr,position),templatePtr);
            return;
           }

         /*=============================*/
         /* Store the value in the slot */
         /*=============================*/

         newFact->theProposition.contents[position].value = computeResult.value;
        }

      /*=================================*/
      /* Else replace a multifield slot. */
      /*=================================*/

      else
        {
         /*======================================*/
         /* Determine the new value of the slot. */
         /*======================================*/

         IncrementClearReadyLocks(theEnv);
         StoreInMultifield(theEnv,&computeResult,testPtr->argList,false);
         SetEvaluationError(theEnv,false);
         DecrementClearReadyLocks(theEnv);

         /*=============================*/
         /* Store the value in the slot */
         /*=============================*/

         newFact->theProposition.contents[position].value = computeResult.value;
        }

      testPtr = testPtr->nextArg;
     }

   /*=====================================*/
   /* Copy the multifield values from the */
   /* old fact that were not replaced.    */
   /*=====================================*/

   for (i = 0; i < oldFact->theProposition.length; i++)
     {
      if ((oldFact->theProposition.contents[i].header->type == MULTIFIELD_TYPE) &&
          (newFact->theProposition.contents[i].value == NULL))

        {
         newFact->theProposition.contents[i].value =
            CopyMultifield(theEnv,oldFact->theProposition.contents[i].multifieldValue);
        }
     }

   /*===============================*/
   /* Perform the duplicate action. */
   /*===============================*/

   theFact = AssertDriver(newFact,0,NULL,NULL,NULL);

   /*========================================*/
   /* The asserted fact is the return value. */
   /*========================================*/

   if (theFact != NULL)
     {
      returnValue->begin = 0;
      returnValue->range = theFact->theProposition.length;
      returnValue->value = theFact;
     }

   return;
  }

/****************************************************/
/* DeftemplateSlotNamesFunction: H/L access routine */
/*   for the deftemplate-slot-names function.       */
/****************************************************/
void DeftemplateSlotNamesFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *deftemplateName;
   Deftemplate *theDeftemplate;
   CLIPSValue cv;

   /*=============================================*/
   /* Set up the default return value for errors. */
   /*=============================================*/

   returnValue->value = FalseSymbol(theEnv);

   /*=======================================*/
   /* Get the reference to the deftemplate. */
   /*=======================================*/

   deftemplateName = GetConstructName(context,"deftemplate-slot-names","deftemplate name");
   if (deftemplateName == NULL) return;

   theDeftemplate = FindDeftemplate(theEnv,deftemplateName);
   if (theDeftemplate == NULL)
     {
      CantFindItemErrorMessage(theEnv,"deftemplate",deftemplateName,true);
      return;
     }

   /*=====================*/
   /* Get the slot names. */
   /*=====================*/

   DeftemplateSlotNames(theDeftemplate,&cv);
   CLIPSToUDFValue(&cv,returnValue);
  }

/**********************************************/
/* DeftemplateSlotNames: C access routine for */
/*   the deftemplate-slot-names function.     */
/**********************************************/
void DeftemplateSlotNames(
  Deftemplate *theDeftemplate,
  CLIPSValue *returnValue)
  {
   Multifield *theList;
   struct templateSlot *theSlot;
   unsigned long count;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      theList = CreateMultifield(theEnv,1);
      theList->contents[0].lexemeValue = CreateSymbol(theEnv,"implied");
      returnValue->value = theList;
      return;
     }

   /*=================================*/
   /* Count the number of slot names. */
   /*=================================*/

   for (count = 0, theSlot = theDeftemplate->slotList;
        theSlot != NULL;
        count++, theSlot = theSlot->next)
     { /* Do Nothing */ }

   /*=============================================================*/
   /* Create a multifield value in which to store the slot names. */
   /*=============================================================*/

   theList = CreateMultifield(theEnv,count);
   returnValue->value = theList;

   /*===============================================*/
   /* Store the slot names in the multifield value. */
   /*===============================================*/

   for (count = 0, theSlot = theDeftemplate->slotList;
        theSlot != NULL;
        count++, theSlot = theSlot->next)
     {
      theList->contents[count].lexemeValue = theSlot->slotName;
     }
  }

/*******************************************************/
/* DeftemplateSlotDefaultPFunction: H/L access routine */
/*   for the deftemplate-slot-defaultp function.       */
/*******************************************************/
void DeftemplateSlotDefaultPFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   int defaultType;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*===============================*/
   /* Does the slot have a default? */
   /*===============================*/

   defaultType = DeftemplateSlotDefaultP(theDeftemplate,slotName->contents);

   if (defaultType == STATIC_DEFAULT)
     { returnValue->lexemeValue = CreateSymbol(theEnv,"static"); }
   else if (defaultType == DYNAMIC_DEFAULT)
     { returnValue->lexemeValue = CreateSymbol(theEnv,"dynamic"); }
   else
     { returnValue->lexemeValue = FalseSymbol(theEnv); }
  }

/*************************************************/
/* DeftemplateSlotDefaultP: C access routine for */
/*   the deftemplate-slot-defaultp function.     */
/*************************************************/
DefaultType DeftemplateSlotDefaultP(
  Deftemplate *theDeftemplate,
  const char *slotName)
  {
   struct templateSlot *theSlot;
   Environment *theEnv = theDeftemplate->header.env;

   /*==================================================*/
   /* Make sure the slot exists (the symbol implied is */
   /* used for the implied slot of an ordered fact).   */
   /*==================================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        {
         return STATIC_DEFAULT;
        }
      else
        {
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return NO_DEFAULT;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return NO_DEFAULT;
     }

   /*======================================*/
   /* Return the default type of the slot. */
   /*======================================*/

   if (theSlot->noDefault)
     { return NO_DEFAULT; }
   else if (theSlot->defaultDynamic)
     { return DYNAMIC_DEFAULT; }

   return STATIC_DEFAULT;
  }

/*************************************************************/
/* DeftemplateSlotDefaultValueFunction: H/L access routine   */
/*   for the deftemplate-slot-default-value function.        */
/*************************************************************/
void DeftemplateSlotDefaultValueFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   CLIPSValue cv;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*=========================================*/
   /* Get the deftemplate slot default value. */
   /*=========================================*/

   DeftemplateSlotDefaultValue(theDeftemplate,slotName->contents,&cv);
   CLIPSToUDFValue(&cv,returnValue);
  }

/******************************************************/
/* DeftemplateSlotDefaultValue: C access routine for  */
/*   the deftemplate-slot-default-value function.     */
/******************************************************/
bool DeftemplateSlotDefaultValue(
  Deftemplate *theDeftemplate,
  const char *slotName,
  CLIPSValue *theValue)
  {
   struct templateSlot *theSlot;
   UDFValue tempDO;
   Environment *theEnv = theDeftemplate->header.env;

   /*=============================================*/
   /* Set up the default return value for errors. */
   /*=============================================*/

   theValue->value = FalseSymbol(theEnv);

   /*==================================================*/
   /* Make sure the slot exists (the symbol implied is */
   /* used for the implied slot of an ordered fact).   */
   /*==================================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        {
         theValue->value = CreateMultifield(theEnv,0L);
         return true;
        }
      else
        {
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*=======================================*/
   /* Return the default value of the slot. */
   /*=======================================*/

   if (theSlot->noDefault)
     { theValue->value = CreateSymbol(theEnv,"?NONE"); }
   else if (DeftemplateSlotDefault(theEnv,theDeftemplate,theSlot,&tempDO,true))
     {
      NormalizeMultifield(theEnv,&tempDO);
      theValue->value = tempDO.value;
     }
   else
     { return false; }

   return true;
  }

/**********************************************************/
/* DeftemplateSlotCardinalityFunction: H/L access routine */
/*   for the deftemplate-slot-cardinality function.       */
/**********************************************************/
void DeftemplateSlotCardinalityFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   CLIPSValue cv;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }

   /*=======================================*/
   /* Get the deftemplate slot cardinality. */
   /*=======================================*/

   DeftemplateSlotCardinality(theDeftemplate,slotName->contents,&cv);
   CLIPSToUDFValue(&cv,returnValue);
  }

/****************************************************/
/* DeftemplateSlotCardinality: C access routine for */
/*   the deftemplate-slot-cardinality function.     */
/****************************************************/
bool DeftemplateSlotCardinality(
  Deftemplate *theDeftemplate,
  const char *slotName,
  CLIPSValue *returnValue)
  {
   struct templateSlot *theSlot;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        {
         returnValue->value = CreateMultifield(theEnv,2L);
         returnValue->multifieldValue->contents[0].integerValue = SymbolData(theEnv)->Zero;
         returnValue->multifieldValue->contents[1].lexemeValue = SymbolData(theEnv)->PositiveInfinity;
         return true;
        }
      else
        {
         returnValue->multifieldValue = CreateMultifield(theEnv,0L);
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      returnValue->multifieldValue = CreateMultifield(theEnv,0L);
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*=====================================*/
   /* Return the cardinality of the slot. */
   /*=====================================*/

   if (theSlot->multislot == 0)
     {
      returnValue->multifieldValue = CreateMultifield(theEnv,0L);
      return true;
     }

   returnValue->value = CreateMultifield(theEnv,2L);

   if (theSlot->constraints != NULL)
     {
      returnValue->multifieldValue->contents[0].value = theSlot->constraints->minFields->value;
      returnValue->multifieldValue->contents[1].value = theSlot->constraints->maxFields->value;
     }
   else
     {
      returnValue->multifieldValue->contents[0].integerValue = SymbolData(theEnv)->Zero;
      returnValue->multifieldValue->contents[1].lexemeValue = SymbolData(theEnv)->PositiveInfinity;
     }
     
   return true;
  }

/************************************************************/
/* DeftemplateSlotAllowedValuesFunction: H/L access routine */
/*   for the deftemplate-slot-allowed-values function.      */
/************************************************************/
void DeftemplateSlotAllowedValuesFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   CLIPSValue result;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }

   /*==========================================*/
   /* Get the deftemplate slot allowed values. */
   /*==========================================*/

   DeftemplateSlotAllowedValues(theDeftemplate,slotName->contents,&result);
   CLIPSToUDFValue(&result,returnValue);
  }

/*******************************************************/
/* DeftemplateSlotAllowedValues: C access routine      */
/*   for the deftemplate-slot-allowed-values function. */
/*******************************************************/
bool DeftemplateSlotAllowedValues(
  Deftemplate *theDeftemplate,
  const char *slotName,
  CLIPSValue *returnValue)
  {
   struct templateSlot *theSlot;
   int i;
   Expression *theExp;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        {
         returnValue->value = FalseSymbol(theEnv);
         return true;
        }
      else
        {
         returnValue->multifieldValue = CreateMultifield(theEnv,0L);
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      returnValue->multifieldValue = CreateMultifield(theEnv,0L);
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*========================================*/
   /* Return the allowed values of the slot. */
   /*========================================*/

   if ((theSlot->constraints != NULL) ? (theSlot->constraints->restrictionList == NULL) : true)
     {
      returnValue->value = FalseSymbol(theEnv);
      return true;
     }

   returnValue->value = CreateMultifield(theEnv,ExpressionSize(theSlot->constraints->restrictionList));
   i = 0;

   theExp = theSlot->constraints->restrictionList;
   while (theExp != NULL)
     {
      returnValue->multifieldValue->contents[i].value = theExp->value;
      theExp = theExp->nextArg;
      i++;
     }
     
   return true;
  }

/****************************************************/
/* DeftemplateSlotRangeFunction: H/L access routine */
/*   for the deftemplate-slot-range function.       */
/****************************************************/
void DeftemplateSlotRangeFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   CLIPSValue cv;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }

   /*=================================*/
   /* Get the deftemplate slot range. */
   /*=================================*/

   DeftemplateSlotRange(theDeftemplate,slotName->contents,&cv);
   CLIPSToUDFValue(&cv,returnValue);
  }

/**********************************************/
/* DeftemplateSlotRange: C access routine for */
/*   the deftemplate-slot-range function.     */
/**********************************************/
bool DeftemplateSlotRange(
  Deftemplate *theDeftemplate,
  const char *slotName,
  CLIPSValue *returnValue)
  {
   struct templateSlot *theSlot;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        {
         returnValue->value = CreateMultifield(theEnv,2L);
         returnValue->multifieldValue->contents[0].lexemeValue =
            SymbolData(theEnv)->NegativeInfinity;
         returnValue->multifieldValue->contents[1].lexemeValue =
            SymbolData(theEnv)->PositiveInfinity;
         return true;
        }
      else
        {
         returnValue->multifieldValue = CreateMultifield(theEnv,0L);
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      returnValue->multifieldValue = CreateMultifield(theEnv,0L);
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*===============================*/
   /* Return the range of the slot. */
   /*===============================*/

   if ((theSlot->constraints == NULL) ? false :
       (theSlot->constraints->anyAllowed || theSlot->constraints->floatsAllowed ||
        theSlot->constraints->integersAllowed))
     {
      returnValue->value = CreateMultifield(theEnv,2L);
      returnValue->multifieldValue->contents[0].value = theSlot->constraints->minValue->value;
      returnValue->multifieldValue->contents[1].value = theSlot->constraints->maxValue->value;
     }
   else
     { returnValue->value = FalseSymbol(theEnv); }
     
   return true;
  }

/****************************************************/
/* DeftemplateSlotTypesFunction: H/L access routine */
/*   for the deftemplate-slot-types function.       */
/****************************************************/
void DeftemplateSlotTypesFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   CLIPSValue cv;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }

   /*=================================*/
   /* Get the deftemplate slot types. */
   /*=================================*/

   DeftemplateSlotTypes(theDeftemplate,slotName->contents,&cv);
   CLIPSToUDFValue(&cv,returnValue);
  }

/**********************************************/
/* DeftemplateSlotTypes: C access routine for */
/*   the deftemplate-slot-types function.     */
/**********************************************/
bool DeftemplateSlotTypes(
  Deftemplate *theDeftemplate,
  const char *slotName,
  CLIPSValue *returnValue)
  {
   struct templateSlot *theSlot = NULL;
   unsigned int numTypes, i;
   bool allTypes = false;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot name is "implied."         */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") != 0)
        {
         returnValue->multifieldValue = CreateMultifield(theEnv,0L);
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      returnValue->multifieldValue = CreateMultifield(theEnv,0L);
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*==============================================*/
   /* If the slot has no constraint information or */
   /* there is no type restriction, then all types */
   /* are allowed for the slot.                    */
   /*==============================================*/

   if ((theDeftemplate->implied) ||
       ((theSlot->constraints != NULL) ? theSlot->constraints->anyAllowed : true))
     {
#if OBJECT_SYSTEM
      numTypes = 8;
#else
      numTypes = 6;
#endif
      allTypes = true;
     }

   /*==============================================*/
   /* Otherwise count the number of types allowed. */
   /*==============================================*/

   else
     {
      numTypes = theSlot->constraints->symbolsAllowed +
                 theSlot->constraints->stringsAllowed +
                 theSlot->constraints->floatsAllowed +
                 theSlot->constraints->integersAllowed +
                 theSlot->constraints->instanceNamesAllowed +
                 theSlot->constraints->instanceAddressesAllowed +
                 theSlot->constraints->externalAddressesAllowed +
                 theSlot->constraints->factAddressesAllowed;
     }

   /*========================================*/
   /* Return the allowed types for the slot. */
   /*========================================*/

   returnValue->value = CreateMultifield(theEnv,numTypes);

   i = 0;

   if (allTypes || theSlot->constraints->floatsAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"FLOAT");
     }

   if (allTypes || theSlot->constraints->integersAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"INTEGER");
     }

   if (allTypes || theSlot->constraints->symbolsAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"SYMBOL");
     }

   if (allTypes || theSlot->constraints->stringsAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"STRING");
     }

   if (allTypes || theSlot->constraints->externalAddressesAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"EXTERNAL-ADDRESS");
     }

   if (allTypes || theSlot->constraints->factAddressesAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"FACT-ADDRESS");
     }

#if OBJECT_SYSTEM
   if (allTypes || theSlot->constraints->instanceAddressesAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"INSTANCE-ADDRESS");
     }

   if (allTypes || theSlot->constraints->instanceNamesAllowed)
     {
      returnValue->multifieldValue->contents[i++].lexemeValue = CreateSymbol(theEnv,"INSTANCE-NAME");
     }
#endif

   return true;
  }

/*****************************************************/
/* DeftemplateSlotMultiPFunction: H/L access routine */
/*   for the deftemplate-slot-multip function.       */
/*****************************************************/
void DeftemplateSlotMultiPFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*================================*/
   /* Is the slot a multifield slot? */
   /*================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,DeftemplateSlotMultiP(theDeftemplate,slotName->contents));
  }

/***********************************************/
/* DeftemplateSlotMultiP: C access routine for */
/*   the deftemplate-slot-multip function.     */
/***********************************************/
bool DeftemplateSlotMultiP(
  Deftemplate *theDeftemplate,
  const char *slotName)
  {
   struct templateSlot *theSlot;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        { return true; }
      else
        {
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*================================*/
   /* Is the slot a multifield slot? */
   /*================================*/

   return(theSlot->multislot);
  }

/******************************************************/
/* DeftemplateSlotSinglePFunction: H/L access routine */
/*   for the deftemplate-slot-singlep function.       */
/******************************************************/
void DeftemplateSlotSinglePFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*==================================*/
   /* Is the slot a single field slot? */
   /*==================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,DeftemplateSlotSingleP(theDeftemplate,slotName->contents));
  }

/************************************************/
/* DeftemplateSlotSingleP: C access routine for */
/*   the deftemplate-slot-singlep function.     */
/************************************************/
bool DeftemplateSlotSingleP(
  Deftemplate *theDeftemplate,
  const char *slotName)
  {
   struct templateSlot *theSlot;
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        { return false; }
      else
        {
         SetEvaluationError(theEnv,true);
         InvalidDeftemplateSlotMessage(theEnv,slotName,
                                       theDeftemplate->header.name->contents,false);
         return false;
        }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     {
      SetEvaluationError(theEnv,true);
      InvalidDeftemplateSlotMessage(theEnv,slotName,
                                    theDeftemplate->header.name->contents,false);
      return false;
     }

   /*==================================*/
   /* Is the slot a single field slot? */
   /*==================================*/

   return(! theSlot->multislot);
  }

/*****************************************************/
/* DeftemplateSlotExistPFunction: H/L access routine */
/*   for the deftemplate-slot-existp function.       */
/*****************************************************/
void DeftemplateSlotExistPFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*======================*/
   /* Does the slot exist? */
   /*======================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,DeftemplateSlotExistP(theDeftemplate,slotName->contents));
  }

/***********************************************/
/* DeftemplateSlotExistP: C access routine for */
/*   the deftemplate-slot-existp function.     */
/***********************************************/
bool DeftemplateSlotExistP(
  Deftemplate *theDeftemplate,
  const char *slotName)
  {
   Environment *theEnv = theDeftemplate->header.env;

   /*===============================================*/
   /* If we're dealing with an implied deftemplate, */
   /* then the only slot names is "implied."        */
   /*===============================================*/

   if (theDeftemplate->implied)
     {
      if (strcmp(slotName,"implied") == 0)
        { return true; }
      else
        { return false; }
     }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if (FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL) == NULL)
     { return false; }

   /*==================*/
   /* The slot exists. */
   /*==================*/

   return true;
  }

/**********************************************************/
/* DeftemplateSlotFacetExistPFunction: H/L access routine */
/*   for the deftemplate-slot-facet-existp function.      */
/**********************************************************/
void DeftemplateSlotFacetExistPFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   UDFValue facetName;

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*============================*/
   /* Get the name of the facet. */
   /*============================*/

   if (! UDFNextArgument(context,SYMBOL_BIT,&facetName))
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*======================*/
   /* Does the slot exist? */
   /*======================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,DeftemplateSlotFacetExistP(theEnv,theDeftemplate,slotName->contents,facetName.lexemeValue->contents));
  }

/****************************************************/
/* DeftemplateSlotFacetExistP: C access routine for */
/*   the deftemplate-slot-facet-existp function.    */
/****************************************************/
bool DeftemplateSlotFacetExistP(
  Environment *theEnv,
  Deftemplate *theDeftemplate,
  const char *slotName,
  const char *facetName)
  {
   struct templateSlot *theSlot;
   CLIPSLexeme *facetHN;
   struct expr *tempFacet;

   /*=================================================*/
   /* An implied deftemplate doesn't have any facets. */
   /*=================================================*/

   if (theDeftemplate->implied)
     { return false; }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     { return false; }

   /*=======================*/
   /* Search for the facet. */
   /*=======================*/

   facetHN = FindSymbolHN(theEnv,facetName,SYMBOL_BIT);
   for (tempFacet = theSlot->facetList;
        tempFacet != NULL;
        tempFacet = tempFacet->nextArg)
     {
      if (tempFacet->value == facetHN)
        { return true; }
     }

   /*===========================*/
   /* The facet does not exist. */
   /*===========================*/

   return false;
  }

/*********************************************************/
/* DeftemplateSlotFacetValueFunction: H/L access routine */
/*   for the deftemplate-slot-facet-value function.      */
/*********************************************************/
void DeftemplateSlotFacetValueFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Deftemplate *theDeftemplate;
   CLIPSLexeme *slotName;
   UDFValue facetName;

   /*=============================================*/
   /* Set up the default return value for errors. */
   /*=============================================*/

   returnValue->lexemeValue = FalseSymbol(theEnv);

   /*===================================================*/
   /* Retrieve the deftemplate and slot name arguments. */
   /*===================================================*/

   slotName = CheckDeftemplateAndSlotArguments(context,&theDeftemplate);
   if (slotName == NULL)
     { return; }

   /*============================*/
   /* Get the name of the facet. */
   /*============================*/

   if (! UDFNthArgument(context,3,SYMBOL_BIT,&facetName))
     { return; }

   /*===========================*/
   /* Retrieve the facet value. */
   /*===========================*/

   DeftemplateSlotFacetValue(theEnv,theDeftemplate,slotName->contents,facetName.lexemeValue->contents,returnValue);
  }

/****************************************************/
/* DeftemplateSlotFacetValue: C access routine      */
/*   for the deftemplate-slot-facet-value function. */
/****************************************************/
bool DeftemplateSlotFacetValue(
  Environment *theEnv,
  Deftemplate *theDeftemplate,
  const char *slotName,
  const char *facetName,
  UDFValue *rv)
  {
   struct templateSlot *theSlot;
   CLIPSLexeme *facetHN;
   struct expr *tempFacet;

   /*=================================================*/
   /* An implied deftemplate doesn't have any facets. */
   /*=================================================*/

   if (theDeftemplate->implied)
     { return false; }

   /*============================================*/
   /* Otherwise search for the slot name in the  */
   /* list of slots defined for the deftemplate. */
   /*============================================*/

   else if ((theSlot = FindSlot(theDeftemplate,CreateSymbol(theEnv,slotName),NULL)) == NULL)
     { return false; }

   /*=======================*/
   /* Search for the facet. */
   /*=======================*/

   facetHN = FindSymbolHN(theEnv,facetName,SYMBOL_BIT);
   for (tempFacet = theSlot->facetList;
        tempFacet != NULL;
        tempFacet = tempFacet->nextArg)
     {
      if (tempFacet->value == facetHN)
        {
         EvaluateExpression(theEnv,tempFacet->argList,rv);
         return true;
        }
     }

   /*===========================*/
   /* The facet does not exist. */
   /*===========================*/

   return false;
  }

/************************************************************/
/* CheckDeftemplateAndSlotArguments: Checks the deftemplate */
/*   and slot arguments for various functions.              */
/************************************************************/
static CLIPSLexeme *CheckDeftemplateAndSlotArguments(
  UDFContext *context,
  Deftemplate **theDeftemplate)
  {
   UDFValue theArg;
   const char *deftemplateName;
   Environment *theEnv = context->environment;

   /*=======================================*/
   /* Get the reference to the deftemplate. */
   /*=======================================*/

   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg))
     { return NULL; }

   deftemplateName = theArg.lexemeValue->contents;

   *theDeftemplate = FindDeftemplate(theEnv,deftemplateName);
   if (*theDeftemplate == NULL)
     {
      CantFindItemErrorMessage(theEnv,"deftemplate",deftemplateName,true);
      return NULL;
     }

   /*===========================*/
   /* Get the name of the slot. */
   /*===========================*/

   if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
     { return NULL; }

   return theArg.lexemeValue;
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/***************************************************************/
/* UpdateModifyDuplicate: Changes the modify/duplicate command */
/*   found on the RHS of a rule such that the positions of the */
/*   slots for replacement are stored rather than the slot     */
/*   name which allows quicker replacement of slots. This      */
/*   substitution can only take place when the deftemplate     */
/*   type is known (i.e. if a fact-index is used you don't     */
/*   know which type of deftemplate is going to be replaced    */
/*   until you actually do the replacement of slots).          */
/***************************************************************/
bool UpdateModifyDuplicate(
  Environment *theEnv,
  struct expr *top,
  const char *name,
  void *vTheLHS)
  {
   struct expr *functionArgs, *tempArg;
   CLIPSLexeme *templateName;
   Deftemplate *theDeftemplate;
   struct templateSlot *slotPtr;

   /*========================================*/
   /* Determine the fact-address or index to */
   /* be retracted by the modify command.    */
   /*========================================*/

   functionArgs = top->argList;
   if (functionArgs->type == SF_VARIABLE)
     {
      if (SearchParsedBindNames(theEnv,functionArgs->lexemeValue) != 0)
        { return true; }
      templateName = FindTemplateForFactAddress(functionArgs->lexemeValue,
                                                (struct lhsParseNode *) vTheLHS);
      if (templateName == NULL) return true;
     }
   else
     { return true; }

   /*========================================*/
   /* Make sure that the fact being modified */
   /* has a corresponding deftemplate.       */
   /*========================================*/

   theDeftemplate = (Deftemplate *)
                    LookupConstruct(theEnv,DeftemplateData(theEnv)->DeftemplateConstruct,
                                    templateName->contents,
                                    false);

   if (theDeftemplate == NULL) return true;

   if (theDeftemplate->implied) return true;

   /*=============================================================*/
   /* Make sure all the slot names are valid for the deftemplate. */
   /*=============================================================*/

   tempArg = functionArgs->nextArg;
   while (tempArg != NULL)
     {
      /*======================*/
      /* Does the slot exist? */
      /*======================*/

      if ((slotPtr = FindSlot(theDeftemplate,tempArg->lexemeValue,NULL)) == NULL)
        {
         InvalidDeftemplateSlotMessage(theEnv,tempArg->lexemeValue->contents,
                                       theDeftemplate->header.name->contents,true);
         return false;
        }

      /*=========================================================*/
      /* Is a multifield value being put in a single field slot? */
      /*=========================================================*/

      if (slotPtr->multislot == false)
        {
         if (tempArg->argList == NULL)
           {
            SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
            return false;
           }
         else if (tempArg->argList->nextArg != NULL)
           {
            SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
            return false;
           }
         else if (tempArg->argList->type == FCALL)
           {
            if ((ExpressionUnknownFunctionType(tempArg->argList) & SINGLEFIELD_BITS) == 0)
              {
               SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
               return false;
              }
           }
         else if (tempArg->argList->type == MF_VARIABLE)
           {
            SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
            return false;
           }
        }

      /*======================================*/
      /* Are the slot restrictions satisfied? */
      /*======================================*/

      if (CheckRHSSlotTypes(theEnv,tempArg->argList,slotPtr,name) == 0)
        return false;

      /*=============================================*/
      /* Replace the slot with the integer position. */
      /*=============================================*/

      tempArg->type = INTEGER_TYPE;
      tempArg->value = CreateInteger(theEnv,(long long) (FindSlotPosition(theDeftemplate,tempArg->lexemeValue) - 1));

      tempArg = tempArg->nextArg;
     }

   return true;
  }

/**************************************************/
/* FindTemplateForFactAddress: Searches for the   */
/*   deftemplate name associated with the pattern */
/*   to which a fact address has been bound.      */
/**************************************************/
static CLIPSLexeme *FindTemplateForFactAddress(
  CLIPSLexeme *factAddress,
  struct lhsParseNode *theLHS)
  {
   struct lhsParseNode *thePattern = NULL;

   /*===============================================*/
   /* Look through the LHS patterns for the pattern */
   /* which is bound to the fact address used by    */
   /* the modify/duplicate function.                */
   /*===============================================*/

   while (theLHS != NULL)
     {
      if (theLHS->value == (void *) factAddress)
        {
         thePattern = theLHS;
         theLHS = NULL;
        }
      else
        { theLHS = theLHS->bottom; }
     }

   if (thePattern == NULL) return NULL;

   /*=====================================*/
   /* Verify that just a symbol is stored */
   /* as the first field of the pattern.  */
   /*=====================================*/

   thePattern = thePattern->right;
   if ((thePattern->pnType != SF_WILDCARD_NODE) || (thePattern->bottom == NULL))
     { return NULL; }

   thePattern = thePattern->bottom;
   if ((thePattern->pnType != SYMBOL_NODE) ||
            (thePattern->right != NULL) ||
            (thePattern->bottom != NULL))
    { return NULL; }

   /*==============================*/
   /* Return the deftemplate name. */
   /*==============================*/

   return thePattern->lexemeValue;
  }
#endif

/*******************************************/
/* ModifyParse: Parses the modify command. */
/*******************************************/
struct expr *ModifyParse(
  Environment *theEnv,
  struct expr *top,
  const char *logicalName)
  {
   return ModAndDupParse(theEnv,top,logicalName,"modify");
  }

/*************************************************/
/* DuplicateParse: Parses the duplicate command. */
/*************************************************/
struct expr *DuplicateParse(
  Environment *theEnv,
  struct expr *top,
  const char *logicalName)
  {
   return ModAndDupParse(theEnv,top,logicalName,"duplicate");
  }

/*************************************************************/
/* ModAndDupParse: Parses the modify and duplicate commands. */
/*************************************************************/
static struct expr *ModAndDupParse(
  Environment *theEnv,
  struct expr *top,
  const char *logicalName,
  const char *name)
  {
   bool error = false;
   struct token theToken;
   struct expr *nextOne, *tempSlot;
   struct expr *newField, *firstField, *lastField;
   bool printError;
   bool done;

   /*==================================================================*/
   /* Parse the fact-address or index to the modify/duplicate command. */
   /*==================================================================*/

   SavePPBuffer(theEnv," ");
   GetToken(theEnv,logicalName,&theToken);

   if ((theToken.tknType == SF_VARIABLE_TOKEN) || (theToken.tknType == GBL_VARIABLE_TOKEN))
     { nextOne = GenConstant(theEnv,TokenTypeToType(theToken.tknType),theToken.value); }
   else if (theToken.tknType == INTEGER_TOKEN)
     { nextOne = GenConstant(theEnv,INTEGER_TYPE,theToken.value); }
   else if (theToken.tknType == LEFT_PARENTHESIS_TOKEN)
     {
      nextOne = Function1Parse(theEnv,logicalName);
      if (nextOne == NULL)
        {
         ReturnExpression(theEnv,top);
         return NULL;
        }
     }
   else
     {
      ExpectedTypeError2(theEnv,name,1);
      ReturnExpression(theEnv,top);
      return NULL;
     }

   top->argList = nextOne;
   nextOne = top->argList;

   /*=======================================================*/
   /* Parse the remaining modify/duplicate slot specifiers. */
   /*=======================================================*/

   GetToken(theEnv,logicalName,&theToken);
   while (theToken.tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      PPBackup(theEnv);
      SavePPBuffer(theEnv," ");
      SavePPBuffer(theEnv,theToken.printForm);

      /*=================================================*/
      /* Slot definition begins with a left parenthesis. */
      /*=================================================*/

      if (theToken.tknType != LEFT_PARENTHESIS_TOKEN)
        {
         SyntaxErrorMessage(theEnv,"duplicate/modify function");
         ReturnExpression(theEnv,top);
         return NULL;
        }

      /*=================================*/
      /* The slot name must be a symbol. */
      /*=================================*/

      GetToken(theEnv,logicalName,&theToken);
      if (theToken.tknType != SYMBOL_TOKEN)
        {
         SyntaxErrorMessage(theEnv,"duplicate/modify function");
         ReturnExpression(theEnv,top);
         return NULL;
        }

      /*=================================*/
      /* Check for duplicate slot names. */
      /*=================================*/

      for (tempSlot = top->argList->nextArg;
           tempSlot != NULL;
           tempSlot = tempSlot->nextArg)
        {
         if (tempSlot->value == theToken.value)
           {
            AlreadyParsedErrorMessage(theEnv,"slot ",theToken.lexemeValue->contents);
            ReturnExpression(theEnv,top);
            return NULL;
           }
        }

      /*=========================================*/
      /* Add the slot name to the list of slots. */
      /*=========================================*/

      nextOne->nextArg = GenConstant(theEnv,SYMBOL_TYPE,theToken.value);
      nextOne = nextOne->nextArg;

      /*====================================================*/
      /* Get the values to be stored in the specified slot. */
      /*====================================================*/

      firstField = NULL;
      lastField = NULL;
      done = false;
      while (! done)
        {
         SavePPBuffer(theEnv," ");
         newField = GetAssertArgument(theEnv,logicalName,&theToken,&error,
                                      RIGHT_PARENTHESIS_TOKEN,false,&printError);

         if (error)
           {
            if (printError) SyntaxErrorMessage(theEnv,"deftemplate pattern");
            ReturnExpression(theEnv,top);
            return NULL;
           }

         if (newField == NULL)
           { done = true; }

         if (lastField == NULL)
           { firstField = newField; }
         else
           { lastField->nextArg = newField; }
         lastField = newField;
        }

      /*================================================*/
      /* Slot definition ends with a right parenthesis. */
      /*================================================*/

      if (theToken.tknType != RIGHT_PARENTHESIS_TOKEN)
        {
         SyntaxErrorMessage(theEnv,"duplicate/modify function");
         ReturnExpression(theEnv,top);
         ReturnExpression(theEnv,firstField);
         return NULL;
        }
      else
        {
         PPBackup(theEnv);
         PPBackup(theEnv);
         SavePPBuffer(theEnv,")");
        }

      nextOne->argList = firstField;

      GetToken(theEnv,logicalName,&theToken);
     }

   /*================================================*/
   /* Return the parsed modify/duplicate expression. */
   /*================================================*/

   return top;
  }

#endif /* DEFTEMPLATE_CONSTRUCT */

