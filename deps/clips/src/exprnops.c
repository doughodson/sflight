   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/18/16             */
   /*                                                     */
   /*             EXPRESSION OPERATIONS MODULE            */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides utility routines for manipulating and   */
/*   examining expressions.                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Add NegateExpression function.                 */
/*                                                           */
/*            Added const qualifiers to remove C++           */
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
/*            Eval support for run time and bload only.      */
/*                                                           */
/*************************************************************/

#include "setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cstrnchk.h"
#include "cstrnops.h"
#include "cstrnutl.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "memalloc.h"
#include "prntutil.h"
#include "router.h"

#include "exprnops.h"

/************************************/
/* CheckArgumentAgainstRestriction: */
/************************************/
bool CheckArgumentAgainstRestriction(
  Environment *theEnv,
  struct expr *theExpression,
  unsigned theRestriction)
  {
   CONSTRAINT_RECORD *cr1, *cr2, *cr3;

   /*=============================================*/
   /* Generate a constraint record for the actual */
   /* argument passed to the function.            */
   /*=============================================*/

   cr1 = ExpressionToConstraintRecord(theEnv,theExpression);

   /*================================================*/
   /* Generate a constraint record based on the type */
   /* of argument expected by the function.          */
   /*================================================*/

   cr2 = ArgumentTypeToConstraintRecord(theEnv,theRestriction);

   /*===============================================*/
   /* Intersect the two constraint records and then */
   /* discard them.                                 */
   /*===============================================*/

   cr3 = IntersectConstraints(theEnv,cr1,cr2);

   RemoveConstraint(theEnv,cr1);
   RemoveConstraint(theEnv,cr2);

   /*====================================================*/
   /* If the intersection of the two constraint records  */
   /* is empty, then the argument passed to the function */
   /* doesn't satisfy the restrictions for the argument. */
   /*====================================================*/

   if (UnmatchableConstraint(cr3))
     {
      RemoveConstraint(theEnv,cr3);
      return true;
     }

   /*===================================================*/
   /* The argument satisfies the function restrictions. */
   /*===================================================*/

   RemoveConstraint(theEnv,cr3);
   return false;
  }

/******************************************************/
/* ConstantExpression: Returns true if the expression */
/*   is a constant, otherwise false.                  */
/******************************************************/
bool ConstantExpression(
  struct expr *testPtr)
  {
   while (testPtr != NULL)
     {
      if ((testPtr->type != SYMBOL_TYPE) && (testPtr->type != STRING_TYPE) &&
#if OBJECT_SYSTEM
          (testPtr->type != INSTANCE_NAME_TYPE) && (testPtr->type != INSTANCE_ADDRESS_TYPE) &&
#endif
          (testPtr->type != INTEGER_TYPE) && (testPtr->type != FLOAT_TYPE))
        { return false; }
      testPtr = testPtr->nextArg;
     }

   return true;
  }

/******************************************/
/* ConstantType: Returns true if the type */
/*   is a constant, otherwise false.      */
/******************************************/
bool ConstantType(
  int theType)
  {
   switch (theType)
     {
      case SYMBOL_TYPE:
      case STRING_TYPE:
      case INTEGER_TYPE:
      case FLOAT_TYPE:
#if OBJECT_SYSTEM
      case INSTANCE_NAME_TYPE:
      case INSTANCE_ADDRESS_TYPE:
#endif
        return true;
     }

   return false;
  }

/*****************************************************************************/
/* IdenticalExpression: Determines if two expressions are identical. Returns */
/*   true if the expressions are identical, otherwise false is returned.     */
/*****************************************************************************/
bool IdenticalExpression(
  struct expr *firstList,
  struct expr *secondList)
  {
   /*==============================================*/
   /* Compare each argument in both expressions by */
   /* following the nextArg list.                  */
   /*==============================================*/

   for (;
        (firstList != NULL) && (secondList != NULL);
        firstList = firstList->nextArg, secondList = secondList->nextArg)
     {
      /*=========================*/
      /* Compare type and value. */
      /*=========================*/

      if (firstList->type != secondList->type)
        { return false; }

      if (firstList->value != secondList->value)
        { return false; }

      /*==============================*/
      /* Compare the arguments lists. */
      /*==============================*/

      if (IdenticalExpression(firstList->argList,secondList->argList) == false)
        { return false; }
     }

   /*=====================================================*/
   /* If firstList and secondList aren't both NULL, then  */
   /* one of the lists contains more expressions than the */
   /* other.                                              */
   /*=====================================================*/

   if (firstList != secondList) return false;

   /*============================*/
   /* Expressions are identical. */
   /*============================*/

   return true;
  }

/****************************************************/
/* CountArguments: Returns the number of structures */
/*   stored in an expression as traversed through   */
/*   the nextArg pointer but not the argList        */
/*   pointer.                                       */
/****************************************************/
unsigned short CountArguments(
  struct expr *testPtr)
  {
   unsigned short size = 0;

   while (testPtr != NULL)
     {
      size++;
      testPtr = testPtr->nextArg;
     }

   return size;
  }

/******************************************/
/* CopyExpresssion: Copies an expression. */
/******************************************/
struct expr *CopyExpression(
  Environment *theEnv,
  struct expr *original)
  {
   struct expr *topLevel, *next, *last;

   if (original == NULL) return NULL;

   topLevel = GenConstant(theEnv,original->type,original->value);
   topLevel->argList = CopyExpression(theEnv,original->argList);

   last = topLevel;
   original = original->nextArg;
   while (original != NULL)
     {
      next = GenConstant(theEnv,original->type,original->value);
      next->argList = CopyExpression(theEnv,original->argList);

      last->nextArg = next;
      last = next;
      original = original->nextArg;
     }

   return(topLevel);
  }

/************************************************************/
/* ExpressionContainsVariables: Determines if an expression */
/*   contains any variables. Returns true if the expression */
/*   contains any variables, otherwise false is returned.   */
/************************************************************/
bool ExpressionContainsVariables(
  struct expr *theExpression,
  bool globalsAreVariables)
  {
   while (theExpression != NULL)
     {
      if (theExpression->argList != NULL)
        {
         if (ExpressionContainsVariables(theExpression->argList,globalsAreVariables))
           { return true; }
        }

      if ((theExpression->type == MF_VARIABLE) ||
          (theExpression->type == SF_VARIABLE) ||
          (theExpression->type == FACT_ADDRESS_TYPE) ||
          (((theExpression->type == GBL_VARIABLE) ||
            (theExpression->type == MF_GBL_VARIABLE)) &&
           (globalsAreVariables == true)))
        { return true; }

      theExpression = theExpression->nextArg;
     }

   return false;
  }

/*****************************************/
/* ExpressionSize: Returns the number of */
/*   structures stored in an expression. */
/*****************************************/
unsigned long ExpressionSize(
  struct expr *testPtr)
  {
   unsigned long size = 0;

   while (testPtr != NULL)
     {
      size++;
      if (testPtr->argList != NULL)
        { size += ExpressionSize(testPtr->argList); }
      testPtr = testPtr->nextArg;
     }
   return size;
  }

/************************************************/
/* GenConstant: Generates a constant expression */
/*   value of type string, symbol, or number.   */
/************************************************/
struct expr *GenConstant(
  Environment *theEnv,
  unsigned short type,
  void *value)
  {
   struct expr *top;

   top = get_struct(theEnv,expr);
   top->nextArg = NULL;
   top->argList = NULL;
   top->type = type;
   top->value = value;

   return top;
  }

/*************************************************/
/* PrintExpression: Pretty prints an expression. */
/*************************************************/
void PrintExpression(
  Environment *theEnv,
  const char *fileid,
  struct expr *theExpression)
  {
   struct expr *oldExpression;

   if (theExpression == NULL)
     { return; }

   while (theExpression != NULL)
     {
      switch (theExpression->type)
        {
         case SF_VARIABLE:
         case GBL_VARIABLE:
            WriteString(theEnv,fileid,"?");
            WriteString(theEnv,fileid,theExpression->lexemeValue->contents);
            break;

         case MF_VARIABLE:
         case MF_GBL_VARIABLE:
            WriteString(theEnv,fileid,"$?");
            WriteString(theEnv,fileid,theExpression->lexemeValue->contents);
            break;

         case FCALL:
           WriteString(theEnv,fileid,"(");
           WriteString(theEnv,fileid,ExpressionFunctionCallName(theExpression)->contents);
           if (theExpression->argList != NULL) { WriteString(theEnv,fileid," "); }
           PrintExpression(theEnv,fileid,theExpression->argList);
           WriteString(theEnv,fileid,")");
           break;

         default:
           oldExpression = EvaluationData(theEnv)->CurrentExpression;
           EvaluationData(theEnv)->CurrentExpression = theExpression;
           PrintAtom(theEnv,fileid,theExpression->type,theExpression->value);
           EvaluationData(theEnv)->CurrentExpression = oldExpression;
           break;
        }

      theExpression = theExpression->nextArg;
      if (theExpression != NULL) WriteString(theEnv,fileid," ");
     }

   return;
  }

/*************************************************************************/
/* CombineExpressions: Combines two expressions into a single equivalent */
/*   expression. Mainly serves to merge expressions containing "and"     */
/*   and "or" expressions without unnecessary duplication of the "and"   */
/*   and "or" expressions (i.e., two "and" expressions can be merged by  */
/*   placing them as arguments within another "and" expression, but it   */
/*   is more efficient to add the arguments of one of the "and"          */
/*   expressions to the list of arguments for the other and expression). */
/*************************************************************************/
struct expr *CombineExpressions(
  Environment *theEnv,
  struct expr *expr1,
  struct expr *expr2)
  {
   struct expr *tempPtr;

   /*===========================================================*/
   /* If the 1st expression is NULL, return the 2nd expression. */
   /*===========================================================*/

   if (expr1 == NULL) return(expr2);

   /*===========================================================*/
   /* If the 2nd expression is NULL, return the 1st expression. */
   /*===========================================================*/

   if (expr2 == NULL) return(expr1);

   /*============================================================*/
   /* If the 1st expression is an "and" expression, and the 2nd  */
   /* expression is not an "and" expression, then include the    */
   /* 2nd expression in the argument list of the 1st expression. */
   /*============================================================*/

   if ((expr1->value == ExpressionData(theEnv)->PTR_AND) &&
       (expr2->value != ExpressionData(theEnv)->PTR_AND))
     {
      tempPtr = expr1->argList;
      if (tempPtr == NULL)
        {
         rtn_struct(theEnv,expr,expr1);
         return(expr2);
        }

      while (tempPtr->nextArg != NULL)
        { tempPtr = tempPtr->nextArg; }

      tempPtr->nextArg = expr2;
      return(expr1);
     }

   /*============================================================*/
   /* If the 2nd expression is an "and" expression, and the 1st  */
   /* expression is not an "and" expression, then include the    */
   /* 1st expression in the argument list of the 2nd expression. */
   /*============================================================*/

   if ((expr1->value != ExpressionData(theEnv)->PTR_AND) &&
       (expr2->value == ExpressionData(theEnv)->PTR_AND))
     {
      tempPtr = expr2->argList;
      if (tempPtr == NULL)
        {
         rtn_struct(theEnv,expr,expr2);
         return(expr1);
        }

      expr2->argList = expr1;
      expr1->nextArg = tempPtr;

      return(expr2);
     }

   /*===========================================================*/
   /* If both expressions are "and" expressions, then add the   */
   /* 2nd expression to the argument list of the 1st expression */
   /* and throw away the extraneous "and" expression.           */
   /*===========================================================*/

   if ((expr1->value == ExpressionData(theEnv)->PTR_AND) &&
       (expr2->value == ExpressionData(theEnv)->PTR_AND))
     {
      tempPtr = expr1->argList;
      if (tempPtr == NULL)
        {
         rtn_struct(theEnv,expr,expr1);
         return(expr2);
        }

      while (tempPtr->nextArg != NULL)
        { tempPtr = tempPtr->nextArg; }

      tempPtr->nextArg = expr2->argList;
      rtn_struct(theEnv,expr,expr2);

      return(expr1);
     }

   /*=====================================================*/
   /* If neither expression is an "and" expression, then  */
   /* create an "and" expression and add both expressions */
   /* to the argument list of that "and" expression.      */
   /*=====================================================*/

   tempPtr = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_AND);
   tempPtr->argList = expr1;
   expr1->nextArg = expr2;
   return(tempPtr);
  }

/*********************/
/* NegateExpression: */
/*********************/
struct expr *NegateExpression(
  Environment *theEnv,
  struct expr *theExpression)
  {
   struct expr *tempPtr;

   /*=========================================*/
   /* If the expression is NULL, return NULL. */
   /*=========================================*/

   if (theExpression == NULL) return NULL;

   /*==================================================*/
   /* The expression is already wrapped within a "not" */
   /* function call, just remove the function call.    */
   /*==================================================*/

   if (theExpression->value == ExpressionData(theEnv)->PTR_NOT)
     {
      tempPtr = theExpression->argList;
      rtn_struct(theEnv,expr,theExpression);
      return(tempPtr);
     }

   /*===================================================*/
   /* Wrap the expression within a "not" function call. */
   /*===================================================*/

   tempPtr = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NOT);
   tempPtr->argList = theExpression;

   return(tempPtr);
  }

/********************************************************/
/* AppendExpressions: Attaches an expression to the end */
/*   of another expression's nextArg list.              */
/********************************************************/
struct expr *AppendExpressions(
  struct expr *expr1,
  struct expr *expr2)
  {
   struct expr *tempPtr;

   /*===========================================================*/
   /* If the 1st expression is NULL, return the 2nd expression. */
   /*===========================================================*/

   if (expr1 == NULL) return(expr2);

   /*===========================================================*/
   /* If the 2nd expression is NULL, return the 1st expression. */
   /*===========================================================*/

   if (expr2 == NULL) return(expr1);

   /*====================================*/
   /* Find the end of the 1st expression */
   /* and attach the 2nd expression.     */
   /*====================================*/

   tempPtr = expr1;
   while (tempPtr->nextArg != NULL) tempPtr = tempPtr->nextArg;
   tempPtr->nextArg = expr2;

   /*===============================*/
   /* Return the merged expression. */
   /*===============================*/

   return(expr1);
  }

