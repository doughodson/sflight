   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  11/01/16             */
   /*                                                     */
   /*            DEFTEMPLATE UTILITIES MODULE             */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides utility routines for deftemplates.      */
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
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Added additional arguments to                  */
/*            InvalidDeftemplateSlotMessage function.        */
/*                                                           */
/*            Added additional arguments to                  */
/*            PrintTemplateFact function.                    */
/*                                                           */
/*      6.30: Support for long long integers.                */
/*                                                           */
/*            Used gensprintf instead of sprintf.            */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
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
/*            Static constraint checking is always enabled.  */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Watch facts for modify command only prints     */
/*            changed slots.                                 */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT

#include <stdio.h>
#include <string.h>

#include "argacces.h"
#include "constrct.h"
#include "cstrnchk.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "memalloc.h"
#include "modulutl.h"
#include "multifld.h"
#include "prntutil.h"
#include "router.h"
#include "sysdep.h"
#include "tmpltbsc.h"
#include "tmpltdef.h"
#include "tmpltfun.h"
#include "tmpltpsr.h"
#include "watch.h"

#include "tmpltutl.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                     PrintTemplateSlot(Environment *,const char *,struct templateSlot *,CLIPSValue *);
   static struct templateSlot     *GetNextTemplateSlotToPrint(Environment *,struct fact *,struct templateSlot *,int *,int,const char *);

/********************************************************/
/* InvalidDeftemplateSlotMessage: Generic error message */
/*   for use when a specified slot name isn't defined   */
/*   in its corresponding deftemplate.                  */
/********************************************************/
void InvalidDeftemplateSlotMessage(
  Environment *theEnv,
  const char *slotName,
  const char *deftemplateName,
  bool printCR)
  {
   PrintErrorID(theEnv,"TMPLTDEF",1,printCR);
   WriteString(theEnv,STDERR,"Invalid slot '");
   WriteString(theEnv,STDERR,slotName);
   WriteString(theEnv,STDERR,"' not defined in corresponding deftemplate '");
   WriteString(theEnv,STDERR,deftemplateName);
   WriteString(theEnv,STDERR,"'.\n");
  }

/**********************************************************/
/* SingleFieldSlotCardinalityError: Generic error message */
/*   used when an attempt is made to placed a multifield  */
/*   value into a single field slot.                      */
/**********************************************************/
void SingleFieldSlotCardinalityError(
  Environment *theEnv,
  const char *slotName)
  {
   PrintErrorID(theEnv,"TMPLTDEF",2,true);
   WriteString(theEnv,STDERR,"The single field slot '");
   WriteString(theEnv,STDERR,slotName);
   WriteString(theEnv,STDERR,"' can only contain a single field value.\n");
  }

/**********************************************************************/
/* MultiIntoSingleFieldSlotError: Determines if a multifield value is */
/*   being placed into a single field slot of a deftemplate fact.     */
/**********************************************************************/
void MultiIntoSingleFieldSlotError(
  Environment *theEnv,
  struct templateSlot *theSlot,
  Deftemplate *theDeftemplate)
  {
   PrintErrorID(theEnv,"TMPLTFUN",1,true);
   WriteString(theEnv,STDERR,"Attempted to assert a multifield value ");
   WriteString(theEnv,STDERR,"into the single field slot ");
   if (theSlot != NULL)
     {
      WriteString(theEnv,STDERR,"'");
      WriteString(theEnv,STDERR,theSlot->slotName->contents);
      WriteString(theEnv,STDERR,"'");
     }
   else
     { WriteString(theEnv,STDERR,"<<unknown>>"); }
   WriteString(theEnv,STDERR," of deftemplate ");
   if (theDeftemplate != NULL)
     {
      WriteString(theEnv,STDERR,"'");
      WriteString(theEnv,STDERR,theDeftemplate->header.name->contents);
      WriteString(theEnv,STDERR,"'");
     }
   else
     { WriteString(theEnv,STDERR,"<<unknown>>"); }
   WriteString(theEnv,STDERR,".\n");

   SetEvaluationError(theEnv,true);
  }

/**************************************************************/
/* CheckTemplateFact: Checks a fact to see if it violates any */
/*   deftemplate type, allowed-..., or range specifications.  */
/**************************************************************/
void CheckTemplateFact(
  Environment *theEnv,
  Fact *theFact)
  {
   CLIPSValue *sublist;
   int i;
   Deftemplate *theDeftemplate;
   struct templateSlot *slotPtr;
   UDFValue theData;
   char thePlace[20];
   ConstraintViolationType rv;

   if (! GetDynamicConstraintChecking(theEnv)) return;

   sublist = theFact->theProposition.contents;

   /*========================================================*/
   /* If the deftemplate corresponding to the first field of */
   /* of the fact cannot be found, then the fact cannot be   */
   /* checked against the deftemplate format.                */
   /*========================================================*/

   theDeftemplate = theFact->whichDeftemplate;
   if (theDeftemplate == NULL) return;
   if (theDeftemplate->implied) return;

   /*=============================================*/
   /* Check each of the slots of the deftemplate. */
   /*=============================================*/

   i = 0;
   for (slotPtr = theDeftemplate->slotList;
        slotPtr != NULL;
        slotPtr = slotPtr->next)
     {
      /*================================================*/
      /* Store the slot value in the appropriate format */
      /* for a call to the constraint checking routine. */
      /*================================================*/

      if (slotPtr->multislot == false)
        {
         theData.value = sublist[i].value;
         i++;
        }
      else
        {
         theData.value = (void *) sublist[i].value;
         theData.begin = 0;
         theData.range = sublist[i].multifieldValue->length;
         i++;
        }

      /*=============================================*/
      /* Call the constraint checking routine to see */
      /* if a constraint violation occurred.         */
      /*=============================================*/

      rv = ConstraintCheckDataObject(theEnv,&theData,slotPtr->constraints);
      if (rv != NO_VIOLATION)
        {
         gensprintf(thePlace,"fact f-%lld",theFact->factIndex);

         PrintErrorID(theEnv,"CSTRNCHK",1,true);
         WriteString(theEnv,STDERR,"Slot value ");
         WriteUDFValue(theEnv,STDERR,&theData);
         ConstraintViolationErrorMessage(theEnv,NULL,thePlace,false,0,slotPtr->slotName,
                                         0,rv,slotPtr->constraints,true);
         SetHaltExecution(theEnv,true);
         return;
        }
     }

   return;
  }

/***********************************************************************/
/* CheckRHSSlotTypes: Checks the validity of a change to a slot as the */
/*   result of an assert, modify, or duplicate command. This checking  */
/*   is performed statically (i.e. when the command is being parsed).  */
/***********************************************************************/
bool CheckRHSSlotTypes(
  Environment *theEnv,
  struct expr *rhsSlots,
  struct templateSlot *slotPtr,
  const char *thePlace)
  {
   ConstraintViolationType rv;
   const char *theName;

   rv = ConstraintCheckExpressionChain(theEnv,rhsSlots,slotPtr->constraints);
   if (rv != NO_VIOLATION)
     {
      if (rv != CARDINALITY_VIOLATION) theName = "A literal slot value";
      else theName = "Literal slot values";
      ConstraintViolationErrorMessage(theEnv,theName,thePlace,true,0,
                                      slotPtr->slotName,0,rv,slotPtr->constraints,true);
      return false;
     }

   return true;
  }

/*********************************************************/
/* GetNthSlot: Given a deftemplate and an integer index, */
/*   returns the nth slot of a deftemplate.              */
/*********************************************************/
struct templateSlot *GetNthSlot(
  Deftemplate *theDeftemplate,
  long long position)
  {
   struct templateSlot *slotPtr;
   long long i = 0;

   slotPtr = theDeftemplate->slotList;
   while (slotPtr != NULL)
     {
      if (i == position) return slotPtr;
      slotPtr = slotPtr->next;
      i++;
     }

   return NULL;
  }

/*******************************************************/
/* FindSlotPosition: Finds the position of a specified */
/*   slot in a deftemplate structure.                  */
/*******************************************************/
int FindSlotPosition(
  Deftemplate *theDeftemplate,
  CLIPSLexeme *name)
  {
   struct templateSlot *slotPtr;
   int position;

   for (slotPtr = theDeftemplate->slotList, position = 1;
        slotPtr != NULL;
        slotPtr = slotPtr->next, position++)
     {
      if (slotPtr->slotName == name)
        { return(position); }
     }

   return 0;
  }

/**********************/
/* PrintTemplateSlot: */
/**********************/
static void PrintTemplateSlot(
  Environment *theEnv,
  const char *logicalName,
  struct templateSlot *slotPtr,
  CLIPSValue *slotValue)
  {
   WriteString(theEnv,logicalName,"(");
   WriteString(theEnv,logicalName,slotPtr->slotName->contents);

   /*======================================================*/
   /* Print the value of the slot for a single field slot. */
   /*======================================================*/

   if (slotPtr->multislot == false)
     {
      WriteString(theEnv,logicalName," ");
      PrintAtom(theEnv,logicalName,((TypeHeader *) slotValue->value)->type,slotValue->value);
     }

   /*==========================================================*/
   /* Else print the value of the slot for a multi field slot. */
   /*==========================================================*/

   else
     {
      struct multifield *theSegment;

      theSegment = (Multifield *) slotValue->value;
      if (theSegment->length > 0)
        {
         WriteString(theEnv,logicalName," ");
         PrintMultifieldDriver(theEnv,logicalName,theSegment,0,theSegment->length,false);
        }
     }

   /*============================================*/
   /* Print the closing parenthesis of the slot. */
   /*============================================*/

   WriteString(theEnv,logicalName,")");
  }

/********************************/
/* GetNextTemplateSloteToPrint: */
/********************************/
static struct templateSlot *GetNextTemplateSlotToPrint(
  Environment *theEnv,
  struct fact *theFact,
  struct templateSlot *slotPtr,
  int *position,
  int ignoreDefaults,
  const char *changeMap)
  {
   UDFValue tempDO;
   CLIPSValue *sublist;

   sublist = theFact->theProposition.contents;
   if (slotPtr == NULL)
     { slotPtr = theFact->whichDeftemplate->slotList; }
   else
     {
      slotPtr = slotPtr->next;
      (*position)++;
     }

   while (slotPtr != NULL)
     {
      if ((changeMap != NULL) && (TestBitMap(changeMap,*position) == 0))
        {
         (*position)++;
         slotPtr = slotPtr->next;
         continue;
        }

      if (ignoreDefaults && (slotPtr->defaultDynamic == false))
        {
         DeftemplateSlotDefault(theEnv,theFact->whichDeftemplate,slotPtr,&tempDO,true);

         if (slotPtr->multislot == false)
           {
            if (tempDO.value == sublist[*position].value)
              {
               (*position)++;
               slotPtr = slotPtr->next;
               continue;
              }
           }
         else if (MultifieldsEqual((Multifield *) tempDO.value,
                                   (Multifield *) sublist[*position].value))
           {
            (*position)++;
            slotPtr = slotPtr->next;
            continue;
           }
        }

      return slotPtr;
     }

   return NULL;
  }

/**********************************************************/
/* PrintTemplateFact: Prints a fact using the deftemplate */
/*   format. Returns true if the fact was printed using   */
/*   this format, otherwise false.                        */
/**********************************************************/
void PrintTemplateFact(
  Environment *theEnv,
  const char *logicalName,
  Fact *theFact,
  bool separateLines,
  bool ignoreDefaults,
  const char *changeMap)
  {
   CLIPSValue *sublist;
   int i;
   Deftemplate *theDeftemplate;
   struct templateSlot *slotPtr, *lastPtr = NULL;
   bool slotPrinted = false;

   /*==============================*/
   /* Initialize some information. */
   /*==============================*/

   theDeftemplate = theFact->whichDeftemplate;
   sublist = theFact->theProposition.contents;

   /*=============================================*/
   /* Print the relation name of the deftemplate. */
   /*=============================================*/

   WriteString(theEnv,logicalName,"(");
   WriteString(theEnv,logicalName,theDeftemplate->header.name->contents);

   /*===================================================*/
   /* Print each of the field slots of the deftemplate. */
   /*===================================================*/

   i = 0;
   slotPtr = GetNextTemplateSlotToPrint(theEnv,theFact,lastPtr,&i,
                                        ignoreDefaults,changeMap);

   if ((changeMap != NULL) &&
       (theFact->whichDeftemplate->slotList != slotPtr))
     { WriteString(theEnv,logicalName," ..."); }

   while (slotPtr != NULL)
     {
      /*===========================================*/
      /* Print the opening parenthesis of the slot */
      /* and the slot name.                        */
      /*===========================================*/

      if (! slotPrinted)
        {
         slotPrinted = true;
         WriteString(theEnv,logicalName," ");
        }

      if (separateLines)
        { WriteString(theEnv,logicalName,"\n   "); }

      /*====================================*/
      /* Print the slot name and its value. */
      /*====================================*/

      PrintTemplateSlot(theEnv,logicalName,slotPtr,&sublist[i]);

      /*===========================*/
      /* Move on to the next slot. */
      /*===========================*/

      lastPtr = slotPtr;
      slotPtr = GetNextTemplateSlotToPrint(theEnv,theFact,lastPtr,&i,
                                           ignoreDefaults,changeMap);

      if ((changeMap != NULL) && (lastPtr->next != slotPtr))
        { WriteString(theEnv,logicalName," ..."); }

      if (slotPtr != NULL) WriteString(theEnv,logicalName," ");
     }

   WriteString(theEnv,logicalName,")");
  }

/***************************************************************************/
/* UpdateDeftemplateScope: Updates the scope flag of all the deftemplates. */
/***************************************************************************/
void UpdateDeftemplateScope(
  Environment *theEnv)
  {
   Deftemplate *theDeftemplate;
   unsigned int moduleCount;
   Defmodule *theModule;
   struct defmoduleItemHeader *theItem;

   /*==================================*/
   /* Loop through all of the modules. */
   /*==================================*/

   for (theModule = GetNextDefmodule(theEnv,NULL);
        theModule != NULL;
        theModule = GetNextDefmodule(theEnv,theModule))
     {
      /*======================================================*/
      /* Loop through each of the deftemplates in the module. */
      /*======================================================*/

      theItem = (struct defmoduleItemHeader *)
                GetModuleItem(theEnv,theModule,DeftemplateData(theEnv)->DeftemplateModuleIndex);

      for (theDeftemplate = (Deftemplate *) theItem->firstItem;
           theDeftemplate != NULL ;
           theDeftemplate = GetNextDeftemplate(theEnv,theDeftemplate))
        {
         /*=======================================*/
         /* If the deftemplate can be seen by the */
         /* current module, then it is in scope.  */
         /*=======================================*/

         if (FindImportedConstruct(theEnv,"deftemplate",theModule,
                                   theDeftemplate->header.name->contents,
                                   &moduleCount,true,NULL) != NULL)
           { theDeftemplate->inScope = true; }
         else
           { theDeftemplate->inScope = false; }
        }
     }
  }

/****************************************************************/
/* FindSlot: Finds a specified slot in a deftemplate structure. */
/****************************************************************/
struct templateSlot *FindSlot(
  Deftemplate *theDeftemplate,
  CLIPSLexeme *name,
  unsigned short *whichOne)
  {
   struct templateSlot *slotPtr;

   if (whichOne != NULL) *whichOne = 0;
   slotPtr = theDeftemplate->slotList;
   while (slotPtr != NULL)
     {
      if (slotPtr->slotName == name)
        { return(slotPtr); }
      if (whichOne != NULL) (*whichOne)++;
      slotPtr = slotPtr->next;
     }

   return NULL;
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/************************************************************/
/* CreateImpliedDeftemplate: Creates an implied deftemplate */
/*   and adds it to the list of deftemplates.               */
/************************************************************/
Deftemplate *CreateImpliedDeftemplate(
  Environment *theEnv,
  CLIPSLexeme *deftemplateName,
  bool setFlag)
  {
   Deftemplate *newDeftemplate;

   newDeftemplate = get_struct(theEnv,deftemplate);
   newDeftemplate->header.name = deftemplateName;
   newDeftemplate->header.ppForm = NULL;
   newDeftemplate->header.usrData = NULL;
   newDeftemplate->header.constructType = DEFTEMPLATE;
   newDeftemplate->header.env = theEnv;
   newDeftemplate->slotList = NULL;
   newDeftemplate->implied = setFlag;
   newDeftemplate->numberOfSlots = 0;
   newDeftemplate->inScope = 1;
   newDeftemplate->patternNetwork = NULL;
   newDeftemplate->factList = NULL;
   newDeftemplate->lastFact = NULL;
   newDeftemplate->busyCount = 0;
   newDeftemplate->watch = false;
   newDeftemplate->header.next = NULL;

#if DEBUGGING_FUNCTIONS
   if (GetWatchItem(theEnv,"facts") == 1)
     { DeftemplateSetWatch(newDeftemplate,true); }
#endif

   newDeftemplate->header.whichModule = (struct defmoduleItemHeader *)
                                        GetModuleItem(theEnv,NULL,DeftemplateData(theEnv)->DeftemplateModuleIndex);

   AddConstructToModule(&newDeftemplate->header);
   InstallDeftemplate(theEnv,newDeftemplate);

   return(newDeftemplate);
  }

#endif

#endif /* DEFTEMPLATE_CONSTRUCT */

