   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  08/25/16             */
   /*                                                     */
   /*          DEFTEMPLATE RHS PARSING HEADER FILE        */
   /*******************************************************/

/*************************************************************/
/* Purpose: Parses deftemplate fact patterns used with the   */
/*   assert function.                                        */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Added additional argument required for         */
/*            DeriveDefaultFromConstraints.                  */
/*                                                           */
/*            Added additional argument required for         */
/*            InvalidDeftemplateSlotMessage.                 */
/*                                                           */
/*      6.30: Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
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

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT

#include <stdio.h>

#include "default.h"
#include "extnfunc.h"
#include "factrhs.h"
#include "memalloc.h"
#include "modulutl.h"
#include "pprint.h"
#include "prntutil.h"
#include "router.h"
#include "tmpltdef.h"
#include "tmpltfun.h"
#include "tmpltlhs.h"
#include "tmpltutl.h"

#include "tmpltrhs.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static struct expr            *ParseAssertSlotValues(Environment *,const char *,struct token *,struct templateSlot *,bool *,bool);
   static struct expr            *ReorderAssertSlotValues(Environment *,struct templateSlot *,struct expr *,bool *);
   static struct expr            *GetSlotAssertValues(Environment *,struct templateSlot *,struct expr *,bool *);
   static struct expr            *FindAssertSlotItem(struct templateSlot *,struct expr *);
   static struct templateSlot    *ParseSlotLabel(Environment *,const char *,struct token *,Deftemplate *,bool *,TokenType);

/******************************************************************/
/* ParseAssertTemplate: Parses and builds the list of values that */
/*   are used for an assert of a fact with a deftemplate.         */
/******************************************************************/
struct expr *ParseAssertTemplate(
  Environment *theEnv,
  const char *readSource,
  struct token *theToken,
  bool *error,
  TokenType endType,
  bool constantsOnly,
  Deftemplate *theDeftemplate)
  {
   struct expr *firstSlot, *lastSlot, *nextSlot;
   struct expr *firstArg, *tempSlot;
   struct templateSlot *slotPtr;

   firstSlot = NULL;
   lastSlot = NULL;

   /*==============================================*/
   /* Parse each of the slot fields in the assert. */
   /*==============================================*/

   while ((slotPtr = ParseSlotLabel(theEnv,readSource,theToken,theDeftemplate,error,endType)) != NULL)
     {
      /*========================================================*/
      /* Check to see that the slot hasn't already been parsed. */
      /*========================================================*/

      for (tempSlot = firstSlot;
           tempSlot != NULL;
           tempSlot = tempSlot->nextArg)
        {
         if (tempSlot->value == (void *) slotPtr->slotName)
           {
            AlreadyParsedErrorMessage(theEnv,"slot ",slotPtr->slotName->contents);
            *error = true;
            ReturnExpression(theEnv,firstSlot);
            return NULL;
           }
        }

      /*============================================*/
      /* Parse the values to be stored in the slot. */
      /*============================================*/

      nextSlot = ParseAssertSlotValues(theEnv,readSource,theToken,
                                       slotPtr,error,constantsOnly);

      if (*error)
        {
         ReturnExpression(theEnv,firstSlot);
         return NULL;
        }

      /*============================================*/
      /* Check to see if the values to be stored in */
      /* the slot violate the slot's constraints.   */
      /*============================================*/

      if (CheckRHSSlotTypes(theEnv,nextSlot->argList,slotPtr,"assert") == 0)
        {
         *error = true;
         ReturnExpression(theEnv,firstSlot);
         ReturnExpression(theEnv,nextSlot);
         return NULL;
        }

      /*===================================================*/
      /* Add the slot to the list of slots already parsed. */
      /*===================================================*/

      if (lastSlot == NULL)
        { firstSlot = nextSlot; }
      else
        { lastSlot->nextArg = nextSlot; }

      lastSlot = nextSlot;
     }

   /*=================================================*/
   /* Return if an error occured parsing a slot name. */
   /*=================================================*/

   if (*error)
     {
      ReturnExpression(theEnv,firstSlot);
      return NULL;
     }

   /*=============================================================*/
   /* Reorder the arguments to the order used by the deftemplate. */
   /*=============================================================*/

   firstArg = ReorderAssertSlotValues(theEnv,theDeftemplate->slotList,firstSlot,error);
   ReturnExpression(theEnv,firstSlot);

   /*==============================*/
   /* Return the assert arguments. */
   /*==============================*/

   return(firstArg);
  }

/****************************************************************/
/* ParseSlotLabel: Parses the beginning of a slot definition.   */
/*   Checks for opening left parenthesis and a valid slot name. */
/****************************************************************/
static struct templateSlot *ParseSlotLabel(
  Environment *theEnv,
  const char *inputSource,
  struct token *tempToken,
  Deftemplate *theDeftemplate,
  bool *error,
  TokenType endType)
  {
   struct templateSlot *slotPtr;

   /*========================*/
   /* Initialize error flag. */
   /*========================*/

   *error = false;

   /*============================================*/
   /* If token is a right parenthesis, then fact */
   /* template definition is complete.           */
   /*============================================*/

   GetToken(theEnv,inputSource,tempToken);
   if (tempToken->tknType == endType)
     { return NULL; }

   /*=======================================*/
   /* Put a space between the template name */
   /* and the first slot definition.        */
   /*=======================================*/

   PPBackup(theEnv);
   SavePPBuffer(theEnv," ");
   SavePPBuffer(theEnv,tempToken->printForm);

   /*=======================================================*/
   /* Slot definition begins with opening left parenthesis. */
   /*=======================================================*/

   if (tempToken->tknType != LEFT_PARENTHESIS_TOKEN)
     {
      SyntaxErrorMessage(theEnv,"deftemplate pattern");
      *error = true;
      return NULL;
     }

   /*=============================*/
   /* Slot name must be a symbol. */
   /*=============================*/

   GetToken(theEnv,inputSource,tempToken);
   if (tempToken->tknType != SYMBOL_TOKEN)
     {
      SyntaxErrorMessage(theEnv,"deftemplate pattern");
      *error = true;
      return NULL;
     }

   /*======================================================*/
   /* Check that the slot name is valid for this template. */
   /*======================================================*/

   if ((slotPtr = FindSlot(theDeftemplate,tempToken->lexemeValue,NULL)) == NULL)
     {
      InvalidDeftemplateSlotMessage(theEnv,tempToken->lexemeValue->contents,
                                    theDeftemplate->header.name->contents,true);
      *error = true;
      return NULL;
     }

   /*====================================*/
   /* Return a pointer to the slot name. */
   /*====================================*/

   return slotPtr;
  }

/**************************************************************************/
/* ParseAssertSlotValues: Gets a single assert slot value for a template. */
/**************************************************************************/
static struct expr *ParseAssertSlotValues(
  Environment *theEnv,
  const char *inputSource,
  struct token *tempToken,
  struct templateSlot *slotPtr,
  bool *error,
  bool constantsOnly)
  {
   struct expr *nextSlot;
   struct expr *newField, *valueList, *lastValue;
   bool printError;

   /*=============================*/
   /* Handle a single field slot. */
   /*=============================*/

   if (slotPtr->multislot == false)
     {
      /*=====================*/
      /* Get the slot value. */
      /*=====================*/

      SavePPBuffer(theEnv," ");

      newField = GetAssertArgument(theEnv,inputSource,tempToken,error,
                                   RIGHT_PARENTHESIS_TOKEN,constantsOnly,&printError);
      if (*error)
        {
         if (printError) SyntaxErrorMessage(theEnv,"deftemplate pattern");
         return NULL;
        }

      /*=================================================*/
      /* A single field slot value must contain a value. */
      /* Only a multifield slot can be empty.            */
      /*=================================================*/

      if (newField == NULL)
       {
        *error = true;
        SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
        return NULL;
       }

      /*==============================================*/
      /* A function returning a multifield value can  */
      /* not be called to get the value for the slot. */
      /*==============================================*/

      if (newField->type == MF_VARIABLE)
        {
         *error = true;
         SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
         ReturnExpression(theEnv,newField);
         return NULL;
        }
      else if (newField->type == FCALL)
       {
        if ((ExpressionUnknownFunctionType(newField) & SINGLEFIELD_BITS) == 0)
          {
           *error = true;
           SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
           ReturnExpression(theEnv,newField);
           return NULL;
          }
       }

      /*============================*/
      /* Move on to the next token. */
      /*============================*/

      GetToken(theEnv,inputSource,tempToken);
     }

   /*========================================*/
   /* Handle a multifield slot. Build a list */
   /* of the values stored in the slot.      */
   /*========================================*/

   else
     {
      SavePPBuffer(theEnv," ");
      valueList = GetAssertArgument(theEnv,inputSource,tempToken,error,
                                    RIGHT_PARENTHESIS_TOKEN,constantsOnly,&printError);
      if (*error)
        {
         if (printError) SyntaxErrorMessage(theEnv,"deftemplate pattern");
         return NULL;
        }

      if (valueList == NULL)
        {
         PPBackup(theEnv);
         PPBackup(theEnv);
         SavePPBuffer(theEnv,")");
        }

      lastValue = valueList;

      while (lastValue != NULL) /* (tempToken->tknType != RIGHT_PARENTHESIS_TOKEN) */
        {
         if (tempToken->tknType == RIGHT_PARENTHESIS_TOKEN)
           { SavePPBuffer(theEnv," "); }
         else
           {
            /* PPBackup(theEnv); */
            SavePPBuffer(theEnv," ");
            /* SavePPBuffer(theEnv,tempToken->printForm); */
           }

         newField = GetAssertArgument(theEnv,inputSource,tempToken,error,
                                      RIGHT_PARENTHESIS_TOKEN,constantsOnly,&printError);
         if (*error)
           {
            if (printError) SyntaxErrorMessage(theEnv,"deftemplate pattern");
            ReturnExpression(theEnv,valueList);
            return NULL;
           }

         if (newField == NULL)
           {
            PPBackup(theEnv);
            PPBackup(theEnv);
            SavePPBuffer(theEnv,")");
           }

         lastValue->nextArg = newField;
         lastValue = newField;
        }

      newField = valueList;
     }

   /*==========================================================*/
   /* Slot definition must be closed with a right parenthesis. */
   /*==========================================================*/

   if (tempToken->tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      SingleFieldSlotCardinalityError(theEnv,slotPtr->slotName->contents);
      *error = true;
      ReturnExpression(theEnv,newField);
      return NULL;
     }

   /*=========================================================*/
   /* Build and return a structure describing the slot value. */
   /*=========================================================*/

   nextSlot = GenConstant(theEnv,SYMBOL_TYPE,slotPtr->slotName);
   nextSlot->argList = newField;

   return(nextSlot);
  }

/*************************************************************************/
/* ReorderAssertSlotValues: Rearranges the asserted values to correspond */
/*   to the order of the values described by the deftemplate.            */
/*************************************************************************/
static struct expr *ReorderAssertSlotValues(
  Environment *theEnv,
  struct templateSlot *slotPtr,
  struct expr *firstSlot,
  bool *error)
  {
   struct expr *firstArg = NULL;
   struct expr *lastArg = NULL, *newArg;

   /*=============================================*/
   /* Loop through each of the slots in the order */
   /* they're found in the deftemplate.           */
   /*=============================================*/

   for (;
        slotPtr != NULL;
        slotPtr = slotPtr->next)
     {
      /*==============================================*/
      /* Get either the value specified in the assert */
      /* command or the default value for the slot.   */
      /*==============================================*/

      newArg = GetSlotAssertValues(theEnv,slotPtr,firstSlot,error);

      if (*error)
        {
         ReturnExpression(theEnv,firstArg);
         return NULL;
        }

      /*=====================================*/
      /* Add the value to the list of values */
      /* for the assert command.             */
      /*=====================================*/

      if (newArg != NULL)
        {
         if (lastArg == NULL)
           { firstArg = newArg; }
         else
           { lastArg->nextArg = newArg; }

         lastArg = newArg;
        }
     }

   /*==============================*/
   /* Return the list of arguments */
   /* for the assert command.      */
   /*==============================*/

   return(firstArg);
  }

/***************************************************************/
/* GetSlotAssertValues: Gets the assert value for a given slot */
/*   of a deftemplate. If the value was supplied by the user,  */
/*   it will be used. If not the default value or default      */
/*   default value will be used.                               */
/***************************************************************/
static struct expr *GetSlotAssertValues(
  Environment *theEnv,
  struct templateSlot *slotPtr,
  struct expr *firstSlot,
  bool *error)
  {
   struct expr *slotItem;
   struct expr *newArg, *tempArg;
   UDFValue theDefault;
   const char *nullBitMap = "\0";

   /*==================================================*/
   /* Determine if the slot is assigned in the assert. */
   /*==================================================*/

   slotItem = FindAssertSlotItem(slotPtr,firstSlot);

   /*==========================================*/
   /* If the slot is assigned, use that value. */
   /*==========================================*/

   if (slotItem != NULL)
     {
      newArg = slotItem->argList;
      slotItem->argList = NULL;
     }

   /*=================================*/
   /* Otherwise, use a default value. */
   /*=================================*/

   else
     {
      /*================================================*/
      /* If the (default ?NONE) attribute was specified */
      /* for the slot, then a value must be supplied.   */
      /*================================================*/

      if (slotPtr->noDefault)
        {
         PrintErrorID(theEnv,"TMPLTRHS",1,true);
         WriteString(theEnv,STDERR,"Slot '");
         WriteString(theEnv,STDERR,slotPtr->slotName->contents);
         WriteString(theEnv,STDERR,"' requires a value because of its (default ?NONE) attribute.\n");
         *error = true;
         return NULL;
        }

      /*===================================================*/
      /* If the (default ?DERIVE) attribute was specified  */
      /* (the default), then derive the default value from */
      /* the slot's constraints.                           */
      /*===================================================*/

      else if ((slotPtr->defaultPresent == false) &&
               (slotPtr->defaultDynamic == false))
        {
         DeriveDefaultFromConstraints(theEnv,slotPtr->constraints,&theDefault,
                                      slotPtr->multislot,true);
         newArg = ConvertValueToExpression(theEnv,&theDefault);
        }

      /*=========================================*/
      /* Otherwise, use the expression contained */
      /* in the default attribute.               */
      /*=========================================*/

      else
        { newArg = CopyExpression(theEnv,slotPtr->defaultList); }
     }

   /*=======================================================*/
   /* Since a multifield slot default can contain a list of */
   /* values, the values need to have a store-multifield    */
   /* function called wrapped around it to group all of the */
   /* values into a single multifield value.                */
   /*=======================================================*/

   if (slotPtr->multislot)
     {
      tempArg = GenConstant(theEnv,FACT_STORE_MULTIFIELD,AddBitMap(theEnv,(void *) nullBitMap,1));
      tempArg->argList = newArg;
      newArg = tempArg;
     }

   /*==============================================*/
   /* Return the value to be asserted in the slot. */
   /*==============================================*/

   return(newArg);
  }

/*******************************************************************/
/* FindAssertSlotItem: Finds a particular slot in a list of slots. */
/*******************************************************************/
static struct expr *FindAssertSlotItem(
  struct templateSlot *slotPtr,
  struct expr *listOfSlots)
  {
   while (listOfSlots != NULL)
     {
      if (listOfSlots->value == (void *) slotPtr->slotName) return (listOfSlots);
      listOfSlots = listOfSlots->nextArg;
     }

   return NULL;
  }

#endif /* DEFTEMPLATE_CONSTRUCT */

