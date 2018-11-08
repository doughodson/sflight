   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/18/16             */
   /*                                                     */
   /*             CONSTRAINT CHECKING MODULE              */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides functions for constraint checking of    */
/*   data types.                                             */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian Dantes                                         */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Added allowed-classes slot facet.              */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Dynamic constraint checking for the            */
/*            allowed-classes constraint now searches        */
/*            imported modules.                              */
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

#include "cstrnutl.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "multifld.h"
#include "prntutil.h"
#include "router.h"

#if OBJECT_SYSTEM
#include "classcom.h"
#include "classexm.h"
#include "inscom.h"
#include "insfun.h"
#endif

#include "cstrnchk.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static bool                    CheckRangeAgainstCardinalityConstraint(Environment *,int,int,CONSTRAINT_RECORD *);
   static bool                    CheckFunctionReturnType(unsigned,CONSTRAINT_RECORD *);
   static bool                    CheckTypeConstraint(int,CONSTRAINT_RECORD *);
   static bool                    CheckRangeConstraint(Environment *,int,void *,CONSTRAINT_RECORD *);
   static void                    PrintRange(Environment *,const char *,CONSTRAINT_RECORD *);

/******************************************************/
/* CheckFunctionReturnType: Checks a functions return */
/*   type against a set of permissable return values. */
/*   Returns true if the return type is included      */
/*   among the permissible values, otherwise false.   */
/******************************************************/
static bool CheckFunctionReturnType(
  unsigned functionReturnType,
  CONSTRAINT_RECORD *constraints)
  {
   if (constraints == NULL) return true;

   if (constraints->anyAllowed) return true;

   if (constraints->voidAllowed)
     { if (functionReturnType & VOID_BIT) return true; }

   if (constraints->symbolsAllowed)
     { if (functionReturnType & SYMBOL_BIT) return true; }

   if (constraints->stringsAllowed)
     { if (functionReturnType & STRING_BIT) return true; }

   if (constraints->instanceNamesAllowed)
     { if (functionReturnType & INSTANCE_NAME_BIT) return true; }

   if (constraints->floatsAllowed)
     { if (functionReturnType & FLOAT_BIT) return true; }

   if (constraints->integersAllowed)
     { if (functionReturnType & INTEGER_BIT) return true; }

   if (constraints->multifieldsAllowed)
     { if (functionReturnType & MULTIFIELD_BIT) return true; }

   if (constraints->externalAddressesAllowed)
     { if (functionReturnType & EXTERNAL_ADDRESS_BIT) return true; }

   if (constraints->factAddressesAllowed)
     { if (functionReturnType & FACT_ADDRESS_BIT) return true; }

   if (constraints->instanceAddressesAllowed)
     { if (functionReturnType & INSTANCE_ADDRESS_BIT) return true; }

   return false;
  }

/****************************************************/
/* CheckTypeConstraint: Determines if a primitive   */
/*   data type satisfies the type constraint fields */
/*   of aconstraint record.                         */
/****************************************************/
static bool CheckTypeConstraint(
  int type,
  CONSTRAINT_RECORD *constraints)
  {
   if (type == VOID_TYPE) return false;

   if (constraints == NULL) return true;

   if (constraints->anyAllowed == true) return true;

   if ((type == SYMBOL_TYPE) && (constraints->symbolsAllowed != true))
     { return false; }

   if ((type == STRING_TYPE) && (constraints->stringsAllowed != true))
     { return false; }

   if ((type == FLOAT_TYPE) && (constraints->floatsAllowed != true))
     { return false; }

   if ((type == INTEGER_TYPE) && (constraints->integersAllowed != true))
     { return false; }

#if OBJECT_SYSTEM
   if ((type == INSTANCE_NAME_TYPE) && (constraints->instanceNamesAllowed != true))
     { return false; }

   if ((type == INSTANCE_ADDRESS_TYPE) && (constraints->instanceAddressesAllowed != true))
     { return false; }
#endif

   if ((type == EXTERNAL_ADDRESS_TYPE) && (constraints->externalAddressesAllowed != true))
     { return false; }

   if ((type == VOID_TYPE) && (constraints->voidAllowed != true))
     { return false; }

   if ((type == FACT_ADDRESS_TYPE) && (constraints->factAddressesAllowed != true))
     { return false; }

   return true;
  }

/********************************************************/
/* CheckCardinalityConstraint: Determines if an integer */
/*   falls within the range of allowed cardinalities    */
/*   for a constraint record.                           */
/********************************************************/
bool CheckCardinalityConstraint(
  Environment *theEnv,
  size_t number,
  CONSTRAINT_RECORD *constraints)
  {
   /*=========================================*/
   /* If the constraint record is NULL, there */
   /* are no cardinality restrictions.        */
   /*=========================================*/

   if (constraints == NULL) return true;

   /*==================================*/
   /* Determine if the integer is less */
   /* than the minimum cardinality.    */
   /*==================================*/

   if (constraints->minFields != NULL)
     {
      if (constraints->minFields->value != SymbolData(theEnv)->NegativeInfinity)
        {
         if (number < (size_t) constraints->minFields->integerValue->contents)
           { return false; }
        }
     }

   /*=====================================*/
   /* Determine if the integer is greater */
   /* than the maximum cardinality.       */
   /*=====================================*/

   if (constraints->maxFields != NULL)
     {
      if (constraints->maxFields->value != SymbolData(theEnv)->PositiveInfinity)
        {
         if (number > (size_t) constraints->maxFields->integerValue->contents)
           { return false; }
        }
     }

   /*=========================================================*/
   /* The integer falls within the allowed cardinality range. */
   /*=========================================================*/

   return true;
  }

/*****************************************************************/
/* CheckRangeAgainstCardinalityConstraint: Determines if a range */
/*   of numbers could possibly fall within the range of allowed  */
/*   cardinalities for a constraint record. Returns true if at   */
/*   least one of the numbers in the range is within the allowed */
/*   cardinality, otherwise false is returned.                   */
/*****************************************************************/
static bool CheckRangeAgainstCardinalityConstraint(
  Environment *theEnv,
  int min,
  int max,
  CONSTRAINT_RECORD *constraints)
  {
   /*=========================================*/
   /* If the constraint record is NULL, there */
   /* are no cardinality restrictions.        */
   /*=========================================*/

   if (constraints == NULL) return true;

   /*===============================================================*/
   /* If the minimum value of the range is greater than the maximum */
   /* value of the cardinality, then there are no numbers in the    */
   /* range which could fall within the cardinality range, and so   */
   /* false is returned.                                            */
   /*===============================================================*/

   if (constraints->maxFields != NULL)
     {
      if (constraints->maxFields->value != SymbolData(theEnv)->PositiveInfinity)
        {
         if (min > constraints->maxFields->integerValue->contents)
           { return false; }
        }
     }

   /*===============================================================*/
   /* If the maximum value of the range is less than the minimum    */
   /* value of the cardinality, then there are no numbers in the    */
   /* range which could fall within the cardinality range, and so   */
   /* false is returned. A maximum range value of -1 indicates that */
   /* the maximum possible value of the range is positive infinity. */
   /*===============================================================*/

   if ((constraints->minFields != NULL) && (max != -1))
     {
      if (constraints->minFields->value != SymbolData(theEnv)->NegativeInfinity)
        {
         if (max < constraints->minFields->integerValue->contents)
           { return false; }
        }
     }

   /*=============================================*/
   /* At least one number in the specified range  */
   /* falls within the allowed cardinality range. */
   /*=============================================*/

   return true;
  }

/**********************************************************************/
/* CheckAllowedValuesConstraint: Determines if a primitive data type  */
/*   satisfies the allowed-... constraint fields of a constraint      */
/*   record. Returns true if the constraints are satisfied, otherwise */
/*   false is returned.                                               */
/**********************************************************************/
bool CheckAllowedValuesConstraint(
  int type,
  void *vPtr,
  CONSTRAINT_RECORD *constraints)
  {
   struct expr *tmpPtr;

   /*=========================================*/
   /* If the constraint record is NULL, there */
   /* are no allowed-... restrictions.        */
   /*=========================================*/

   if (constraints == NULL) return true;

   /*=====================================================*/
   /* Determine if there are any allowed-... restrictions */
   /* for the type of the value being checked.            */
   /*=====================================================*/

   switch (type)
     {
      case SYMBOL_TYPE:
        if ((constraints->symbolRestriction == false) &&
            (constraints->anyRestriction == false))
          { return true; }
        break;

#if OBJECT_SYSTEM
      case INSTANCE_NAME_TYPE:
        if ((constraints->instanceNameRestriction == false) &&
            (constraints->anyRestriction == false))
          { return true; }
        break;
#endif

      case STRING_TYPE:
        if ((constraints->stringRestriction == false) &&
            (constraints->anyRestriction == false))
          { return true; }
        break;

      case INTEGER_TYPE:
        if ((constraints->integerRestriction == false) &&
            (constraints->anyRestriction == false))
          { return true; }
        break;

      case FLOAT_TYPE:
        if ((constraints->floatRestriction == false) &&
            (constraints->anyRestriction == false))
          { return true; }
        break;

      default:
        return true;
     }

   /*=========================================================*/
   /* Search through the restriction list to see if the value */
   /* matches one of the allowed values in the list.          */
   /*=========================================================*/

   for (tmpPtr = constraints->restrictionList;
        tmpPtr != NULL;
        tmpPtr = tmpPtr->nextArg)
     {
      if ((tmpPtr->type == type) && (tmpPtr->value == vPtr)) return true;
     }

   /*====================================================*/
   /* If the value wasn't found in the list, then return */
   /* false because the constraint has been violated.    */
   /*====================================================*/

   return false;
  }

/**********************************************************************/
/* CheckAllowedClassesConstraint: Determines if a primitive data type */
/*   satisfies the allowed-classes constraint fields of a constraint  */
/*   record. Returns true if the constraints are satisfied, otherwise */
/*   false is returned.                                               */
/**********************************************************************/
bool CheckAllowedClassesConstraint(
  Environment *theEnv,
  int type,
  void *vPtr,
  CONSTRAINT_RECORD *constraints)
  {
#if OBJECT_SYSTEM
   struct expr *tmpPtr;
   Instance *ins;
   Defclass *insClass, *cmpClass;

   /*=========================================*/
   /* If the constraint record is NULL, there */
   /* is no allowed-classes restriction.      */
   /*=========================================*/

   if (constraints == NULL) return true;

   /*======================================*/
   /* The constraint is satisfied if there */
   /* aren't any class restrictions.       */
   /*======================================*/

   if (constraints->classList == NULL)
     { return true; }

   /*==================================*/
   /* Class restrictions only apply to */
   /* instances and instance names.    */
   /*==================================*/

   if ((type != INSTANCE_ADDRESS_TYPE) && (type != INSTANCE_NAME_TYPE))
     { return true; }

   /*=============================================*/
   /* If an instance name is specified, determine */
   /* whether the instance exists.                */
   /*=============================================*/

   if (type == INSTANCE_ADDRESS_TYPE)
     { ins = (Instance *) vPtr; }
   else
     { ins = FindInstanceBySymbol(theEnv,(CLIPSLexeme *) vPtr); }

   if (ins == NULL)
     { return false; }

   /*======================================================*/
   /* Search through the class list to see if the instance */
   /* belongs to one of the allowed classes in the list.   */
   /*======================================================*/

   insClass = InstanceClass(ins);
   for (tmpPtr = constraints->classList;
        tmpPtr != NULL;
        tmpPtr = tmpPtr->nextArg)
     {
      cmpClass = LookupDefclassByMdlOrScope(theEnv,tmpPtr->lexemeValue->contents);
      if (cmpClass == NULL) continue;
      if (cmpClass == insClass) return true;
      if (SubclassP(insClass,cmpClass)) return true;
     }

   /*=========================================================*/
   /* If a parent class wasn't found in the list, then return */
   /* false because the constraint has been violated.         */
   /*=========================================================*/

   return false;
#else

#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(type)
#pragma unused(vPtr)
#pragma unused(constraints)
#endif

   return true;
#endif
  }

/*************************************************************/
/* CheckRangeConstraint: Determines if a primitive data type */
/*   satisfies the range constraint of a constraint record.  */
/*************************************************************/
static bool CheckRangeConstraint(
  Environment *theEnv,
  int type,
  void *vPtr,
  CONSTRAINT_RECORD *constraints)
  {
   struct expr *minList, *maxList;

   /*===================================*/
   /* If the constraint record is NULL, */
   /* there are no range restrictions.  */
   /*===================================*/

   if (constraints == NULL) return true;

   /*============================================*/
   /* If the value being checked isn't a number, */
   /* then the range restrictions don't apply.   */
   /*============================================*/

   if ((type != INTEGER_TYPE) && (type != FLOAT_TYPE)) return true;

   /*=====================================================*/
   /* Check each of the range restrictions to see if the  */
   /* number falls within at least one of the allowed     */
   /* ranges. If it falls within one of the ranges, then  */
   /* return true since the constraint is satisifed.      */
   /*=====================================================*/

   minList = constraints->minValue;
   maxList = constraints->maxValue;

   while (minList != NULL)
     {
      if (CompareNumbers(theEnv,type,vPtr,minList->type,minList->value) == LESS_THAN)
        {
         minList = minList->nextArg;
         maxList = maxList->nextArg;
        }
      else if (CompareNumbers(theEnv,type,vPtr,maxList->type,maxList->value) == GREATER_THAN)
        {
         minList = minList->nextArg;
         maxList = maxList->nextArg;
        }
      else
        { return true; }
     }

   /*===========================================*/
   /* Return false since the number didn't fall */
   /* within one of the allowed numeric ranges. */
   /*===========================================*/

   return false;
  }

/************************************************/
/* ConstraintViolationErrorMessage: Generalized */
/*   error message for constraint violations.   */
/************************************************/
void ConstraintViolationErrorMessage(
  Environment *theEnv,
  const char *theWhat,
  const char *thePlace,
  bool command,
  unsigned short thePattern,
  CLIPSLexeme *theSlot,
  unsigned short theField,
  int violationType,
  CONSTRAINT_RECORD *theConstraint,
  bool printPrelude)
  {
   /*======================================================*/
   /* Don't print anything other than the tail explanation */
   /* of the error unless asked to do so.                  */
   /*======================================================*/

   if (printPrelude)
     {
      /*===================================*/
      /* Print the name of the thing which */
      /* caused the constraint violation.  */
      /*===================================*/

      if (violationType == FUNCTION_RETURN_TYPE_VIOLATION)
        {
         PrintErrorID(theEnv,"CSTRNCHK",1,true);
         WriteString(theEnv,STDERR,"The function return value");
        }
      else if (theWhat != NULL)
        {
         PrintErrorID(theEnv,"CSTRNCHK",1,true);
         WriteString(theEnv,STDERR,theWhat);
        }

      /*=======================================*/
      /* Print the location of the thing which */
      /* caused the constraint violation.      */
      /*=======================================*/

      if (thePlace != NULL)
        {
         WriteString(theEnv,STDERR," found in ");
         if (command) WriteString(theEnv,STDERR,"the '");
         WriteString(theEnv,STDERR,thePlace);
         if (command) WriteString(theEnv,STDERR,"' command");
        }

      /*================================================*/
      /* If the violation occured in the LHS of a rule, */
      /* indicate which pattern was at fault.           */
      /*================================================*/

      if (thePattern > 0)
        {
         WriteString(theEnv,STDERR," found in CE #");
         WriteInteger(theEnv,STDERR,thePattern);
        }
     }

   /*===============================================================*/
   /* Indicate the type of constraint violation (type, range, etc). */
   /*===============================================================*/

   if ((violationType == TYPE_VIOLATION) ||
       (violationType == FUNCTION_RETURN_TYPE_VIOLATION))
     { WriteString(theEnv,STDERR," does not match the allowed types"); }
   else if (violationType == RANGE_VIOLATION)
     {
      WriteString(theEnv,STDERR," does not fall in the allowed range ");
      PrintRange(theEnv,STDERR,theConstraint);
     }
   else if (violationType == ALLOWED_VALUES_VIOLATION)
     { WriteString(theEnv,STDERR," does not match the allowed values"); }
   else if (violationType == CARDINALITY_VIOLATION)
     { WriteString(theEnv,STDERR," does not satisfy the cardinality restrictions"); }
   else if (violationType == ALLOWED_CLASSES_VIOLATION)
     { WriteString(theEnv,STDERR," does not match the allowed classes"); }

   /*==============================================*/
   /* Print either the slot name or field position */
   /* where the constraint violation occured.      */
   /*==============================================*/

   if (theSlot != NULL)
     {
      WriteString(theEnv,STDERR," for slot '");
      WriteString(theEnv,STDERR,theSlot->contents);
      WriteString(theEnv,STDERR,"'");
     }
   else if (theField > 0)
     {
      WriteString(theEnv,STDERR," for field #");
      WriteInteger(theEnv,STDERR,theField);
     }

   WriteString(theEnv,STDERR,".\n");
  }

/********************************************************************/
/* PrintRange: Prints the range restriction of a constraint record. */
/*   For example, 8 to +00 (eight to positive infinity).            */
/********************************************************************/
static void PrintRange(
  Environment *theEnv,
  const char *logicalName,
  CONSTRAINT_RECORD *theConstraint)
  {
   if (theConstraint->minValue->value == SymbolData(theEnv)->NegativeInfinity)
     { WriteString(theEnv,logicalName,SymbolData(theEnv)->NegativeInfinity->contents); }
   else PrintExpression(theEnv,logicalName,theConstraint->minValue);
   WriteString(theEnv,logicalName," to ");
   if (theConstraint->maxValue->value == SymbolData(theEnv)->PositiveInfinity)
     { WriteString(theEnv,logicalName,SymbolData(theEnv)->PositiveInfinity->contents); }
   else PrintExpression(theEnv,logicalName,theConstraint->maxValue);
  }

/*************************************************************/
/* ConstraintCheckDataObject: Given a value stored in a data */
/*   object structure and a constraint record, determines if */
/*   the data object satisfies the constraint record.        */
/*************************************************************/
ConstraintViolationType ConstraintCheckDataObject(
  Environment *theEnv,
  UDFValue *theData,
  CONSTRAINT_RECORD *theConstraints)
  {
   size_t i; /* 6.04 Bug Fix */
   ConstraintViolationType rv;
   CLIPSValue *theMultifield;

   if (theConstraints == NULL) return NO_VIOLATION;

   if (theData->header->type == MULTIFIELD_TYPE)
     {
      if (CheckCardinalityConstraint(theEnv,theData->range,theConstraints) == false)
        { return CARDINALITY_VIOLATION; }

      theMultifield = theData->multifieldValue->contents;
      for (i = theData->begin; i < theData->begin + theData->range; i++)
        {
         if ((rv = ConstraintCheckValue(theEnv,theMultifield[i].header->type,
                                        theMultifield[i].value,
                                        theConstraints)) != NO_VIOLATION)
           { return rv; }
        }

      return NO_VIOLATION;
     }

   if (CheckCardinalityConstraint(theEnv,1,theConstraints) == false)
    { return CARDINALITY_VIOLATION; }

   return ConstraintCheckValue(theEnv,theData->header->type,theData->value,theConstraints);
  }

/****************************************************************/
/* ConstraintCheckValue: Given a value and a constraint record, */
/*   determines if the value satisfies the constraint record.   */
/****************************************************************/
ConstraintViolationType ConstraintCheckValue(
  Environment *theEnv,
  int theType,
  void *theValue,
  CONSTRAINT_RECORD *theConstraints)
  {
   if (CheckTypeConstraint(theType,theConstraints) == false)
     { return TYPE_VIOLATION; }

   else if (CheckAllowedValuesConstraint(theType,theValue,theConstraints) == false)
     { return ALLOWED_VALUES_VIOLATION; }

   else if (CheckAllowedClassesConstraint(theEnv,theType,theValue,theConstraints) == false)
     { return ALLOWED_CLASSES_VIOLATION; }

   else if (CheckRangeConstraint(theEnv,theType,theValue,theConstraints) == false)
     { return RANGE_VIOLATION; }

   else if (theType == FCALL)
     {
      if (CheckFunctionReturnType(UnknownFunctionType(theValue),theConstraints) == false)
        { return FUNCTION_RETURN_TYPE_VIOLATION; }
     }

   return NO_VIOLATION;
  }

/********************************************************************/
/* ConstraintCheckExpressionChain: Checks an expression and nextArg */
/* links for constraint conflicts (argList is not followed).        */
/********************************************************************/
ConstraintViolationType ConstraintCheckExpressionChain(
  Environment *theEnv,
  struct expr *theExpression,
  CONSTRAINT_RECORD *theConstraints)
  {
   struct expr *theExp;
   int min = 0, max = 0;
   ConstraintViolationType vCode;

   /*===========================================================*/
   /* Determine the minimum and maximum number of value which   */
   /* can be derived from the expression chain (max of -1 means */
   /* positive infinity).                                       */
   /*===========================================================*/

   for (theExp = theExpression ; theExp != NULL ; theExp = theExp->nextArg)
     {
      if (ConstantType(theExp->type)) min++;
      else if (theExp->type == FCALL)
        {
         unsigned restriction = ExpressionUnknownFunctionType(theExp);
         if (restriction & MULTIFIELD_BIT)
           { max = -1; }
         else
           { min++; }
        }
      else max = -1;
     }

   /*====================================*/
   /* Check for a cardinality violation. */
   /*====================================*/

   if (max == 0) max = min;
   if (CheckRangeAgainstCardinalityConstraint(theEnv,min,max,theConstraints) == false)
     { return CARDINALITY_VIOLATION; }

   /*========================================*/
   /* Check for other constraint violations. */
   /*========================================*/

   for (theExp = theExpression ; theExp != NULL ; theExp = theExp->nextArg)
     {
      vCode = ConstraintCheckValue(theEnv,theExp->type,theExp->value,theConstraints);
      if (vCode != NO_VIOLATION)
        return vCode;
     }

   return NO_VIOLATION;
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/***************************************************/
/* ConstraintCheckExpression: Checks an expression */
/*   for constraint conflicts. Returns zero if     */
/*   conflicts are found, otherwise non-zero.      */
/***************************************************/
ConstraintViolationType ConstraintCheckExpression(
  Environment *theEnv,
  struct expr *theExpression,
  CONSTRAINT_RECORD *theConstraints)
  {
   ConstraintViolationType rv = NO_VIOLATION;

   if (theConstraints == NULL) return(rv);

   while (theExpression != NULL)
     {
      rv = ConstraintCheckValue(theEnv,theExpression->type,
                                theExpression->value,
                                theConstraints);
      if (rv != NO_VIOLATION) return rv;
      rv = ConstraintCheckExpression(theEnv,theExpression->argList,theConstraints);
      if (rv != NO_VIOLATION) return rv;
      theExpression = theExpression->nextArg;
     }

   return rv;
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) */

/*****************************************************/
/* UnmatchableConstraint: Determines if a constraint */
/*  record can still be satisfied by some value.     */
/*****************************************************/
bool UnmatchableConstraint(
  CONSTRAINT_RECORD *theConstraint)
  {
   if (theConstraint == NULL) return false;

   if ((! theConstraint->anyAllowed) &&
       (! theConstraint->symbolsAllowed) &&
       (! theConstraint->stringsAllowed) &&
       (! theConstraint->floatsAllowed) &&
       (! theConstraint->integersAllowed) &&
       (! theConstraint->instanceNamesAllowed) &&
       (! theConstraint->instanceAddressesAllowed) &&
       (! theConstraint->multifieldsAllowed) &&
       (! theConstraint->externalAddressesAllowed) &&
       (! theConstraint->voidAllowed) &&
       (! theConstraint->factAddressesAllowed))
     { return true; }

   return false;
  }

