   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  12/07/17             */
   /*                                                     */
   /*          FACT RETE FUNCTION GENERATION MODULE       */
   /*******************************************************/

/*************************************************************/
/* Purpose: Creates expressions used by the fact pattern     */
/*   matcher and the join network. The expressions created   */
/*   are used to extract and compare values from facts as    */
/*   needed by the Rete pattern matching algorithm. These    */
/*   expressions are also used to extract values from facts  */
/*   needed by expressions on the RHS of a rule.             */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.30: Support for performance optimizations.         */
/*                                                           */
/*            Increased maximum values for pattern/slot      */
/*            indices.                                       */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT

#include <stdio.h>

#include "constant.h"
#include "constrct.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "factmch.h"
#include "factmngr.h"
#include "factprt.h"
#include "factrete.h"
#include "memalloc.h"
#include "network.h"
#include "pattern.h"
#include "prcdrpsr.h"
#include "reteutil.h"
#include "router.h"
#include "scanner.h"
#include "sysdep.h"
#include "tmpltdef.h"
#include "tmpltfun.h"
#include "tmpltlhs.h"
#include "tmpltutl.h"

#include "factgen.h"

#define FACTGEN_DATA 2

struct factgenData
  {
   struct entityRecord   FactJNGV1Info;
   struct entityRecord   FactJNGV2Info;
   struct entityRecord   FactJNGV3Info;
   struct entityRecord   FactPNGV1Info;
   struct entityRecord   FactPNGV2Info;
   struct entityRecord   FactPNGV3Info;
   struct entityRecord   FactJNCV1Info;
   struct entityRecord   FactJNCV2Info;
   struct entityRecord   FactPNCV1Info;
   struct entityRecord   FactStoreMFInfo;
   struct entityRecord   FactSlotLengthInfo;
   struct entityRecord   FactPNConstant1Info;
   struct entityRecord   FactPNConstant2Info;
  };

#define FactgenData(theEnv) ((struct factgenData *) GetEnvironmentData(theEnv,FACTGEN_DATA))

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if (! RUN_TIME) && (! BLOAD_ONLY)
   static void                      *FactGetVarJN1(Environment *,struct lhsParseNode *,int);
   static void                      *FactGetVarJN2(Environment *,struct lhsParseNode *,int);
   static void                      *FactGetVarJN3(Environment *,struct lhsParseNode *,int);
   static void                      *FactGetVarPN1(Environment *,struct lhsParseNode *);
   static void                      *FactGetVarPN2(Environment *,struct lhsParseNode *);
   static void                      *FactGetVarPN3(Environment *,struct lhsParseNode *);
#endif

/*******************************************************************/
/* InitializeFactReteFunctions: Installs the fact pattern matching */
/*   and value access routines as primitive operations.            */
/*******************************************************************/
void InitializeFactReteFunctions(
  Environment *theEnv)
  {
#if DEFRULE_CONSTRUCT
   struct entityRecord   factJNGV1Info = { "FACT_JN_VAR1", FACT_JN_VAR1,0,1,0,
                                                  PrintFactJNGetVar1,
                                                  PrintFactJNGetVar1,NULL,
                                                  FactJNGetVar1,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factJNGV2Info = { "FACT_JN_VAR2", FACT_JN_VAR2,0,1,0,
                                                  PrintFactJNGetVar2,
                                                  PrintFactJNGetVar2,NULL,
                                                  FactJNGetVar2,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factJNGV3Info = { "FACT_JN_VAR3", FACT_JN_VAR3,0,1,0,
                                                  PrintFactJNGetVar3,
                                                  PrintFactJNGetVar3,NULL,
                                                  FactJNGetVar3,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factPNGV1Info = { "FACT_PN_VAR1", FACT_PN_VAR1,0,1,0,
                                                  PrintFactPNGetVar1,
                                                  PrintFactPNGetVar1,NULL,
                                                  FactPNGetVar1,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factPNGV2Info = { "FACT_PN_VAR2", FACT_PN_VAR2,0,1,0,
                                                  PrintFactPNGetVar2,
                                                  PrintFactPNGetVar2,NULL,
                                                  FactPNGetVar2,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factPNGV3Info = { "FACT_PN_VAR3", FACT_PN_VAR3,0,1,0,
                                                  PrintFactPNGetVar3,
                                                  PrintFactPNGetVar3,NULL,
                                                  FactPNGetVar3,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factJNCV1Info = { "FACT_JN_CMP1", FACT_JN_CMP1,0,1,1,
                                                  PrintFactJNCompVars1,
                                                  PrintFactJNCompVars1,NULL,
                                                  FactJNCompVars1,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factJNCV2Info = { "FACT_JN_CMP2", FACT_JN_CMP2,0,1,1,
                                                  PrintFactJNCompVars2,
                                                  PrintFactJNCompVars2,NULL,
                                                  FactJNCompVars2,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factPNCV1Info = { "FACT_PN_CMP1", FACT_PN_CMP1,0,1,1,
                                                  PrintFactPNCompVars1,
                                                  PrintFactPNCompVars1,NULL,
                                                  FactPNCompVars1,
                                                  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factStoreMFInfo = { "FACT_STORE_MULTIFIELD",
                                                    FACT_STORE_MULTIFIELD,0,1,0,
                                                    NULL,NULL,NULL,
                                                    (EntityEvaluationFunction *) FactStoreMultifield,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factSlotLengthInfo = { "FACT_SLOT_LENGTH",
                                                       FACT_SLOT_LENGTH,0,1,0,
                                                       PrintFactSlotLength,
                                                       PrintFactSlotLength,NULL,
                                                       FactSlotLength,
                                                       NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factPNConstant1Info = { "FACT_PN_CONSTANT1",
                                                        FACT_PN_CONSTANT1,0,1,1,
                                                        PrintFactPNConstant1,
                                                        PrintFactPNConstant1,NULL,
                                                        FactPNConstant1,
                                                        NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord   factPNConstant2Info = { "FACT_PN_CONSTANT2",
                                                        FACT_PN_CONSTANT2,0,1,1,
                                                        PrintFactPNConstant2,
                                                        PrintFactPNConstant2,NULL,
                                                        FactPNConstant2,
                                                        NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   AllocateEnvironmentData(theEnv,FACTGEN_DATA,sizeof(struct factgenData),NULL);

   memcpy(&FactgenData(theEnv)->FactJNGV1Info,&factJNGV1Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactJNGV2Info,&factJNGV2Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactJNGV3Info,&factJNGV3Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactPNGV1Info,&factPNGV1Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactPNGV2Info,&factPNGV2Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactPNGV3Info,&factPNGV3Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactJNCV1Info,&factJNCV1Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactJNCV2Info,&factJNCV2Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactPNCV1Info,&factPNCV1Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactStoreMFInfo,&factStoreMFInfo,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactSlotLengthInfo,&factSlotLengthInfo,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactPNConstant1Info,&factPNConstant1Info,sizeof(struct entityRecord));
   memcpy(&FactgenData(theEnv)->FactPNConstant2Info,&factPNConstant2Info,sizeof(struct entityRecord));

   InstallPrimitive(theEnv,(EntityRecord *) &FactData(theEnv)->FactInfo,FACT_ADDRESS_TYPE);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactJNGV1Info,FACT_JN_VAR1);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactJNGV2Info,FACT_JN_VAR2);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactJNGV3Info,FACT_JN_VAR3);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactPNGV1Info,FACT_PN_VAR1);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactPNGV2Info,FACT_PN_VAR2);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactPNGV3Info,FACT_PN_VAR3);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactJNCV1Info,FACT_JN_CMP1);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactJNCV2Info,FACT_JN_CMP2);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactPNCV1Info,FACT_PN_CMP1);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactStoreMFInfo,FACT_STORE_MULTIFIELD);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactSlotLengthInfo,FACT_SLOT_LENGTH);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactPNConstant1Info,FACT_PN_CONSTANT1);
   InstallPrimitive(theEnv,&FactgenData(theEnv)->FactPNConstant2Info,FACT_PN_CONSTANT2);
#endif
  }

#if (! RUN_TIME) && (! BLOAD_ONLY)

/******************************************************************/
/* FactGenPNConstant: Generates an expression for use in the fact */
/*   pattern network that compares a field from a single field or */
/*   multifield slot against a constant.                          */
/******************************************************************/
struct expr *FactGenPNConstant(
  Environment *theEnv,
  struct lhsParseNode *theField)
  {
   struct expr *top;
   ParseNodeType tempValue;
   struct factConstantPN1Call hack1;
   struct factConstantPN2Call hack2;

   /*=================================================================*/
   /* If the value of a single field slot (or relation name) is being */
   /* compared against a constant, then use specialized routines for  */
   /* doing the comparison.                                           */
   /*=================================================================*/

   if (theField->withinMultifieldSlot == false)
     {
      ClearBitString(&hack1,sizeof(struct factConstantPN1Call));

      if (theField->negated) hack1.testForEquality = false;
      else hack1.testForEquality = true;

      hack1.whichSlot = (theField->slotNumber - 1);

      top = GenConstant(theEnv,FACT_PN_CONSTANT1,AddBitMap(theEnv,&hack1,sizeof(struct factConstantPN1Call)));

      top->argList = GenConstant(theEnv,NodeTypeToType(theField),theField->value);

      return(top);
     }

   /*=================================================================*/
   /* If a constant comparison is being done within a multifield slot */
   /* and the constant's position has no multifields to the left,     */
   /* then use the same routine used for the single field slot case,  */
   /* but include the offset from the beginning of the slot.          */
   /*=================================================================*/

   else if ((theField->multiFieldsBefore == 0) ||
            ((theField->multiFieldsBefore == 1) && (theField->multiFieldsAfter == 0)))
     {
      ClearBitString(&hack2,sizeof(struct factConstantPN2Call));

      if (theField->negated) hack2.testForEquality = false;
      else hack2.testForEquality = true;

      hack2.whichSlot = (theField->slotNumber - 1);

      if (theField->multiFieldsBefore == 0)
        {
         hack2.fromBeginning = true;
         hack2.offset = theField->singleFieldsBefore;
        }
      else
        {
         hack2.fromBeginning = false;
         hack2.offset = theField->singleFieldsAfter;
        }

      top = GenConstant(theEnv,FACT_PN_CONSTANT2,AddBitMap(theEnv,&hack2,sizeof(struct factConstantPN2Call)));

      top->argList = GenConstant(theEnv,NodeTypeToType(theField),theField->value);

      return(top);
     }

   /*===============================================================*/
   /* Otherwise, use the equality or inequality function to compare */
   /* the constant against the value returned by the appropriate    */
   /* pattern network variable retrieval function call.             */
   /*===============================================================*/

   else
     {
      if (theField->negated)
        { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ); }
      else
        { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ); }

      tempValue = theField->pnType;
      theField->pnType = SF_VARIABLE_NODE;
      top->argList = FactGenGetfield(theEnv,theField);
      theField->pnType = tempValue;

      top->argList->nextArg = GenConstant(theEnv,NodeTypeToType(theField),theField->value);
     }

   /*===============================================================*/
   /* Return the expression for performing the constant comparison. */
   /*===============================================================*/

   return(top);
  }

/*******************************************************/
/* FactGenGetfield: Generates an expression for use in */
/*   the fact pattern network that retrieves a value   */
/*   from a single or multifield slot.                 */
/*******************************************************/
struct expr *FactGenGetfield(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   /*===================================================*/
   /* Generate call to retrieve single field slot value */
   /* or the fact relation name.                        */
   /*===================================================*/

   if ((theNode->slotNumber > 0) && (theNode->slotNumber != UNSPECIFIED_SLOT) && (theNode->withinMultifieldSlot == false))
     { return(GenConstant(theEnv,FACT_PN_VAR2,FactGetVarPN2(theEnv,theNode))); }

   /*=====================================================*/
   /* Generate call to retrieve a value from a multifield */
   /* slot that contains at most one multifield variable  */
   /* or contains no multifield variables before the      */
   /* value to be retrieved.                              */
   /*=====================================================*/

   if (((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE) || ConstantNode(theNode)) &&
       ((theNode->multiFieldsBefore == 0) ||
        ((theNode->multiFieldsBefore == 1) && (theNode->multiFieldsAfter == 0))))
     { return(GenConstant(theEnv,FACT_PN_VAR3,FactGetVarPN3(theEnv,theNode))); }

   if (((theNode->pnType == MF_WILDCARD_NODE) || (theNode->pnType == MF_VARIABLE_NODE)) &&
       (theNode->multiFieldsBefore == 0) && (theNode->multiFieldsAfter == 0))
     { return(GenConstant(theEnv,FACT_PN_VAR3,FactGetVarPN3(theEnv,theNode))); }

   /*=========================================*/
   /* Generate call to retrieve a value using */
   /* the most general retrieval function.    */
   /*=========================================*/

   return(GenConstant(theEnv,FACT_PN_VAR1,FactGetVarPN1(theEnv,theNode)));
  }

/**************************************************/
/* FactGenGetvar: Generates an expression for use */
/*   in the join network that retrieves a value   */
/*   from a single or multifield slot of a fact.  */
/**************************************************/
struct expr *FactGenGetvar(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  int side)
  {
   /*====================================================*/
   /* Generate call to retrieve single field slot value. */
   /*====================================================*/

   if ((theNode->slotNumber > 0) && (theNode->slotNumber != UNSPECIFIED_SLOT) && (theNode->withinMultifieldSlot == false))
     { return(GenConstant(theEnv,FACT_JN_VAR2,FactGetVarJN2(theEnv,theNode,side))); }

   /*=====================================================*/
   /* Generate call to retrieve a value from a multifield */
   /* slot that contains at most one multifield variable  */
   /* or contains no multifield variables before the      */
   /* value to be retrieved.                              */
   /*=====================================================*/

   if (((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE)) &&
       ((theNode->multiFieldsBefore == 0) ||
        ((theNode->multiFieldsBefore == 1) && (theNode->multiFieldsAfter == 0))))
     { return(GenConstant(theEnv,FACT_JN_VAR3,FactGetVarJN3(theEnv,theNode,side))); }

   if (((theNode->pnType == MF_WILDCARD_NODE) || (theNode->pnType == MF_VARIABLE_NODE)) &&
       (theNode->multiFieldsBefore == 0) &&
       (theNode->multiFieldsAfter == 0))
     { return(GenConstant(theEnv,FACT_JN_VAR3,FactGetVarJN3(theEnv,theNode,side))); }

   /*=========================================*/
   /* Generate call to retrieve a value using */
   /* the most general retrieval function.    */
   /*=========================================*/

   return(GenConstant(theEnv,FACT_JN_VAR1,FactGetVarJN1(theEnv,theNode,side)));
  }

/**************************************************************/
/* FactGenCheckLength: Generates an expression for use in the */
/*   fact pattern network that determines if the value of a   */
/*   multifield slot contains enough fields to satisfy the    */
/*   number of pattern matching constaints. For example, the  */
/*   slot constraints (foo ?x a $? ?y) couldn't be matched    */
/*   unless the foo slot contained at least 3 fields.         */
/**************************************************************/
struct expr *FactGenCheckLength(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct factCheckLengthPNCall hack;

   /*===================================================*/
   /* If the slot contains no single field constraints, */
   /* then a length test is not necessary.              */
   /*===================================================*/

   if ((theNode->singleFieldsAfter == 0) &&
       (theNode->pnType != SF_VARIABLE_NODE) &&
       (theNode->pnType != SF_WILDCARD_NODE))
     { return NULL; }

   /*=======================================*/
   /* Initialize the length test arguments. */
   /*=======================================*/

   ClearBitString(&hack,sizeof(struct factCheckLengthPNCall));
   hack.whichSlot = (theNode->slotNumber - 1);

   /*============================================*/
   /* If the slot has no multifield constraints, */
   /* then the length must match exactly.        */
   /*============================================*/

   if ((theNode->pnType != MF_VARIABLE_NODE) &&
       (theNode->pnType != MF_WILDCARD_NODE) &&
       (theNode->multiFieldsAfter == 0))
     { hack.exactly = 1; }
   else
     { hack.exactly = 0; }

   /*============================================*/
   /* The minimum length is the number of single */
   /* field constraints contained in the slot.   */
   /*============================================*/

   if ((theNode->pnType == SF_VARIABLE_NODE) || (theNode->pnType == SF_WILDCARD_NODE))
     { hack.minLength = 1 + theNode->singleFieldsAfter; }
   else
     { hack.minLength = theNode->singleFieldsAfter; }

   /*========================================================*/
   /* Generate call to test the length of a multifield slot. */
   /*========================================================*/

   return(GenConstant(theEnv,FACT_SLOT_LENGTH,AddBitMap(theEnv,&hack,sizeof(struct factCheckLengthPNCall))));
  }

/**************************************************************/
/* FactGenCheckZeroLength: Generates an expression for use in */
/*   the fact pattern network that determines if the value of */
/*   a multifield slot is a zero length multifield value.     */
/**************************************************************/
struct expr *FactGenCheckZeroLength(
  Environment *theEnv,
  unsigned short theSlot)
  {
   struct factCheckLengthPNCall hack;

   ClearBitString(&hack,sizeof(struct factCheckLengthPNCall));

   hack.whichSlot = theSlot - 1;
   hack.exactly = true;
   hack.minLength = 0;

   return(GenConstant(theEnv,FACT_SLOT_LENGTH,AddBitMap(theEnv,&hack,sizeof(struct factCheckLengthPNCall))));
  }

/*********************************************************************/
/* FactReplaceGetvar: Replaces a variable reference in an expression */
/*   with a function call to retrieve the variable using the join    */
/*   network variable access functions for facts.                    */
/*********************************************************************/
void FactReplaceGetvar(
  Environment *theEnv,
  struct expr *theItem,
  struct lhsParseNode *theNode,
  int side)
  {
   /*====================================================*/
   /* Generate call to retrieve single field slot value. */
   /*====================================================*/

   if ((theNode->slotNumber > 0) &&
       (theNode->slotNumber != UNSPECIFIED_SLOT) &&
       (theNode->withinMultifieldSlot == false))
     {
      theItem->type = FACT_JN_VAR2;
      theItem->value = FactGetVarJN2(theEnv,theNode,side);
      return;
     }

   /*=====================================================*/
   /* Generate call to retrieve a value from a multifield */
   /* slot that contains at most one multifield variable  */
   /* or contains no multifield variables before the      */
   /* value to be retrieved.                              */
   /*=====================================================*/

   if (((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE)) &&
       ((theNode->multiFieldsBefore == 0) ||
        ((theNode->multiFieldsBefore == 1) && (theNode->multiFieldsAfter == 0))))
     {
      theItem->type = FACT_JN_VAR3;
      theItem->value = FactGetVarJN3(theEnv,theNode,side);
      return;
     }

   if (((theNode->pnType == MF_WILDCARD_NODE) || (theNode->pnType == MF_VARIABLE_NODE)) &&
       (theNode->multiFieldsBefore == 0) &&
       (theNode->multiFieldsAfter == 0))
     {
      theItem->type = FACT_JN_VAR3;
      theItem->value = FactGetVarJN3(theEnv,theNode,side);
      return;
     }

   /*=========================================*/
   /* Generate call to retrieve a value using */
   /* the most general retrieval function.    */
   /*=========================================*/

   theItem->type = FACT_JN_VAR1;
   theItem->value = FactGetVarJN1(theEnv,theNode,side);
  }

/***********************************************************************/
/* FactReplaceGetfield: Replaces a variable reference in an expression */
/*   with a function call to retrieve the variable using the pattern   */
/*   network variable access functions for facts.                      */
/***********************************************************************/
void FactReplaceGetfield(
  Environment *theEnv,
  struct expr *theItem,
  struct lhsParseNode *theNode)
  {
   /*====================================================*/
   /* Generate call to retrieve single field slot value. */
   /*====================================================*/

   if (theNode->withinMultifieldSlot == false)
     {
      theItem->type = FACT_PN_VAR2;
      theItem->value = FactGetVarPN2(theEnv,theNode);
      return;
     }

   /*=====================================================*/
   /* Generate call to retrieve a value from a multifield */
   /* slot that contains at most one multifield variable  */
   /* or contains no multifield variables before the      */
   /* value to be retrieved.                              */
   /*=====================================================*/

   if (((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE)) &&
       ((theNode->multiFieldsBefore == 0) ||
        ((theNode->multiFieldsBefore == 1) && (theNode->multiFieldsAfter == 0))))
     {
      theItem->type = FACT_PN_VAR3;
      theItem->value = FactGetVarPN3(theEnv,theNode);
      return;
     }

   if (((theNode->pnType == MF_WILDCARD_NODE) || (theNode->pnType == MF_VARIABLE_NODE)) &&
       (theNode->multiFieldsBefore == 0) &&
       (theNode->multiFieldsAfter == 0))
     {
      theItem->type = FACT_PN_VAR3;
      theItem->value = FactGetVarPN3(theEnv,theNode);
      return;
     }

   /*=========================================*/
   /* Generate call to retrieve a value using */
   /* the most general retrieval function.    */
   /*=========================================*/

   theItem->type = FACT_PN_VAR1;
   theItem->value = FactGetVarPN1(theEnv,theNode);
  }

/*************************************************************/
/* FactGetVarJN1: Creates the arguments for the most general */
/*   routine for retrieving a variable's value from the slot */
/*   of a fact. The retrieval relies on information stored   */
/*   in a partial match, so this retrieval mechanism is used */
/*   by expressions in the join network or from the RHS of a */
/*   rule.                                                   */
/*************************************************************/
static void *FactGetVarJN1(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  int side)
  {
   struct factGetVarJN1Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factGetVarJN1Call));

   /*=========================================*/
   /* Store the position in the partial match */
   /* from which the fact will be retrieved.  */
   /*=========================================*/

   if (side == LHS)
     {
      hack.lhs = 1;
      hack.whichPattern = theNode->joinDepth;
     }
   else if (side == RHS)
     {
      hack.rhs = 1;
      hack.whichPattern = 0;
     }
   else if (side == NESTED_RHS)
     {
      hack.rhs = 1;
      hack.whichPattern = theNode->joinDepth;
     }
   else
     { hack.whichPattern = theNode->joinDepth; }

   /*========================================*/
   /* A slot value of zero indicates that we */
   /* want the pattern address returned.     */
   /*========================================*/

   if ((theNode->slotNumber == 0) || (theNode->slotNumber == UNSPECIFIED_SLOT))
     {
      hack.factAddress = 1;
      hack.allFields = 0;
      hack.whichSlot = 0;
      hack.whichField = 0;
     }

   /*=====================================================*/
   /* A slot value greater than zero and a field value of */
   /* zero indicate that we want the entire contents of   */
   /* the slot whether it is a single field or multifield */
   /* slot.                                               */
   /*=====================================================*/

   else if (theNode->index <= 0)
     {
      hack.factAddress = 0;
      hack.allFields = 1;
      hack.whichSlot = theNode->slotNumber - 1;
      hack.whichField = 0;
     }

   /*=====================================================*/
   /* A slot value, m, and a field value, n, both greater */
   /* than zero indicate that we want the nth field of    */
   /* the mth slot.                                       */
   /*=====================================================*/

   else
     {
      hack.factAddress = 0;
      hack.allFields = 0;
      hack.whichSlot = theNode->slotNumber - 1;
      hack.whichField = theNode->index - 1;
     }

   /*=============================*/
   /* Return the argument bitmap. */
   /*=============================*/

   return(AddBitMap(theEnv,&hack,sizeof(struct factGetVarJN1Call)));
  }

/**************************************************************/
/* FactGetVarJN2: Creates the arguments for the routine which */
/*   retrieves a variable's value from a single field slot of */
/*   a fact. The retrieval relies on information stored in a  */
/*   partial match, so this retrieval mechanism is used by    */
/*   expressions in the join network or from the RHS of a     */
/*   rule.                                                    */
/**************************************************************/
static void *FactGetVarJN2(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  int side)
  {
   struct factGetVarJN2Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factGetVarJN2Call));

   /*=====================================================*/
   /* Store the position in the partial match from which  */
   /* the fact will be retrieved and the slot in the fact */
   /* from which the value will be retrieved.             */
   /*=====================================================*/

   hack.whichSlot = theNode->slotNumber - 1;

   if (side == LHS)
     {
      hack.lhs = 1;
      hack.whichPattern = theNode->joinDepth;
     }
   else if (side == RHS)
     {
      hack.rhs = 1;
      hack.whichPattern = 0;
     }
   else if (side == NESTED_RHS)
     {
      hack.rhs = 1;
      hack.whichPattern = theNode->joinDepth;
     }
   else
     { hack.whichPattern = theNode->joinDepth; }

   /*=============================*/
   /* Return the argument bitmap. */
   /*=============================*/

   return AddBitMap(theEnv,&hack,sizeof(struct factGetVarJN2Call));
  }

/*************************************************************/
/* FactGetVarJN3: Creates the arguments for the routine for  */
/*   retrieving a variable's value from a multifield slot of */
/*   a fact. For this routine, the variable's value must be  */
/*   from a multifield slot that contains at most one        */
/*   multifield variable or contains no multifield variables */
/*   before the variable's value to be retrieved. The        */
/*   retrieval relies on information stored in a partial     */
/*   match, so this retrieval mechanism is used by           */
/*   expressions in the join network or from the RHS of a    */
/*   rule.                                                   */
/*************************************************************/
static void *FactGetVarJN3(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  int side)
  {
   struct factGetVarJN3Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factGetVarJN3Call));

   /*=====================================================*/
   /* Store the position in the partial match from which  */
   /* the fact will be retrieved and the slot in the fact */
   /* from which the value will be retrieved.             */
   /*=====================================================*/

   hack.whichSlot = theNode->slotNumber - 1;

   if (side == LHS)
     {
      hack.lhs = 1;
      hack.whichPattern = theNode->joinDepth;
     }
   else if (side == RHS)
     {
      hack.rhs = 1;
      hack.whichPattern = 0;
     }
   else if (side == NESTED_RHS)
     {
      hack.rhs = 1;
      hack.whichPattern = theNode->joinDepth;
     }
   else
     { hack.whichPattern = theNode->joinDepth; }

   /*==============================================================*/
   /* If a single field variable value is being retrieved, then... */
   /*==============================================================*/

   if ((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE))
     {
      /*=========================================================*/
      /* If no multifield values occur before the variable, then */
      /* the variable's value can be retrieved based on its      */
      /* offset from the beginning of the slot's value           */
      /* regardless of the number of multifield variables or     */
      /* wildcards following the variable being retrieved.       */
      /*=========================================================*/

      if (theNode->multiFieldsBefore == 0)
        {
         hack.fromBeginning = 1;
         hack.fromEnd = 0;
         hack.beginOffset = theNode->singleFieldsBefore;
         hack.endOffset = 0;
        }

      /*===============================================*/
      /* Otherwise the variable is retrieved based its */
      /* position relative to the end of the slot.     */
      /*===============================================*/

      else
        {
         hack.fromBeginning = 0;
         hack.fromEnd = 1;
         hack.beginOffset = 0;
         hack.endOffset = theNode->singleFieldsAfter;
        }

      /*=============================*/
      /* Return the argument bitmap. */
      /*=============================*/

      return(AddBitMap(theEnv,&hack,sizeof(struct factGetVarJN3Call)));
     }

   /*============================================================*/
   /* A multifield variable value is being retrieved. This means */
   /* that there are no other multifield variables or wildcards  */
   /* in the slot. The multifield value is retrieved by storing  */
   /* the number of single field values which come before and    */
   /* after the multifield value. The multifield value can then  */
   /* be retrieved based on the length of the value in the slot  */
   /* and the number of single field values which must occur     */
   /* before and after the multifield value.                     */
   /*============================================================*/

   hack.fromBeginning = 1;
   hack.fromEnd = 1;
   hack.beginOffset = theNode->singleFieldsBefore;
   hack.endOffset = theNode->singleFieldsAfter;

   /*=============================*/
   /* Return the argument bitmap. */
   /*=============================*/

   return AddBitMap(theEnv,&hack,sizeof(struct factGetVarJN3Call));
  }

/**************************************************************/
/* FactGetVarPN1: Creates the arguments for the most general  */
/*   routine for retrieving a variable's value from the slot  */
/*   of a fact. The retrieval relies on information stored    */
/*   during fact pattern matching, so this retrieval          */
/*   mechanism is only used by expressions in the pattern     */
/*   network.                                                 */
/**************************************************************/
static void *FactGetVarPN1(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct factGetVarPN1Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factGetVarPN1Call));

   /*========================================*/
   /* A slot value of zero indicates that we */
   /* want the pattern address returned.     */
   /*========================================*/

   if ((theNode->slotNumber == 0) ||
       (theNode->slotNumber == UNSPECIFIED_SLOT))
     {
      hack.factAddress = 1;
      hack.allFields = 0;
      hack.whichSlot = 0;
      hack.whichField = 0;
     }

   /*=====================================================*/
   /* A slot value greater than zero and a field value of */
   /* zero indicate that we want the entire contents of   */
   /* the slot whether it is a single field or multifield */
   /* slot.                                               */
   /*=====================================================*/

   else if (theNode->index <= 0)
     {
      hack.factAddress = 0;
      hack.allFields = 1;
      hack.whichSlot = theNode->slotNumber - 1;
      hack.whichField = 0;
     }

   /*=============================================*/
   /* A slot value, m, and a field value, n, both */
   /* greater than zero indicate that we want the */
   /* nth field of the mth slot.                  */
   /*=============================================*/

   else
     {
      hack.factAddress = 0;
      hack.allFields = 0;
      hack.whichSlot = theNode->slotNumber - 1;
      hack.whichField = theNode->index - 1;
     }

   /*=============================*/
   /* Return the argument bitmap. */
   /*=============================*/

   return AddBitMap(theEnv,&hack,sizeof(struct factGetVarPN1Call));
  }

/***************************************************************/
/* FactGetVarPN2: Creates the arguments for the routine which  */
/*   retrieves a variable's value from a single field slot of  */
/*   a fact. The retrieval relies on information stored during */
/*   fact pattern matching, so this retrieval mechanism is     */
/*   only used by expressions in the pattern network.          */
/***************************************************************/
static void *FactGetVarPN2(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct factGetVarPN2Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factGetVarPN2Call));

   /*=======================================*/
   /* Store the slot in the fact from which */
   /* the value will be retrieved.          */
   /*=======================================*/

   hack.whichSlot = theNode->slotNumber - 1;

   /*=============================*/
   /* Return the argument bitmap. */
   /*=============================*/

   return AddBitMap(theEnv,&hack,sizeof(struct factGetVarPN2Call));
  }

/*************************************************************/
/* FactGetVarPN3: Creates the arguments for the routine for  */
/*   retrieving a variable's value from a multifield slot of */
/*   a fact. For this routine, the variable's value must be  */
/*   from a multifield slot that contains at most one        */
/*   multifield variable or contains no multifield variables */
/*   before the variable's value to be retrieved. The        */
/*   retrieval relies on information stored during fact      */
/*   pattern matching, so this retrieval mechanism is only   */
/*   used by expressions in the pattern network.             */
/*************************************************************/
static void *FactGetVarPN3(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct factGetVarPN3Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factGetVarPN3Call));

   /*=======================================*/
   /* Store the slot in the fact from which */
   /* the value will be retrieved.          */
   /*=======================================*/

   hack.whichSlot = theNode->slotNumber - 1;

   /*==============================================================*/
   /* If a single field variable value is being retrieved, then... */
   /*==============================================================*/

   if ((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE) || ConstantNode(theNode))
     {
      /*=========================================================*/
      /* If no multifield values occur before the variable, then */
      /* the variable's value can be retrieved based on its      */
      /* offset from the beginning of the slot's value           */
      /* regardless of the number of multifield variables or     */
      /* wildcards following the variable being retrieved.       */
      /*=========================================================*/

      if (theNode->multiFieldsBefore == 0)
        {
         hack.fromBeginning = 1;
         hack.fromEnd = 0;
         hack.beginOffset = theNode->singleFieldsBefore;
         hack.endOffset = 0;
        }

      /*===============================================*/
      /* Otherwise the variable is retrieved based its */
      /* position relative to the end of the slot.     */
      /*===============================================*/

      else
        {
         hack.fromBeginning = 0;
         hack.fromEnd = 1;
         hack.beginOffset = 0;
         hack.endOffset = theNode->singleFieldsAfter;
        }

      return(AddBitMap(theEnv,&hack,sizeof(struct factGetVarPN3Call)));
     }

   /*============================================================*/
   /* A multifield variable value is being retrieved. This means */
   /* that there are no other multifield variables or wildcards  */
   /* in the slot. The multifield value is retrieved by storing  */
   /* the number of single field values which come before and    */
   /* after the multifield value. The multifield value can then  */
   /* be retrieved based on the length of the value in the slot  */
   /* and the number of single field values which must occur     */
   /* before and after the multifield value.                     */
   /*============================================================*/

   hack.fromBeginning = 1;
   hack.fromEnd = 1;
   hack.beginOffset = theNode->singleFieldsBefore;
   hack.endOffset = theNode->singleFieldsAfter;

   /*=============================*/
   /* Return the argument bitmap. */
   /*=============================*/

   return AddBitMap(theEnv,&hack,sizeof(struct factGetVarPN3Call));
  }

/*************************************************************/
/* FactPNVariableComparison: Generates an expression for use */
/*   in the fact pattern network to compare two variables of */
/*   the same name found in the same pattern.                */
/*************************************************************/
struct expr *FactPNVariableComparison(
  Environment *theEnv,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode)
  {
   struct expr *top;
   struct factCompVarsPN1Call hack;

   /*===================================================*/
   /* Clear the bitmap for storing the argument values. */
   /*===================================================*/

   ClearBitString(&hack,sizeof(struct factCompVarsPN1Call));

   /*================================================================*/
   /* If two single field slots of a deftemplate are being compared, */
   /* then use the following specified variable comparison routine.  */
   /*================================================================*/

   if ((selfNode->withinMultifieldSlot == false) &&
       (selfNode->slotNumber > 0) &&
       (selfNode->slotNumber != UNSPECIFIED_SLOT) &&
       (referringNode->withinMultifieldSlot == false) &&
       (referringNode->slotNumber > 0) &&
       (referringNode->slotNumber != UNSPECIFIED_SLOT))
     {
      hack.pass = 0;
      hack.fail = 0;
      hack.field1 = selfNode->slotNumber - 1;
      hack.field2 = referringNode->slotNumber - 1;

      if (selfNode->negated) hack.fail = 1;
      else hack.pass = 1;

      top = GenConstant(theEnv,FACT_PN_CMP1,AddBitMap(theEnv,&hack,sizeof(struct factCompVarsPN1Call)));
     }

   /*================================================================*/
   /* Otherwise, use the eq function to compare the values retrieved */
   /* by the appropriate get variable value functions.               */
   /*================================================================*/

   else
     {
      if (selfNode->negated) top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ);
      else top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ);

      top->argList = FactGenGetfield(theEnv,selfNode);
      top->argList->nextArg = FactGenGetfield(theEnv,referringNode);
     }

   /*======================================*/
   /* Return the expression for performing */
   /* the variable comparison.             */
   /*======================================*/

   return top;
  }

/*********************************************************/
/* FactJNVariableComparison: Generates an expression for */
/*   use in the join network to compare two variables of */
/*   the same name found in different patterns.          */
/*********************************************************/
struct expr *FactJNVariableComparison(
  Environment *theEnv,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode,
  bool nandJoin)
  {
   struct expr *top;
   struct factCompVarsJN1Call hack1;
   struct factCompVarsJN2Call hack2;
   struct lhsParseNode *firstNode;

   /*================================================================*/
   /* If two single field slots of a deftemplate are being compared, */
   /* then use the following specified variable comparison routine.  */
   /*================================================================*/

   if ((selfNode->withinMultifieldSlot == false) &&
       (selfNode->slotNumber > 0) &&
       (selfNode->slotNumber != UNSPECIFIED_SLOT) &&
       (referringNode->withinMultifieldSlot == false) &&
       (referringNode->slotNumber > 0) &&
       (referringNode->slotNumber != UNSPECIFIED_SLOT))
     {
      ClearBitString(&hack1,sizeof(struct factCompVarsJN1Call));
      hack1.pass = 0;
      hack1.fail = 0;

      if (nandJoin)
        { firstNode = referringNode; }
      else
        { firstNode = selfNode; }

      hack1.slot1 = firstNode->slotNumber - 1;

      if (nandJoin)
        { hack1.pattern1 = referringNode->joinDepth; }
      else
        { hack1.pattern1 = 0; }

      hack1.p1rhs = true;
      hack1.p2lhs = true;

      hack1.pattern2 = referringNode->joinDepth;

      if (referringNode->index == NO_INDEX) hack1.slot2 = 0;
      else hack1.slot2 = referringNode->slotNumber - 1;

      if (selfNode->negated) hack1.fail = 1;
      else hack1.pass = 1;

      top = GenConstant(theEnv,FACT_JN_CMP1,AddBitMap(theEnv,&hack1,sizeof(struct factCompVarsJN1Call)));
     }

   /*===============================================================*/
   /* If two single field values are compared and either or both of */
   /* them are contained in multifield slots (and the value can be  */
   /* accessed relative to either the beginning or end of the slot  */
   /* with no intervening multifield variables), then use the       */
   /* following specified variable comparison routine.              */
   /*===============================================================*/

   else if ((selfNode->slotNumber > 0) &&
            (selfNode->slotNumber != UNSPECIFIED_SLOT) &&
            (selfNode->pnType == SF_VARIABLE_NODE) &&
            ((selfNode->multiFieldsBefore == 0) ||
             ((selfNode->multiFieldsBefore == 1) &&
              (selfNode->multiFieldsAfter == 0))) &&
            (referringNode->slotNumber > 0) &&
            (referringNode->slotNumber != UNSPECIFIED_SLOT) &&
            (referringNode->pnType == SF_VARIABLE_NODE) &&
            ((referringNode->multiFieldsBefore == 0) ||
             (referringNode->multiFieldsAfter == 0)))
     {
      ClearBitString(&hack2,sizeof(struct factCompVarsJN2Call));
      hack2.pass = 0;
      hack2.fail = 0;

      if (nandJoin)
        { firstNode = referringNode; }
      else
        { firstNode = selfNode; }

      hack2.slot1 = firstNode->slotNumber - 1;

      if (nandJoin)
        { hack2.pattern1 = referringNode->joinDepth; }
      else
        { hack2.pattern1 = 0; }

      hack2.p1rhs = true;
      hack2.p2lhs = true;

      hack2.pattern2 = referringNode->joinDepth;
      hack2.slot2 = referringNode->slotNumber - 1;

      if (firstNode->multiFieldsBefore == 0)
        {
         hack2.fromBeginning1 = 1;
         hack2.offset1 = firstNode->singleFieldsBefore;
        }
      else
        {
         hack2.fromBeginning1 = 0;
         hack2.offset1 = firstNode->singleFieldsAfter;
        }

      if (referringNode->multiFieldsBefore == 0)
        {
         hack2.fromBeginning2 = 1;
         hack2.offset2 = referringNode->singleFieldsBefore;
        }
      else
        {
         hack2.fromBeginning2 = 0;
         hack2.offset2 = referringNode->singleFieldsAfter;
        }

      if (selfNode->negated) hack2.fail = 1;
      else hack2.pass = 1;

      top = GenConstant(theEnv,FACT_JN_CMP2,AddBitMap(theEnv,&hack2,sizeof(struct factCompVarsJN2Call)));
     }

   /*===============================================================*/
   /* Otherwise, use the equality or inequality function to compare */
   /* the values returned by the appropriate join network variable  */
   /* retrieval function call.                                      */
   /*===============================================================*/

   else
     {
      if (selfNode->negated)
        { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_NEQ); }
      else
        { top = GenConstant(theEnv,FCALL,ExpressionData(theEnv)->PTR_EQ); }

      if (nandJoin)
        { top->argList = FactGenGetvar(theEnv,selfNode,NESTED_RHS); }
      else
        { top->argList = FactGenGetvar(theEnv,selfNode,RHS); }

      top->argList->nextArg = FactGenGetvar(theEnv,referringNode,LHS);
     }

   /*======================================*/
   /* Return the expression for performing */
   /* the variable comparison.             */
   /*======================================*/

   return(top);
  }

#endif /* (! RUN_TIME) && (! BLOAD_ONLY) */

#endif /* DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT */


