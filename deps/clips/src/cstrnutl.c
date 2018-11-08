   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/18/16             */
   /*                                                     */
   /*             CONSTRAINT UTILITY MODULE               */
   /*******************************************************/

/*************************************************************/
/* Purpose: Utility routines for manipulating, initializing, */
/*   creating, copying, and comparing constraint records.    */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian Dantes                                         */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Added allowed-classes slot facet.              */
/*                                                           */
/*      6.30: Support for long long integers.                */
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
/*            Eval support for run time and bload only.      */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "setup.h"

#include "argacces.h"
#include "constant.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "memalloc.h"
#include "multifld.h"
#include "router.h"
#include "scanner.h"

#include "cstrnutl.h"

/************************************************/
/* GetConstraintRecord: Creates and initializes */
/*   the values of a constraint record.         */
/************************************************/
struct constraintRecord *GetConstraintRecord(
  Environment *theEnv)
  {
   CONSTRAINT_RECORD *constraints;
   unsigned i;

   constraints = get_struct(theEnv,constraintRecord);

   for (i = 0 ; i < sizeof(CONSTRAINT_RECORD) ; i++)
     { ((char *) constraints)[i] = '\0'; }

   SetAnyAllowedFlags(constraints,true);

   constraints->multifieldsAllowed = false;
   constraints->singlefieldsAllowed = true;

   constraints->anyRestriction = false;
   constraints->symbolRestriction = false;
   constraints->stringRestriction = false;
   constraints->floatRestriction = false;
   constraints->integerRestriction = false;
   constraints->classRestriction = false;
   constraints->instanceNameRestriction = false;
   constraints->classList = NULL;
   constraints->restrictionList = NULL;
   constraints->minValue = GenConstant(theEnv,SYMBOL_TYPE,SymbolData(theEnv)->NegativeInfinity);
   constraints->maxValue = GenConstant(theEnv,SYMBOL_TYPE,SymbolData(theEnv)->PositiveInfinity);
   constraints->minFields = GenConstant(theEnv,INTEGER_TYPE,SymbolData(theEnv)->Zero);
   constraints->maxFields = GenConstant(theEnv,SYMBOL_TYPE,SymbolData(theEnv)->PositiveInfinity);
   constraints->installed = false;
   constraints->bucket = 0;
   constraints->count = 0;
   constraints->multifield = NULL;
   constraints->next = NULL;

   return constraints;
  }

/********************************************************/
/* SetAnyAllowedFlags: Sets the allowed type flags of a */
/*   constraint record to allow all types. If passed an */
/*   argument of true, just the "any allowed" flag is   */
/*   set to true. If passed an argument of false, then  */
/*   all of the individual type flags are set to true.  */
/********************************************************/
void SetAnyAllowedFlags(
  CONSTRAINT_RECORD *theConstraint,
  bool justOne)
  {
   bool flag1, flag2;

   if (justOne)
     {
      flag1 = true;
      flag2 = false;
     }
   else
     {
      flag1 = false;
      flag2 = true;
     }

   theConstraint->anyAllowed = flag1;
   theConstraint->symbolsAllowed = flag2;
   theConstraint->stringsAllowed = flag2;
   theConstraint->floatsAllowed = flag2;
   theConstraint->integersAllowed = flag2;
   theConstraint->instanceNamesAllowed = flag2;
   theConstraint->instanceAddressesAllowed = flag2;
   theConstraint->externalAddressesAllowed = flag2;
   theConstraint->voidAllowed = flag2;
   theConstraint->factAddressesAllowed = flag2;
  }

/*****************************************************/
/* CopyConstraintRecord: Copies a constraint record. */
/*****************************************************/
struct constraintRecord *CopyConstraintRecord(
  Environment *theEnv,
  CONSTRAINT_RECORD *sourceConstraint)
  {
   CONSTRAINT_RECORD *theConstraint;

   if (sourceConstraint == NULL) return NULL;

   theConstraint = get_struct(theEnv,constraintRecord);

   theConstraint->anyAllowed = sourceConstraint->anyAllowed;
   theConstraint->symbolsAllowed = sourceConstraint->symbolsAllowed;
   theConstraint->stringsAllowed = sourceConstraint->stringsAllowed;
   theConstraint->floatsAllowed = sourceConstraint->floatsAllowed;
   theConstraint->integersAllowed = sourceConstraint->integersAllowed;
   theConstraint->instanceNamesAllowed = sourceConstraint->instanceNamesAllowed;
   theConstraint->instanceAddressesAllowed = sourceConstraint->instanceAddressesAllowed;
   theConstraint->externalAddressesAllowed = sourceConstraint->externalAddressesAllowed;
   theConstraint->voidAllowed = sourceConstraint->voidAllowed;
   theConstraint->multifieldsAllowed = sourceConstraint->multifieldsAllowed;
   theConstraint->singlefieldsAllowed = sourceConstraint->singlefieldsAllowed;
   theConstraint->factAddressesAllowed = sourceConstraint->factAddressesAllowed;
   theConstraint->anyRestriction = sourceConstraint->anyRestriction;
   theConstraint->symbolRestriction = sourceConstraint->symbolRestriction;
   theConstraint->stringRestriction = sourceConstraint->stringRestriction;
   theConstraint->floatRestriction = sourceConstraint->floatRestriction;
   theConstraint->integerRestriction = sourceConstraint->integerRestriction;
   theConstraint->classRestriction = sourceConstraint->classRestriction;
   theConstraint->instanceNameRestriction = sourceConstraint->instanceNameRestriction;
   theConstraint->classList = CopyExpression(theEnv,sourceConstraint->classList);
   theConstraint->restrictionList = CopyExpression(theEnv,sourceConstraint->restrictionList);
   theConstraint->minValue = CopyExpression(theEnv,sourceConstraint->minValue);
   theConstraint->maxValue = CopyExpression(theEnv,sourceConstraint->maxValue);
   theConstraint->minFields = CopyExpression(theEnv,sourceConstraint->minFields);
   theConstraint->maxFields = CopyExpression(theEnv,sourceConstraint->maxFields);
   theConstraint->bucket = 0;
   theConstraint->installed = false;
   theConstraint->count = 0;
   theConstraint->multifield = CopyConstraintRecord(theEnv,sourceConstraint->multifield);
   theConstraint->next = NULL;

   return(theConstraint);
  }

/**************************************************************/
/* SetAnyRestrictionFlags: Sets the restriction type flags of */
/*   a constraint record to indicate there are restriction on */
/*   all types. If passed an argument of true, just the       */
/*   "any restriction" flag is set to true. If passed an      */
/*   argument of false, then all of the individual type       */
/*   restriction flags are set to true.                       */
/**************************************************************/
void SetAnyRestrictionFlags(
  CONSTRAINT_RECORD *theConstraint,
  bool justOne)
  {
   bool flag1, flag2;

   if (justOne)
     {
      flag1 = true;
      flag2 = false;
     }
   else
     {
      flag1 = false;
      flag2 = true;
     }

   theConstraint->anyRestriction = flag1;
   theConstraint->symbolRestriction = flag2;
   theConstraint->stringRestriction = flag2;
   theConstraint->floatRestriction = flag2;
   theConstraint->integerRestriction = flag2;
   theConstraint->instanceNameRestriction = flag2;
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/*****************************************************/
/* SetConstraintType: Given a constraint type and a  */
/*   constraint, sets the allowed type flags for the */
/*   specified type in the constraint to true.       */
/*****************************************************/
bool SetConstraintType(
  int theType,
  CONSTRAINT_RECORD *constraints)
  {
   bool rv = true;

   switch(theType)
     {
      case UNKNOWN_VALUE:
         rv = constraints->anyAllowed;
         constraints->anyAllowed = true;
         break;

      case SYMBOL_TYPE:
         rv = constraints->symbolsAllowed;
         constraints->symbolsAllowed = true;
         break;

      case STRING_TYPE:
         rv = constraints->stringsAllowed;
         constraints->stringsAllowed = true;
         break;

      case SYMBOL_OR_STRING:
         rv = (constraints->stringsAllowed || constraints->symbolsAllowed);
         constraints->symbolsAllowed = true;
         constraints->stringsAllowed = true;
         break;

      case INTEGER_TYPE:
         rv = constraints->integersAllowed;
         constraints->integersAllowed = true;
         break;

      case FLOAT_TYPE:
         rv = constraints->floatsAllowed;
         constraints->floatsAllowed = true;
         break;

      case INTEGER_OR_FLOAT:
         rv = (constraints->integersAllowed || constraints->floatsAllowed);
         constraints->integersAllowed = true;
         constraints->floatsAllowed = true;
         break;

      case INSTANCE_ADDRESS_TYPE:
         rv = constraints->instanceAddressesAllowed;
         constraints->instanceAddressesAllowed = true;
         break;

      case INSTANCE_NAME_TYPE:
         rv = constraints->instanceNamesAllowed;
         constraints->instanceNamesAllowed = true;
         break;

      case INSTANCE_OR_INSTANCE_NAME:
         rv = (constraints->instanceNamesAllowed || constraints->instanceAddressesAllowed);
         constraints->instanceNamesAllowed = true;
         constraints->instanceAddressesAllowed = true;
         break;

      case EXTERNAL_ADDRESS_TYPE:
         rv = constraints->externalAddressesAllowed;
         constraints->externalAddressesAllowed = true;
         break;

      case VOID_TYPE:
         rv = constraints->voidAllowed;
         constraints->voidAllowed = true;
         break;

      case FACT_ADDRESS_TYPE:
         rv = constraints->factAddressesAllowed;
         constraints->factAddressesAllowed = true;
         break;

      case MULTIFIELD_TYPE:
         rv = constraints->multifieldsAllowed;
         constraints->multifieldsAllowed = true;
         break;
     }

   if (theType != UNKNOWN_VALUE) constraints->anyAllowed = false;
   return(rv);
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) */

/*************************************************************/
/* CompareNumbers: Given two numbers (which can be integers, */
/*   floats, or the symbols for positive/negative infinity)  */
/*   returns the relationship between the numbers (greater   */
/*   than, less than or equal).                              */
/*************************************************************/
int CompareNumbers(
  Environment *theEnv,
  int type1,
  void *vptr1,
  int type2,
  void *vptr2)
  {
   /*============================================*/
   /* Handle the situation in which the values   */
   /* are exactly equal (same type, same value). */
   /*============================================*/

   if (vptr1 == vptr2) return(EQUAL);

   /*=======================================*/
   /* Handle the special cases for positive */
   /* and negative infinity.                */
   /*=======================================*/

   if (vptr1 == SymbolData(theEnv)->PositiveInfinity) return(GREATER_THAN);

   if (vptr1 == SymbolData(theEnv)->NegativeInfinity) return(LESS_THAN);

   if (vptr2 == SymbolData(theEnv)->PositiveInfinity) return(LESS_THAN);

   if (vptr2 == SymbolData(theEnv)->NegativeInfinity) return(GREATER_THAN);

   /*=======================*/
   /* Compare two integers. */
   /*=======================*/

   if ((type1 == INTEGER_TYPE) && (type2 == INTEGER_TYPE))
     {
      if (((CLIPSInteger *) vptr1)->contents < ((CLIPSInteger *) vptr2)->contents)
        { return(LESS_THAN); }
      else if (((CLIPSInteger *) vptr1)->contents > ((CLIPSInteger *) vptr2)->contents)
        { return(GREATER_THAN); }

      return(EQUAL);
     }

   /*=====================*/
   /* Compare two floats. */
   /*=====================*/

   if ((type1 == FLOAT_TYPE) && (type2 == FLOAT_TYPE))
     {
      if (((CLIPSFloat *) vptr1)->contents < ((CLIPSFloat *) vptr2)->contents)
        { return(LESS_THAN); }
      else if (((CLIPSFloat *) vptr1)->contents > ((CLIPSFloat *) vptr2)->contents)
        { return(GREATER_THAN); }

      return(EQUAL);
     }

   /*================================*/
   /* Compare an integer to a float. */
   /*================================*/

   if ((type1 == INTEGER_TYPE) && (type2 == FLOAT_TYPE))
     {
      if (((double) ((CLIPSInteger *) vptr1)->contents) < ((CLIPSFloat *) vptr2)->contents)
        { return(LESS_THAN); }
      else if (((double) ((CLIPSInteger *) vptr1)->contents) > ((CLIPSFloat *) vptr2)->contents)
        { return(GREATER_THAN); }

      return(EQUAL);
     }

   /*================================*/
   /* Compare a float to an integer. */
   /*================================*/

   if ((type1 == FLOAT_TYPE) && (type2 == INTEGER_TYPE))
     {
      if (((CLIPSFloat *) vptr1)->contents < ((double) ((CLIPSInteger *) vptr2)->contents))
        { return(LESS_THAN); }
      else if (((CLIPSFloat *) vptr1)->contents > ((double) ((CLIPSInteger *) vptr2)->contents))
        { return(GREATER_THAN); }

      return(EQUAL);
     }

   /*===================================*/
   /* One of the arguments was invalid. */
   /* Return -1 to indicate an error.   */
   /*===================================*/

   return(-1);
  }

/****************************************************************/
/* ExpressionToConstraintRecord: Converts an expression into a  */
/*   constraint record. For example, an expression representing */
/*   the symbol BLUE would be converted to a  record with       */
/*   allowed types SYMBOL_TYPE and allow-values BLUE.                */
/****************************************************************/
CONSTRAINT_RECORD *ExpressionToConstraintRecord(
  Environment *theEnv,
  struct expr *theExpression)
  {
   CONSTRAINT_RECORD *rv;

   /*================================================*/
   /* A NULL expression is converted to a constraint */
   /* record with no values allowed.                 */
   /*================================================*/

   if (theExpression == NULL)
     {
      rv = GetConstraintRecord(theEnv);
      rv->anyAllowed = false;
      return(rv);
     }

   /*=============================================================*/
   /* Convert variables and function calls to constraint records. */
   /*=============================================================*/

   if ((theExpression->type == SF_VARIABLE) ||
       (theExpression->type == MF_VARIABLE) ||
#if DEFGENERIC_CONSTRUCT
       (theExpression->type == GCALL) ||
#endif
#if DEFFUNCTION_CONSTRUCT
       (theExpression->type == PCALL) ||
#endif
       (theExpression->type == GBL_VARIABLE) ||
       (theExpression->type == MF_GBL_VARIABLE))
     {
      rv = GetConstraintRecord(theEnv);
      rv->multifieldsAllowed = true;
      return(rv);
     }
   else if (theExpression->type == FCALL)
     { return(FunctionCallToConstraintRecord(theEnv,theExpression->value)); }

   /*============================================*/
   /* Convert a constant to a constraint record. */
   /*============================================*/

   rv = GetConstraintRecord(theEnv);
   rv->anyAllowed = false;

   switch (theExpression->type)
     {
      case FLOAT_TYPE:
        rv->floatRestriction = true;
        rv->floatsAllowed = true;
        break;
        
      case INTEGER_TYPE:
        rv->integerRestriction = true;
        rv->integersAllowed = true;
        break;
        
      case SYMBOL_TYPE:
        rv->symbolRestriction = true;
        rv->symbolsAllowed = true;
        break;
        
      case STRING_TYPE:
        rv->stringRestriction = true;
        rv->stringsAllowed = true;
        break;
        
      case INSTANCE_NAME_TYPE:
      rv->instanceNameRestriction = true;
      rv->instanceNamesAllowed = true;
        break;
        
      case INSTANCE_ADDRESS_TYPE:
        rv->instanceAddressesAllowed = true;
        break;
        
      default:
        break;
     }

   if (rv->floatsAllowed || rv->integersAllowed || rv->symbolsAllowed ||
       rv->stringsAllowed || rv->instanceNamesAllowed)
     { rv->restrictionList = GenConstant(theEnv,theExpression->type,theExpression->value); }

   return rv;
  }

/*******************************************************/
/* FunctionCallToConstraintRecord: Converts a function */
/*   call to a constraint record. For example, the +   */
/*   function when converted would be a constraint     */
/*   record with allowed types INTEGER_TYPE and FLOAT_TYPE.      */
/*******************************************************/
CONSTRAINT_RECORD *FunctionCallToConstraintRecord(
  Environment *theEnv,
  void *theFunction)
  {
   return ArgumentTypeToConstraintRecord(theEnv,UnknownFunctionType(theFunction));
  }

/*********************************************/
/* ArgumentTypeToConstraintRecord2: Uses the */
/*   new argument type codes for 6.4.        */
/*********************************************/
CONSTRAINT_RECORD *ArgumentTypeToConstraintRecord(
  Environment *theEnv,
  unsigned bitTypes)
  {
   CONSTRAINT_RECORD *rv;

   rv = GetConstraintRecord(theEnv);
   rv->anyAllowed = false;

   if (bitTypes & VOID_BIT)
     { rv->voidAllowed = true; }
   if (bitTypes & FLOAT_BIT)
     { rv->floatsAllowed = true; }
   if (bitTypes & INTEGER_BIT)
     { rv->integersAllowed = true; }
   if (bitTypes & SYMBOL_BIT)
     { rv->symbolsAllowed = true; }
   if (bitTypes & STRING_BIT)
     { rv->stringsAllowed = true; }
   if (bitTypes & MULTIFIELD_BIT)
     { rv->multifieldsAllowed = true; }
   if (bitTypes & EXTERNAL_ADDRESS_BIT)
     { rv->externalAddressesAllowed = true; }
   if (bitTypes & FACT_ADDRESS_BIT)
     { rv->factAddressesAllowed = true; }
   if (bitTypes & INSTANCE_ADDRESS_BIT)
     { rv->instanceAddressesAllowed = true; }
   if (bitTypes & INSTANCE_NAME_BIT)
     { rv->instanceNamesAllowed = true; }
   if (bitTypes & BOOLEAN_BIT)
     { rv->symbolsAllowed = true; }

   if (bitTypes == ANY_TYPE_BITS)
     { rv->anyAllowed = true; }

   return(rv);
  }

