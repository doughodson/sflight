   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  08/25/16             */
   /*                                                     */
   /*              RULE CONSTRAINTS MODULE                */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines for detecting constraint       */
/*   conflicts in the LHS and RHS of rules.                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
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
/*            Static constraint checking is always enabled.  */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if (! RUN_TIME) && (! BLOAD_ONLY) && DEFRULE_CONSTRUCT

#include <stdio.h>

#include "analysis.h"
#include "cstrnchk.h"
#include "cstrnops.h"
#include "cstrnutl.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "prcdrpsr.h"
#include "prntutil.h"
#include "reorder.h"
#include "router.h"
#include "rulepsr.h"

#include "rulecstr.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static bool                    CheckForUnmatchableConstraints(Environment *,struct lhsParseNode *,unsigned short);
   static bool                    MultifieldCardinalityViolation(Environment *,struct lhsParseNode *);
   static struct lhsParseNode    *UnionVariableConstraints(Environment *,struct lhsParseNode *,
                                                           struct lhsParseNode *);
   static struct lhsParseNode    *AddToVariableConstraints(Environment *,struct lhsParseNode *,
                                                           struct lhsParseNode *);
   static void                    ConstraintConflictMessage(Environment *,CLIPSLexeme *,
                                                            unsigned short,unsigned short,CLIPSLexeme *);
   static bool                    CheckArgumentForConstraintError(Environment *,struct expr *,struct expr*,
                                                                  unsigned int,struct functionDefinition *,
                                                                  struct lhsParseNode *);

/***********************************************************/
/* CheckForUnmatchableConstraints: Determines if a LHS CE  */
/*   node contains unmatchable constraints. Return true if */
/*   there are unmatchable constraints, otherwise false.   */
/***********************************************************/
static bool CheckForUnmatchableConstraints(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  unsigned short whichCE)
  {
   if (UnmatchableConstraint(theNode->constraints))
     {
      ConstraintConflictMessage(theEnv,theNode->lexemeValue,whichCE,
                                theNode->index,theNode->slot);
      return true;
     }

   return false;
  }

/******************************************************/
/* ConstraintConflictMessage: Error message used when */
/*   a constraint restriction for a slot prevents any */
/*   value from matching the pattern constraint.      */
/******************************************************/
static void ConstraintConflictMessage(
  Environment *theEnv,
  CLIPSLexeme *variableName,
  unsigned short thePattern,
  unsigned short theField,
  CLIPSLexeme *theSlot)
  {
   /*=========================*/
   /* Print the error header. */
   /*=========================*/

   PrintErrorID(theEnv,"RULECSTR",1,true);

   /*======================================================*/
   /* Print the variable name (if available) and CE number */
   /* for which the constraint violation occurred.         */
   /*======================================================*/

   if (variableName != NULL)
     {
      WriteString(theEnv,STDERR,"Variable ?");
      WriteString(theEnv,STDERR,variableName->contents);
      WriteString(theEnv,STDERR," in CE #");
      WriteInteger(theEnv,STDERR,thePattern);
     }
   else
     {
      WriteString(theEnv,STDERR,"Pattern #");
      WriteInteger(theEnv,STDERR,thePattern);
     }

   /*=======================================*/
   /* Print the slot name or field position */
   /* in which the violation occurred.      */
   /*=======================================*/

   if (theSlot == NULL)
     {
      WriteString(theEnv,STDERR," field #");
      WriteInteger(theEnv,STDERR,theField);
     }
   else
     {
      WriteString(theEnv,STDERR," slot '");
      WriteString(theEnv,STDERR,theSlot->contents);
      WriteString(theEnv,STDERR,"'");
     }

   /*======================================*/
   /* Print the rest of the error message. */
   /*======================================*/

   WriteString(theEnv,STDERR," has constraint conflicts which make the pattern unmatchable.\n");
  }

/***************************************************************/
/* MultifieldCardinalityViolation: Determines if a cardinality */
/*   violation has occurred for a LHS CE node.                 */
/***************************************************************/
static bool MultifieldCardinalityViolation(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct lhsParseNode *tmpNode;
   struct expr *tmpMax;
   long long minFields = 0;
   long long maxFields = 0;
   bool posInfinity = false;
   CONSTRAINT_RECORD *newConstraint, *tempConstraint;

   /*================================*/
   /* A single field slot can't have */
   /* a cardinality violation.       */
   /*================================*/

   if (theNode->multifieldSlot == false) return false;

   /*=============================================*/
   /* Determine the minimum and maximum number of */
   /* fields the slot could contain based on the  */
   /* slot constraints found in the pattern.      */
   /*=============================================*/

   for (tmpNode = theNode->bottom;
        tmpNode != NULL;
        tmpNode = tmpNode->right)
     {
      /*====================================================*/
      /* A single field variable increases both the minimum */
      /* and maximum number of fields by one.               */
      /*====================================================*/

      if ((tmpNode->pnType == SF_VARIABLE_NODE) ||
          (tmpNode->pnType == SF_WILDCARD_NODE))
        {
         minFields++;
         maxFields++;
        }

      /*=================================================*/
      /* Otherwise a multifield wildcard or variable has */
      /* been encountered. If it is constrained then use */
      /* minimum and maximum number of fields constraint */
      /* associated with this LHS node.                  */
      /*=================================================*/

      else if (tmpNode->constraints != NULL)
        {
         /*=======================================*/
         /* The lowest minimum of all the min/max */
         /* pairs will be the first in the list.  */
         /*=======================================*/

         if (tmpNode->constraints->minFields->value != SymbolData(theEnv)->NegativeInfinity)
           { minFields += tmpNode->constraints->minFields->integerValue->contents; }

         /*=========================================*/
         /* The greatest maximum of all the min/max */
         /* pairs will be the last in the list.     */
         /*=========================================*/

         tmpMax = tmpNode->constraints->maxFields;
         while (tmpMax->nextArg != NULL) tmpMax = tmpMax->nextArg;
         if (tmpMax->value == SymbolData(theEnv)->PositiveInfinity)
           { posInfinity = true; }
         else
           { maxFields += tmpMax->integerValue->contents; }
        }

      /*================================================*/
      /* Otherwise an unconstrained multifield wildcard */
      /* or variable increases the maximum number of    */
      /* fields to positive infinity.                   */
      /*================================================*/

      else
        { posInfinity = true; }
     }

   /*==================================================================*/
   /* Create a constraint record for the cardinality of the sum of the */
   /* cardinalities of the restrictions inside the multifield slot.    */
   /*==================================================================*/

   if (theNode->constraints == NULL) tempConstraint = GetConstraintRecord(theEnv);
   else tempConstraint = CopyConstraintRecord(theEnv,theNode->constraints);
   ReturnExpression(theEnv,tempConstraint->minFields);
   ReturnExpression(theEnv,tempConstraint->maxFields);
   tempConstraint->minFields = GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,minFields));
   if (posInfinity) tempConstraint->maxFields = GenConstant(theEnv,SYMBOL_TYPE,SymbolData(theEnv)->PositiveInfinity);
   else tempConstraint->maxFields = GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,maxFields));

   /*================================================================*/
   /* Determine the final cardinality for the multifield slot by     */
   /* intersecting the cardinality sum of the restrictions within    */
   /* the multifield slot with the original cardinality of the slot. */
   /*================================================================*/

   newConstraint = IntersectConstraints(theEnv,theNode->constraints,tempConstraint);
   if (theNode->derivedConstraints) RemoveConstraint(theEnv,theNode->constraints);
   RemoveConstraint(theEnv,tempConstraint);
   theNode->constraints = newConstraint;
   theNode->derivedConstraints = true;

   /*===================================================================*/
   /* Determine if the final cardinality for the slot can be satisfied. */
   /*===================================================================*/

   if (UnmatchableConstraint(newConstraint)) return true;

   return false;
  }

/***************************************************/
/* ProcessConnectedConstraints: Examines a single  */
/*   connected constraint searching for constraint */
/*   violations.                                   */
/***************************************************/
bool ProcessConnectedConstraints(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  struct lhsParseNode *multifieldHeader,
  struct lhsParseNode *patternHead)
  {
   struct constraintRecord *orConstraints = NULL, *andConstraints;
   struct constraintRecord *tmpConstraints, *rvConstraints;
   struct lhsParseNode *orNode, *andNode;
   struct expr *tmpExpr;

   /*============================================*/
   /* Loop through all of the or (|) constraints */
   /* found in the connected constraint.         */
   /*============================================*/

   for (orNode = theNode->bottom; orNode != NULL; orNode = orNode->bottom)
     {
      /*=================================================*/
      /* Intersect all of the &'ed constraints together. */
      /*=================================================*/

      andConstraints = NULL;
      for (andNode = orNode; andNode != NULL; andNode = andNode->right)
        {
         if (! andNode->negated)
           {
            if (andNode->pnType == RETURN_VALUE_CONSTRAINT_NODE)
              {
               if (andNode->expression->pnType == FCALL_NODE)
                 {
                  rvConstraints = FunctionCallToConstraintRecord(theEnv,andNode->expression->value);
                  tmpConstraints = andConstraints;
                  andConstraints = IntersectConstraints(theEnv,andConstraints,rvConstraints);
                  RemoveConstraint(theEnv,tmpConstraints);
                  RemoveConstraint(theEnv,rvConstraints);
                 }
              }
            else if (ConstantNode(andNode))
              {
               tmpExpr = GenConstant(theEnv,NodeTypeToType(andNode),andNode->value);
               rvConstraints = ExpressionToConstraintRecord(theEnv,tmpExpr);
               tmpConstraints = andConstraints;
               andConstraints = IntersectConstraints(theEnv,andConstraints,rvConstraints);
               RemoveConstraint(theEnv,tmpConstraints);
               RemoveConstraint(theEnv,rvConstraints);
               ReturnExpression(theEnv,tmpExpr);
              }
            else if (andNode->constraints != NULL)
              {
               tmpConstraints = andConstraints;
               andConstraints = IntersectConstraints(theEnv,andConstraints,andNode->constraints);
               RemoveConstraint(theEnv,tmpConstraints);
              }
           }
        }

      /*===========================================================*/
      /* Intersect the &'ed constraints with the slot constraints. */
      /*===========================================================*/

      tmpConstraints = andConstraints;
      andConstraints = IntersectConstraints(theEnv,andConstraints,theNode->constraints);
      RemoveConstraint(theEnv,tmpConstraints);

      /*===============================================================*/
      /* Remove any negated constants from the list of allowed values. */
      /*===============================================================*/

      for (andNode = orNode; andNode != NULL; andNode = andNode->right)
        {
         if ((andNode->negated) && ConstantNode(andNode))
           { RemoveConstantFromConstraint(theEnv,NodeTypeToType(andNode),andNode->value,andConstraints); }
        }

      /*=======================================================*/
      /* Union the &'ed constraints with the |'ed constraints. */
      /*=======================================================*/

      tmpConstraints = orConstraints;
      orConstraints = UnionConstraints(theEnv,orConstraints,andConstraints);
      RemoveConstraint(theEnv,tmpConstraints);
      RemoveConstraint(theEnv,andConstraints);
     }

   /*===============================================*/
   /* Replace the constraints for the slot with the */
   /* constraints derived from the connected        */
   /* constraints (which should be a subset.        */
   /*===============================================*/

   if (orConstraints != NULL)
     {
      if (theNode->derivedConstraints) RemoveConstraint(theEnv,theNode->constraints);
      theNode->constraints = orConstraints;
      theNode->derivedConstraints = true;
     }

   /*==================================*/
   /* Check for constraint violations. */
   /*==================================*/

   if (CheckForUnmatchableConstraints(theEnv,theNode,patternHead->whichCE))
     { return true; }

   /*=========================================*/
   /* If the constraints are for a multifield */
   /* slot, check for cardinality violations. */
   /*=========================================*/

   if ((multifieldHeader != NULL) && (theNode->right == NULL))
     {
      if (MultifieldCardinalityViolation(theEnv,multifieldHeader))
        {
         ConstraintViolationErrorMessage(theEnv,"The group of restrictions",
                                                  NULL,false,
                                                  patternHead->whichCE,
                                                  multifieldHeader->slot,
                                                  multifieldHeader->index,
                                                  CARDINALITY_VIOLATION,
                                                  multifieldHeader->constraints,true);
          return true;
         }
      }

   /*=======================================*/
   /* Return false indicating no constraint */
   /* violations were detected.             */
   /*=======================================*/

   return false;
  }

/**************************************************/
/* ConstraintReferenceErrorMessage: Generic error */
/*   message for LHS constraint violation errors  */
/*   that occur within an expression.             */
/**************************************************/
void ConstraintReferenceErrorMessage(
  Environment *theEnv,
  CLIPSLexeme *theVariable,
  struct lhsParseNode *theExpression,
  int whichArgument,
  int whichCE,
  CLIPSLexeme *slotName,
  int theField)
  {
   struct expr *temprv;

   PrintErrorID(theEnv,"RULECSTR",2,true);

   /*==========================*/
   /* Print the variable name. */
   /*==========================*/

   WriteString(theEnv,STDERR,"Previous variable bindings of ?");
   WriteString(theEnv,STDERR,theVariable->contents);
   WriteString(theEnv,STDERR," caused the type restrictions");

   /*============================*/
   /* Print the argument number. */
   /*============================*/

   WriteString(theEnv,STDERR,"\nfor argument #");
   WriteInteger(theEnv,STDERR,whichArgument);

   /*=======================*/
   /* Print the expression. */
   /*=======================*/

   WriteString(theEnv,STDERR," of the expression ");
   temprv = LHSParseNodesToExpression(theEnv,theExpression);
   ReturnExpression(theEnv,temprv->nextArg);
   temprv->nextArg = NULL;
   PrintExpression(theEnv,STDERR,temprv);
   WriteString(theEnv,STDERR,"\n");
   ReturnExpression(theEnv,temprv);

   /*========================================*/
   /* Print out the index of the conditional */
   /* element and the slot name or field     */
   /* index where the violation occured.     */
   /*========================================*/

   WriteString(theEnv,STDERR,"found in CE #");
   WriteInteger(theEnv,STDERR,theExpression->whichCE);
   if (slotName == NULL)
     {
      if (theField > 0)
        {
         WriteString(theEnv,STDERR," field #");
         WriteInteger(theEnv,STDERR,theField);
        }
     }
   else
     {
      WriteString(theEnv,STDERR," slot '");
      WriteString(theEnv,STDERR,slotName->contents);
      WriteString(theEnv,STDERR,"'");
     }

   WriteString(theEnv,STDERR," to be violated.\n");
  }

/********************************************************/
/* AddToVariableConstraints: Adds the constraints for a */
/*   variable to a list of constraints. If the variable */
/*   is already in the list, the constraints for the    */
/*   variable are intersected with the new constraints. */
/********************************************************/
static struct lhsParseNode *AddToVariableConstraints(
  Environment *theEnv,
  struct lhsParseNode *oldList,
  struct lhsParseNode *newItems)
  {
   CONSTRAINT_RECORD *newConstraints;
   struct lhsParseNode *temp, *trace;

   /*=================================================*/
   /* Loop through each of the new constraints adding */
   /* it to the list if it's not already present or   */
   /* modifying the constraint if it is.              */
   /*=================================================*/

   while (newItems != NULL)
     {
      /*==========================================*/
      /* Get the next item since the next pointer */
      /* value (right) needs to be set to NULL.   */
      /*==========================================*/

      temp = newItems->right;
      newItems->right = NULL;

      /*===================================*/
      /* Search the list for the variable. */
      /*===================================*/

      for (trace = oldList; trace != NULL; trace = trace->right)
        {
         /*=========================================*/
         /* If the variable is already in the list, */
         /* modify the constraint already there to  */
         /* include the new constraint.             */
         /*=========================================*/

         if (trace->value == newItems->value)
           {
            newConstraints = IntersectConstraints(theEnv,trace->constraints,
                                                  newItems->constraints);
            RemoveConstraint(theEnv,trace->constraints);
            trace->constraints = newConstraints;
            ReturnLHSParseNodes(theEnv,newItems);
            break;
           }
        }

      /*=================================*/
      /* Add the variable constraints to */
      /* the list if it wasn't found.    */
      /*=================================*/

      if (trace == NULL)
        {
         newItems->right = oldList;
         oldList = newItems;
        }

      /*===========================*/
      /* Move on to the next item. */
      /*===========================*/

      newItems = temp;
     }

   return(oldList);
  }

/***********************************************************/
/* UnionVariableConstraints: Unions two lists of variable  */
/*   constraints. If a variable appears in one list  but   */
/*   not the other, then the variable is unconstrained and */
/*   thus not included in the unioned list.                */
/***********************************************************/
static struct lhsParseNode *UnionVariableConstraints(
  Environment *theEnv,
  struct lhsParseNode *list1,
  struct lhsParseNode *list2)
  {
   struct lhsParseNode *list3 = NULL, *trace, *temp;

   /*===================================*/
   /* Loop through all of the variables */
   /* in the first list.                */
   /*===================================*/

   while (list1 != NULL)
     {
      /*=============================================*/
      /* Search for the variable in the second list. */
      /*=============================================*/

      for (trace = list2; trace != NULL; trace = trace->right)
        {
         /*============================================*/
         /* If the variable is found in both lists,    */
         /* union the constraints and add the variable */
         /* to the new list being constructed.         */
         /*============================================*/

         if (list1->value == trace->value)
           {
            temp = GetLHSParseNode(theEnv);
            temp->derivedConstraints = true;
            temp->value = list1->value;
            temp->constraints = UnionConstraints(theEnv,list1->constraints,trace->constraints);
            temp->right = list3;
            list3 = temp;
            break;
           }
        }

      /*==============================*/
      /* Move on to the next variable */
      /* in the first list.           */
      /*==============================*/

      temp = list1->right;
      list1->right = NULL;
      ReturnLHSParseNodes(theEnv,list1);
      list1 = temp;
     }

   /*====================================*/
   /* Free the items in the second list. */
   /*====================================*/

   ReturnLHSParseNodes(theEnv,list2);

   /*======================*/
   /* Return the new list. */
   /*======================*/

   return(list3);
  }

/*****************************************************************/
/* GetExpressionVarConstraints: Given an expression stored using */
/*   the LHS parse node data structures, determines and returns  */
/*   the constraints on variables caused by that expression. For */
/*   example, the expression (+ ?x 1) would imply a numeric type */
/*   constraint for the variable ?x since the addition function  */
/*   expects numeric arguments.                                  */
/*****************************************************************/
struct lhsParseNode *GetExpressionVarConstraints(
  Environment *theEnv,
  struct lhsParseNode *theExpression)
  {
   struct lhsParseNode *list1 = NULL, *list2;

   for (; theExpression != NULL; theExpression = theExpression->bottom)
     {
      if (theExpression->right != NULL)
        {
         list2 = GetExpressionVarConstraints(theEnv,theExpression->right);
         list1 = AddToVariableConstraints(theEnv,list2,list1);
        }

      if (theExpression->pnType == SF_VARIABLE_NODE)
        {
         list2 = GetLHSParseNode(theEnv);
         if (theExpression->referringNode != NULL)
           { list2->pnType = theExpression->referringNode->pnType; }
         else
           { list2->pnType = SF_VARIABLE_NODE; }
         list2->value = theExpression->value;
         list2->derivedConstraints = true;
         list2->constraints = CopyConstraintRecord(theEnv,theExpression->constraints);
         list1 = AddToVariableConstraints(theEnv,list2,list1);
        }
     }

   return(list1);
  }

/***********************************************/
/* DeriveVariableConstraints: Derives the list */
/*   of variable constraints associated with a */
/*   single connected constraint.              */
/***********************************************/
struct lhsParseNode *DeriveVariableConstraints(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct lhsParseNode *orNode, *andNode;
   struct lhsParseNode *list1, *list2, *list3 = NULL;
   bool first = true;

   /*===============================*/
   /* Process the constraints for a */
   /* single connected constraint.  */
   /*===============================*/

   for (orNode = theNode->bottom; orNode != NULL; orNode = orNode->bottom)
     {
      /*=================================================*/
      /* Intersect all of the &'ed constraints together. */
      /*=================================================*/

      list2 = NULL;
      for (andNode = orNode; andNode != NULL; andNode = andNode->right)
        {
         if ((andNode->pnType == RETURN_VALUE_CONSTRAINT_NODE) ||
             (andNode->pnType == PREDICATE_CONSTRAINT_NODE))
           {
            list1 = GetExpressionVarConstraints(theEnv,andNode->expression);
            list2 = AddToVariableConstraints(theEnv,list2,list1);
           }
        }

      if (first)
        {
         list3 = list2;
         first = false;
        }
      else
        { list3 = UnionVariableConstraints(theEnv,list3,list2); }
     }

   return(list3);
  }

/*******************************************/
/* CheckRHSForConstraintErrors: Checks the */
/*   RHS of a rule for constraint errors.  */
/*******************************************/
bool CheckRHSForConstraintErrors(
  Environment *theEnv,
  struct expr *expressionList,
  struct lhsParseNode *theLHS)
  {
   struct functionDefinition *theFunction;
   unsigned int i;
   struct expr *lastOne = NULL, *checkList, *tmpPtr;

   if (expressionList == NULL) return false;

   for (checkList = expressionList;
        checkList != NULL;
        checkList = checkList->nextArg)
      {
       expressionList = checkList->argList;
       i = 1;
       if (checkList->type == FCALL)
         {
          lastOne = checkList;
          theFunction = checkList->functionValue;
         }
       else
         { theFunction = NULL; }

       while (expressionList != NULL)
         {
          if (CheckArgumentForConstraintError(theEnv,expressionList,lastOne,i,
                                              theFunction,theLHS))
            { return true; }

          i++;
          tmpPtr = expressionList->nextArg;
          expressionList->nextArg = NULL;
          if (CheckRHSForConstraintErrors(theEnv,expressionList,theLHS))
            {
             expressionList->nextArg = tmpPtr;
             return true;
            }
          expressionList->nextArg = tmpPtr;
          expressionList = expressionList->nextArg;
         }
      }

   return false;
  }

/*************************************************************/
/* CheckArgumentForConstraintError: Checks a single argument */
/*   found in the RHS of a rule for constraint errors.       */
/*   Returns true if an error is detected, otherwise false.  */
/*************************************************************/
static bool CheckArgumentForConstraintError(
  Environment *theEnv,
  struct expr *expressionList,
  struct expr *lastOne,
  unsigned int i,
  struct functionDefinition *theFunction,
  struct lhsParseNode *theLHS)
  {
   unsigned theRestriction2;
   CONSTRAINT_RECORD *constraint1, *constraint2, *constraint3, *constraint4;
   struct lhsParseNode *theVariable;
   struct expr *tmpPtr;
   bool rv = false;

   /*=============================================================*/
   /* Skip anything that isn't a variable or isn't an argument to */
   /* a user defined function (i.e. deffunctions and generic have */
   /* no constraint information so they aren't checked).          */
   /*=============================================================*/

   if ((expressionList->type != SF_VARIABLE) || (theFunction == NULL))
     { return (rv); }

   /*===========================================*/
   /* Get the restrictions for the argument and */
   /* convert them to a constraint record.      */
   /*===========================================*/

   theRestriction2 = GetNthRestriction(theEnv,theFunction,i);
   constraint1 = ArgumentTypeToConstraintRecord(theEnv,theRestriction2);

   /*================================================*/
   /* Look for the constraint record associated with */
   /* binding the variable in the LHS of the rule.   */
   /*================================================*/

   theVariable = FindVariable(expressionList->lexemeValue,theLHS);
   if (theVariable != NULL)
     {
      if (theVariable->pnType == MF_VARIABLE_NODE)
        {
         constraint2 = GetConstraintRecord(theEnv);
         SetConstraintType(MULTIFIELD_TYPE,constraint2);
        }
      else if (theVariable->constraints == NULL)
        { constraint2 = GetConstraintRecord(theEnv); }
      else
        { constraint2 = CopyConstraintRecord(theEnv,theVariable->constraints); }
     }
   else
     { constraint2 = NULL; }

   /*================================================*/
   /* Look for the constraint record associated with */
   /* binding the variable on the RHS of the rule.   */
   /*================================================*/

   constraint3 = FindBindConstraints(theEnv,expressionList->lexemeValue);

   /*====================================================*/
   /* Union the LHS and RHS variable binding constraints */
   /* (the variable must satisfy one or the other).      */
   /*====================================================*/

   constraint3 = UnionConstraints(theEnv,constraint3,constraint2);

   /*====================================================*/
   /* Intersect the LHS/RHS variable binding constraints */
   /* with the function argument restriction constraints */
   /* (the variable must satisfy both).                  */
   /*====================================================*/

   constraint4 = IntersectConstraints(theEnv,constraint3,constraint1);

   /*====================================*/
   /* Check for unmatchable constraints. */
   /*====================================*/

   if (UnmatchableConstraint(constraint4))
     {
      PrintErrorID(theEnv,"RULECSTR",3,true);
      WriteString(theEnv,STDERR,"Previous variable bindings of ?");
      WriteString(theEnv,STDERR,expressionList->lexemeValue->contents);
      WriteString(theEnv,STDERR," caused the type restrictions");
      WriteString(theEnv,STDERR,"\nfor argument #");
      WriteInteger(theEnv,STDERR,i);
      WriteString(theEnv,STDERR," of the expression ");
      tmpPtr = lastOne->nextArg;
      lastOne->nextArg = NULL;
      PrintExpression(theEnv,STDERR,lastOne);
      lastOne->nextArg = tmpPtr;
      WriteString(theEnv,STDERR," found in the rule's RHS to be violated.\n");

      rv = true;
     }

   /*===========================================*/
   /* Free the temporarily created constraints. */
   /*===========================================*/

   RemoveConstraint(theEnv,constraint1);
   RemoveConstraint(theEnv,constraint2);
   RemoveConstraint(theEnv,constraint3);
   RemoveConstraint(theEnv,constraint4);

   /*========================================*/
   /* Return true if unmatchable constraints */
   /* were detected, otherwise false.        */
   /*========================================*/

   return(rv);
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) && DEFRULE_CONSTRUCT */
