   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/30/16             */
   /*                                                     */
   /*    INFERENCE ENGINE OBJECT PARSING ROUTINES MODULE  */
   /*******************************************************/

/*************************************************************/
/* Purpose: RETE Network Parsing Interface for Objects       */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Added support for hashed memories and other    */
/*            join network changes.                          */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*************************************************************/
/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if DEFRULE_CONSTRUCT && OBJECT_SYSTEM && (! RUN_TIME) && (! BLOAD_ONLY)

#include <stdio.h>

#include "classfun.h"
#include "envrnmnt.h"
#include "objrtfnx.h"

#include "objrtgen.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    GenObjectGetVar(Environment *,bool,Expression *,struct lhsParseNode *,int);
   static bool                    IsSimpleSlotVariable(struct lhsParseNode *);
   static Expression             *GenerateSlotComparisonTest(Environment *,bool,bool,struct lhsParseNode *,struct lhsParseNode *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/**********************************************
  Build functions used by AddPatternParser() to
  provide object access to the join nertwork
 **********************************************/
void ReplaceGetJNObjectValue(
  Environment *theEnv,
  Expression *theItem,
  struct lhsParseNode *theNode,
  int side)
  {
   GenObjectGetVar(theEnv,true,theItem,theNode,side);
  }

Expression *GenGetJNObjectValue(
  Environment *theEnv,
  struct lhsParseNode *theNode,
  int side)
  {
   Expression *theItem;

   theItem = GenConstant(theEnv,0,NULL);
   GenObjectGetVar(theEnv,true,theItem,theNode,side);
   return(theItem);
  }

Expression *ObjectJNVariableComparison(
  Environment *theEnv,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode,
  bool isNand)
  {
   return(GenerateSlotComparisonTest(theEnv,true,isNand,selfNode,referringNode));
  }

/**********************************************
  Build functions used by AddPatternParser() to
  provide object access to the pattern network
 **********************************************/
Expression *GenObjectPNConstantCompare(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct ObjectCmpPNConstant hack;
   Expression *theExp;
   ParseNodeType tmpType;

   /* ===============================================================
      If the value of a single field slot (or relation name) is being
      compared against a constant, then use specialized routines for
      doing the comparison.

      If a constant comparison is being done within a multifield slot
      and the constant's position has no multifields to the left or
      no multifields to the right, then use the same routine used for
      the single field slot case, but include the offset from either
      the beginning or end of the slot.

      Otherwise, use a general eq/neq test.
      =============================================================== */
   ClearBitString(&hack,sizeof(struct ObjectCmpPNConstant));
   if (theNode->negated)
     hack.fail = 1;
   else
     hack.pass = 1;
   if (((theNode->withinMultifieldSlot == false) ||
        (theNode->multiFieldsAfter == 0) ||
        (theNode->multiFieldsBefore == 0)) &&
       (theNode->slotNumber != ISA_ID) && (theNode->slotNumber != NAME_ID))
     {
      if (theNode->withinMultifieldSlot == false)
        hack.fromBeginning = true;
      else if (theNode->multiFieldsBefore == 0)
        {
         hack.fromBeginning = true;
         hack.offset = theNode->singleFieldsBefore;
        }
      else
        hack.offset = theNode->singleFieldsAfter;
      theExp = GenConstant(theEnv,OBJ_PN_CONSTANT,AddBitMap(theEnv,&hack,
                                        sizeof(struct ObjectCmpPNConstant)));
      theExp->argList = GenConstant(theEnv,NodeTypeToType(theNode),theNode->value);
     }
   else
     {
      hack.general = 1;
      theExp = GenConstant(theEnv,OBJ_PN_CONSTANT,AddBitMap(theEnv,&hack,
                                        sizeof(struct ObjectCmpPNConstant)));
      theExp->argList = GenConstant(theEnv,0,NULL);
      tmpType = theNode->pnType;
      theNode->pnType = SF_VARIABLE_NODE;
      GenObjectGetVar(theEnv,false,theExp->argList,theNode,-1);
      theNode->pnType = tmpType;
      theExp->argList->nextArg = GenConstant(theEnv,NodeTypeToType(theNode),theNode->value);
     }
   return(theExp);
  }

void ReplaceGetPNObjectValue(
  Environment *theEnv,
  Expression *theItem,
  struct lhsParseNode *theNode)
  {
   GenObjectGetVar(theEnv,false,theItem,theNode,-1);
  }

Expression *GenGetPNObjectValue(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   Expression *theItem;

   theItem = GenConstant(theEnv,0,NULL);
   GenObjectGetVar(theEnv,false,theItem,theNode,-1);
   return(theItem);
  }

Expression *ObjectPNVariableComparison(
  Environment *theEnv,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode)
  {
   return(GenerateSlotComparisonTest(theEnv,false,false,selfNode,referringNode));
  }

/****************************************************
  NAME         : GenObjectLengthTest
  DESCRIPTION  : Generates a test on the cardinality
                 of a slot matching an object pattern
  INPUTS       : The first lhsParseNode for a slot
                 in an object pattern
  RETURNS      : Nothing useful
  SIDE EFFECTS : The lhsParseNode network test is
                 modified to include the length test
  NOTES        : None
 ****************************************************/
void GenObjectLengthTest(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct ObjectMatchLength hack;
   Expression *theTest;

   if ((theNode->singleFieldsAfter == 0) &&
       (theNode->pnType != SF_VARIABLE_NODE) &&
       (theNode->pnType != SF_WILDCARD_NODE))
     return;

   ClearBitString(&hack,sizeof(struct ObjectMatchLength));

   if ((theNode->pnType != MF_VARIABLE_NODE) &&
       (theNode->pnType != MF_WILDCARD_NODE) &&
       (theNode->multiFieldsAfter == 0))
     hack.exactly = 1;
   else
     hack.exactly = 0;

   if ((theNode->pnType == SF_VARIABLE_NODE) || (theNode->pnType == SF_WILDCARD_NODE))
     hack.minLength = 1 + theNode->singleFieldsAfter;
   else
     hack.minLength = theNode->singleFieldsAfter;

   theTest = GenConstant(theEnv,OBJ_SLOT_LENGTH,AddBitMap(theEnv,&hack,
                                         sizeof(struct ObjectMatchLength)));

   if (theNode->constantSelector != NULL)
     { theNode->constantSelector->nextArg = CopyExpression(theEnv,theTest); }

   theNode->networkTest = CombineExpressions(theEnv,theTest,theNode->networkTest);
  }

/****************************************************
  NAME         : GenObjectZeroLengthTest
  DESCRIPTION  : Generates a test on the cardinality
                 of a slot matching an object pattern
  INPUTS       : The first lhsParseNode for a slot
                 in an object pattern
  RETURNS      : Nothing useful
  SIDE EFFECTS : The lhsParseNode network test is
                 modified to include the length test
  NOTES        : None
 ****************************************************/
void GenObjectZeroLengthTest(
  Environment *theEnv,
  struct lhsParseNode *theNode)
  {
   struct ObjectMatchLength hack;
   Expression *theTest;

   ClearBitString(&hack,sizeof(struct ObjectMatchLength));
   hack.exactly = 1;
   hack.minLength = 0;
   theTest = GenConstant(theEnv,OBJ_SLOT_LENGTH,
                         AddBitMap(theEnv,&hack,sizeof(struct ObjectMatchLength)));
   theNode->networkTest = CombineExpressions(theEnv,theTest,theNode->networkTest);
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************
  NAME         : GenObjectGetVar
  DESCRIPTION  : Generates the expressions necessary
                 to access object pattern variables
  INPUTS       : 1) An integer code indicating if
                    this is a join network reference
                    or a pattern network reference
                 2) The expression for which to set
                    the type and value
                 3) The lhsParseNode for the
                    variable reference
                 4) For a join reference, the side from
                    which the variable must be retrieved.
  RETURNS      : Nothing useful
  SIDE EFFECTS : The value is a packed long holding
                 pattern index, slot number,
                 field index, etc.
  NOTES        : None
 ***************************************************/
static void GenObjectGetVar(
  Environment *theEnv,
  bool joinReference,
  Expression *theItem,
  struct lhsParseNode *theNode,
  int side)
  {
   struct ObjectMatchVar1 hack1;
   struct ObjectMatchVar2 hack2;

   ClearBitString(&hack1,sizeof(struct ObjectMatchVar1));
   ClearBitString(&hack2,sizeof(struct ObjectMatchVar2));

   if (joinReference)
     {
      if (side == LHS)
        {
         hack1.lhs = 1;
         hack2.lhs = 1;
         hack1.whichPattern = theNode->joinDepth;
         hack2.whichPattern = theNode->joinDepth;
        }
      else if (side == RHS)
        {
         hack1.rhs = 1;
         hack2.rhs = 1;
         hack1.whichPattern = 0;
         hack2.whichPattern = 0;
        }
      else if (side == NESTED_RHS)
        {
         hack1.rhs = 1;
         hack2.rhs = 1;
         hack1.whichPattern = theNode->joinDepth;
         hack2.whichPattern = theNode->joinDepth;
        }
      else
        {
         hack1.whichPattern = theNode->joinDepth;
         hack2.whichPattern = theNode->joinDepth;
        }
     }

   /* ========================
      Access an object address
      ======================== */
   if (theNode->slotNumber == UNSPECIFIED_SLOT)
     {
      hack1.objectAddress = 1;
      if (joinReference)
        { theItem->type = OBJ_GET_SLOT_JNVAR1; }
      else
        { theItem->type = OBJ_GET_SLOT_PNVAR1; }
      theItem->value = AddBitMap(theEnv,&hack1,sizeof(struct ObjectMatchVar1));
      return;
     }

   /* ======================================
      Access the entire contents of the slot
      ====================================== */
   if ((theNode->singleFieldsBefore == 0) &&
       (theNode->singleFieldsAfter == 0) &&
       (theNode->multiFieldsBefore == 0) &&
       (theNode->multiFieldsAfter == 0) &&
       ((theNode->withinMultifieldSlot == false) ||
        (theNode->pnType == MF_VARIABLE_NODE) ||
        (theNode->pnType == MF_WILDCARD_NODE)))
     {
      hack1.allFields = 1;
      hack1.whichSlot = theNode->slotNumber;
      theItem->type = (joinReference ? OBJ_GET_SLOT_JNVAR1 : OBJ_GET_SLOT_PNVAR1);
      theItem->value = AddBitMap(theEnv,&hack1,sizeof(struct ObjectMatchVar1));
      return;
     }

   /* =============================================================
      Access a particular field(s) in a multifield slot pattern
      containing at most one multifield variable and at least
      one (or two if no multifield variables) single-field variable
      ============================================================= */
   if (((theNode->pnType == SF_WILDCARD_NODE) || (theNode->pnType == SF_VARIABLE_NODE) || ConstantNode(theNode)) &&
       ((theNode->multiFieldsBefore == 0) || (theNode->multiFieldsAfter == 0)))
     {
      hack2.whichSlot = theNode->slotNumber;
      if (theNode->multiFieldsBefore == 0)
        {
         hack2.fromBeginning = 1;
         hack2.beginningOffset = theNode->singleFieldsBefore;
        }
      else
        {
         hack2.fromEnd = 1;
         hack2.endOffset = theNode->singleFieldsAfter;
        }
      theItem->type = (joinReference ? OBJ_GET_SLOT_JNVAR2 : OBJ_GET_SLOT_PNVAR2);
      theItem->value = AddBitMap(theEnv,&hack2,sizeof(struct ObjectMatchVar2));
      return;
     }

   if (((theNode->pnType == MF_WILDCARD_NODE) || (theNode->pnType == MF_VARIABLE_NODE) || ConstantNode(theNode)) &&
       (theNode->multiFieldsBefore == 0) &&
       (theNode->multiFieldsAfter == 0))
     {
      hack2.whichSlot = theNode->slotNumber;
      hack2.fromBeginning = 1;
      hack2.fromEnd = 1;
      hack2.beginningOffset = theNode->singleFieldsBefore;
      hack2.endOffset = theNode->singleFieldsAfter;
      theItem->type = (joinReference ? OBJ_GET_SLOT_JNVAR2 : OBJ_GET_SLOT_PNVAR2);
      theItem->value = AddBitMap(theEnv,&hack2,sizeof(struct ObjectMatchVar2));
      return;
     }

   /* ==================================================
      General slot field access using multifield markers
      ================================================== */
   hack1.whichSlot = theNode->slotNumber;
   hack1.whichField = theNode->index;
   theItem->type = (joinReference ? OBJ_GET_SLOT_JNVAR1 : OBJ_GET_SLOT_PNVAR1);
   theItem->value = AddBitMap(theEnv,&hack1,sizeof(struct ObjectMatchVar1));
  }

/****************************************************************
  NAME         : IsSimpleSlotVariable
  DESCRIPTION  : Determines if a slot pattern variable
                 references a single-field slot or a single-field
                 in a multifield slot which does not require
                 use of multifield markers
                 (Object addresses are not simple variables)
  INPUTS       : The intermediate parse node
  RETURNS      : True if the variable is simple,
                 false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ****************************************************************/
static bool IsSimpleSlotVariable(
  struct lhsParseNode *node)
  {
   if ((node->pnType == MF_WILDCARD_NODE) || (node->pnType == MF_VARIABLE_NODE))
     return false;
   if ((node->slotNumber == UNSPECIFIED_SLOT) ||
       (node->slotNumber == ISA_ID) ||
       (node->slotNumber == NAME_ID))
     return false;
   if (node->withinMultifieldSlot == false)
     return true;
   if (node->multifieldSlot == true)
     return false;
   if ((node->multiFieldsBefore == 0) || (node->multiFieldsAfter == 0))
     return true;
   return false;
  }

/***************************************************************
  NAME         : GenerateSlotComparisonTest
  DESCRIPTION  : Generates pattern and join network
                 expressions for comparing object
                 pattern variables
  INPUTS       : 1) A flag indicating if this is a
                    pattern or join network test
                 2) For a join test, a flag indicating
                    if it is a nand join
                 3) The intermediate parse node
                    for the first variable
                 4) The intermediate parse node
                    for the second variable
  RETURNS      : An expression for comparing the
                 variables
  SIDE EFFECTS : Expression and bitmaps generated
  NOTES        : The following tests are generated
                 for the following scenarios:

                 SF slot w/ SF slot: PN_1 or JN_1
                 Example: (foo ?x) with (bar ?xy)

                 SF slot w/ SF reference in MF slot: PN_2 or JN_2
                 Example: (foo ?x) (bar ? ?x ? ?)

                 SF reference w/ SF reference: PN_3 or JN_3
                 Example: (foo ? ?x ?) and (bar ? ? ? ?x)

                 All other cases: EQ/NEQ general test
                 Example: (foo $? ?x $?) and (bar ?x)
 ***************************************************************/
static Expression *GenerateSlotComparisonTest(
  Environment *theEnv,
  bool joinTest,
  bool isNand,
  struct lhsParseNode *selfNode,
  struct lhsParseNode *referringNode)
  {
   Expression *theExp;
   struct ObjectCmpPNSingleSlotVars1 phack1;
   struct ObjectCmpPNSingleSlotVars2 phack2;
   struct ObjectCmpPNSingleSlotVars3 phack3;
   struct ObjectCmpJoinSingleSlotVars1 jhack1;
   struct ObjectCmpJoinSingleSlotVars2 jhack2;
   struct ObjectCmpJoinSingleSlotVars3 jhack3;
   struct lhsParseNode *firstNode;

   if (isNand)
     { firstNode = referringNode; }
   else
     { firstNode = selfNode; }

   /* =========================================================
      If we are comparing two single-field slot variables that
      don't require multifield markers for lookup, use
      a quick comparison.  Otherwise, use a general eq/neq with
      the pattern variable access routines
      ========================================================= */
   if (IsSimpleSlotVariable(firstNode) && IsSimpleSlotVariable(referringNode))
     {
      /* ==============================
         Compare two single-field slots
         ============================== */
      if ((firstNode->withinMultifieldSlot == false) &&
          (referringNode->withinMultifieldSlot == false))
        {
         ClearBitString(&phack1,sizeof(struct ObjectCmpPNSingleSlotVars1));
         ClearBitString(&jhack1,sizeof(struct ObjectCmpJoinSingleSlotVars1));
         if (selfNode->negated)
           phack1.fail = jhack1.fail = 1;
         else
           phack1.pass = jhack1.pass = 1;

         phack1.firstSlot = jhack1.firstSlot = firstNode->slotNumber;
         phack1.secondSlot = jhack1.secondSlot = referringNode->slotNumber;
         if (joinTest)
           {
            if (isNand)
              { jhack1.firstPattern = referringNode->joinDepth; }
            else
              { jhack1.firstPattern = 0; }

            jhack1.firstPatternRHS = true;
            jhack1.secondPatternLHS = true;

            jhack1.secondPattern = referringNode->joinDepth;

            theExp = GenConstant(theEnv,OBJ_JN_CMP1,AddBitMap(theEnv,&jhack1,
                                           sizeof(struct ObjectCmpJoinSingleSlotVars1)));
           }
         else
           theExp = GenConstant(theEnv,OBJ_PN_CMP1,AddBitMap(theEnv,&phack1,
                                           sizeof(struct ObjectCmpPNSingleSlotVars1)));
        }
      /* ============================================
         Compare a single-field slot with a
         single-field in a multifield slot (make sure
         the multifield slot reference is first
         ============================================ */
      else if ((firstNode->withinMultifieldSlot == false) ||
               (referringNode->withinMultifieldSlot == false))
        {
         ClearBitString(&phack2,sizeof(struct ObjectCmpPNSingleSlotVars2));
         ClearBitString(&jhack2,sizeof(struct ObjectCmpJoinSingleSlotVars2));

         if (selfNode->negated)
           phack2.fail = jhack2.fail = 1;
         else
           phack2.pass = jhack2.pass = 1;

         if (firstNode->withinMultifieldSlot == true)
           {
            phack2.firstSlot = jhack2.firstSlot = firstNode->slotNumber;
            phack2.secondSlot = jhack2.secondSlot = referringNode->slotNumber;
            if (joinTest)
              {
               if (isNand)
                 { jhack2.firstPattern = referringNode->joinDepth; }
               else
                 { jhack2.firstPattern = 0; }

               jhack2.firstPatternRHS = true;
               jhack2.secondPatternLHS = true;
               jhack2.secondPattern = referringNode->joinDepth;
              }

            if (firstNode->multiFieldsBefore == 0)
              {
               phack2.fromBeginning = jhack2.fromBeginning = 1;
               phack2.offset = jhack2.offset = firstNode->singleFieldsBefore;
              }
            else
              phack2.offset = jhack2.offset = firstNode->singleFieldsAfter;
           }
         else
           {
            phack2.firstSlot = jhack2.firstSlot = referringNode->slotNumber;
            phack2.secondSlot = jhack2.secondSlot = firstNode->slotNumber;
            if (joinTest)
              {
               if (isNand)
                 { jhack2.secondPattern = firstNode->joinDepth; }
               else
                 { jhack2.secondPattern = 0; }

               jhack2.secondPatternRHS = true;
               jhack2.firstPatternLHS = true;

               jhack2.firstPattern = referringNode->joinDepth;
              }

            if (referringNode->multiFieldsBefore == 0)
              {
               phack2.fromBeginning = jhack2.fromBeginning = 1;
               phack2.offset = jhack2.offset = referringNode->singleFieldsBefore;
              }
            else
              phack2.offset = jhack2.offset = referringNode->singleFieldsAfter;
           }
         if (joinTest)
           theExp = GenConstant(theEnv,OBJ_JN_CMP2,AddBitMap(theEnv,&jhack2,
                                           sizeof(struct ObjectCmpJoinSingleSlotVars2)));
         else
           theExp = GenConstant(theEnv,OBJ_PN_CMP2,AddBitMap(theEnv,&phack2,
                                           sizeof(struct ObjectCmpPNSingleSlotVars2)));
        }

      /* ===================================
         Compare two single-field references
         within multifield slots
         =================================== */
      else
        {
         ClearBitString(&phack3,sizeof(struct ObjectCmpPNSingleSlotVars3));
         ClearBitString(&jhack3,sizeof(struct ObjectCmpJoinSingleSlotVars3));

         if (selfNode->negated)
           phack3.fail = jhack3.fail = 1;
         else
           phack3.pass = jhack3.pass = 1;

         phack3.firstSlot = jhack3.firstSlot = firstNode->slotNumber;
         phack3.secondSlot = jhack3.secondSlot = referringNode->slotNumber;

         if (firstNode->multiFieldsBefore == 0)
           {
            phack3.firstFromBeginning = jhack3.firstFromBeginning = 1;
            phack3.firstOffset = jhack3.firstOffset = firstNode->singleFieldsBefore;
           }
         else
           phack3.firstOffset = jhack3.firstOffset = firstNode->singleFieldsAfter;

         if (referringNode->multiFieldsBefore == 0)
           {
            phack3.secondFromBeginning = jhack3.secondFromBeginning = 1;
            phack3.secondOffset = jhack3.secondOffset = referringNode->singleFieldsBefore;
           }
         else
           phack3.secondOffset = jhack3.secondOffset = referringNode->singleFieldsAfter;

         if (joinTest)
           {
            if (isNand)
              { jhack3.firstPattern = referringNode->joinDepth; }
            else
              { jhack3.firstPattern = 0; }

            jhack3.firstPatternRHS = true;
            jhack3.secondPatternLHS = true;
            jhack3.secondPattern = referringNode->joinDepth;

            theExp = GenConstant(theEnv,OBJ_JN_CMP3,AddBitMap(theEnv,&jhack3,
                                         sizeof(struct ObjectCmpJoinSingleSlotVars3)));
           }
         else
           theExp = GenConstant(theEnv,OBJ_PN_CMP3,AddBitMap(theEnv,&phack3,
                                           sizeof(struct ObjectCmpPNSingleSlotVars3)));
        }
     }

   /* ==================================================
      General comparison for multifield slot references,
      references which require multifield markers, and
      object addresses
      ================================================== */
   else
     {
      theExp = GenConstant(theEnv,FCALL,selfNode->negated ? ExpressionData(theEnv)->PTR_NEQ : ExpressionData(theEnv)->PTR_EQ);
      theExp->argList = GenConstant(theEnv,0,NULL);

      if (isNand)
        { GenObjectGetVar(theEnv,joinTest,theExp->argList,selfNode,NESTED_RHS); }
      else
        { GenObjectGetVar(theEnv,joinTest,theExp->argList,selfNode,RHS); }

      theExp->argList->nextArg = GenConstant(theEnv,0,NULL);

      GenObjectGetVar(theEnv,joinTest,theExp->argList->nextArg,referringNode,LHS);
     }
   return(theExp);
  }

#endif

/***************************************************
  NAME         :
  DESCRIPTION  :
  INPUTS       :
  RETURNS      :
  SIDE EFFECTS :
  NOTES        :
 ***************************************************/
