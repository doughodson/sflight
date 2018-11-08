   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/01/16             */
   /*                                                     */
   /*    INFERENCE ENGINE OBJECT ACCESS ROUTINES MODULE   */
   /*******************************************************/

/*************************************************************/
/* Purpose: RETE Network Interface for Objects               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Support for long long integers.                */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Added support for hashed alpha memories.       */
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
/*************************************************************/
/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if DEFRULE_CONSTRUCT && OBJECT_SYSTEM

#include <stdio.h>
#include <string.h>

#include "classcom.h"
#include "classfun.h"

#if DEVELOPER
#include "exprnops.h"
#endif
#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif
#include "constant.h"
#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "multifld.h"
#include "objrtmch.h"
#include "prntutil.h"
#include "reteutil.h"
#include "router.h"

#include "objrtfnx.h"

/* =========================================
   *****************************************
                 MACROS AND TYPES
   =========================================
   ***************************************** */

#define GetInsSlot(ins,si) ins->slotAddresses[ins->cls->slotNameMap[si]-1]

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    PrintObjectGetVarJN1(Environment *,const char *,void *);
   static bool                    ObjectGetVarJNFunction1(Environment *,void *,UDFValue *);
   static void                    PrintObjectGetVarJN2(Environment *,const char *,void *);
   static bool                    ObjectGetVarJNFunction2(Environment *,void *,UDFValue *);
   static void                    PrintObjectGetVarPN1(Environment *,const char *,void *);
   static bool                    ObjectGetVarPNFunction1(Environment *,void *,UDFValue *);
   static void                    PrintObjectGetVarPN2(Environment *,const char *,void *);
   static bool                    ObjectGetVarPNFunction2(Environment *,void *,UDFValue *);
   static void                    PrintObjectCmpConstant(Environment *,const char *,void *);
   static void                    PrintSlotLengthTest(Environment *,const char *,void *);
   static bool                    SlotLengthTestFunction(Environment *,void *,UDFValue *);
   static void                    PrintPNSimpleCompareFunction1(Environment *,const char *,void *);
   static bool                    PNSimpleCompareFunction1(Environment *,void *,UDFValue *);
   static void                    PrintPNSimpleCompareFunction2(Environment *,const char *,void *);
   static bool                    PNSimpleCompareFunction2(Environment *,void *,UDFValue *);
   static void                    PrintPNSimpleCompareFunction3(Environment *,const char *,void *);
   static bool                    PNSimpleCompareFunction3(Environment *,void *,UDFValue *);
   static void                    PrintJNSimpleCompareFunction1(Environment *,const char *,void *);
   static bool                    JNSimpleCompareFunction1(Environment *,void *,UDFValue *);
   static void                    PrintJNSimpleCompareFunction2(Environment *,const char *,void *);
   static bool                    JNSimpleCompareFunction2(Environment *,void *,UDFValue *);
   static void                    PrintJNSimpleCompareFunction3(Environment *,const char *,void *);
   static bool                    JNSimpleCompareFunction3(Environment *,void *,UDFValue *);
   static void                    GetPatternObjectAndMarks(Environment *,unsigned short,bool,bool,Instance **,struct multifieldMarker **);
   static void                    GetObjectValueGeneral(Environment *,UDFValue *,Instance *,
                                                        struct multifieldMarker *,struct ObjectMatchVar1 *);
   static void                    GetObjectValueSimple(Environment *,UDFValue *,Instance *,struct ObjectMatchVar2 *);
   static size_t                  CalculateSlotField(struct multifieldMarker *,InstanceSlot *,size_t,size_t *);
   static void                    GetInsMultiSlotField(CLIPSValue *,Instance *,unsigned,unsigned,unsigned);
   static void                    DeallocateObjectReteData(Environment *);
   static void                    DestroyObjectPatternNetwork(Environment *,OBJECT_PATTERN_NODE *);
   static void                    DestroyObjectAlphaNodes(Environment *,OBJECT_ALPHA_NODE *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************
  NAME         : InstallObjectPrimitives
  DESCRIPTION  : Installs all the entity records
                 associated with object pattern
                 matching operations
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Primitive operations installed
  NOTES        : None
 ***************************************************/
void InstallObjectPrimitives(
  Environment *theEnv)
  {
   struct entityRecord objectGVInfo1 = { "OBJ_GET_SLOT_JNVAR1", OBJ_GET_SLOT_JNVAR1,0,1,0,
                                             PrintObjectGetVarJN1,
                                             PrintObjectGetVarJN1,NULL,
                                             ObjectGetVarJNFunction1,
                                             NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord objectGVInfo2 = { "OBJ_GET_SLOT_JNVAR2", OBJ_GET_SLOT_JNVAR2,0,1,0,
                                             PrintObjectGetVarJN2,
                                             PrintObjectGetVarJN2,NULL,
                                             ObjectGetVarJNFunction2,
                                             NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord objectGVPNInfo1 = { "OBJ_GET_SLOT_PNVAR1", OBJ_GET_SLOT_PNVAR1,0,1,0,
                                               PrintObjectGetVarPN1,
                                               PrintObjectGetVarPN1,NULL,
                                               ObjectGetVarPNFunction1,
                                               NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord objectGVPNInfo2 = { "OBJ_GET_SLOT_PNVAR2", OBJ_GET_SLOT_PNVAR2,0,1,0,
                                               PrintObjectGetVarPN2,
                                               PrintObjectGetVarPN2,NULL,
                                               ObjectGetVarPNFunction2,
                                               NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord objectCmpConstantInfo = { "OBJ_PN_CONSTANT", OBJ_PN_CONSTANT,0,1,1,
                                                     PrintObjectCmpConstant,
                                                     PrintObjectCmpConstant,NULL,
                                                     ObjectCmpConstantFunction,
                                                     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord lengthTestInfo = { "OBJ_SLOT_LENGTH", OBJ_SLOT_LENGTH,0,1,0,
                                              PrintSlotLengthTest,
                                              PrintSlotLengthTest,NULL,
                                              SlotLengthTestFunction,
                                              NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord pNSimpleCompareInfo1 = { "OBJ_PN_CMP1", OBJ_PN_CMP1,0,1,1,
                                                    PrintPNSimpleCompareFunction1,
                                                    PrintPNSimpleCompareFunction1,NULL,
                                                    PNSimpleCompareFunction1,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord pNSimpleCompareInfo2 = { "OBJ_PN_CMP2", OBJ_PN_CMP2,0,1,1,
                                                    PrintPNSimpleCompareFunction2,
                                                    PrintPNSimpleCompareFunction2,NULL,
                                                    PNSimpleCompareFunction2,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord pNSimpleCompareInfo3 = { "OBJ_PN_CMP3", OBJ_PN_CMP3,0,1,1,
                                                    PrintPNSimpleCompareFunction3,
                                                    PrintPNSimpleCompareFunction3,NULL,
                                                    PNSimpleCompareFunction3,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord jNSimpleCompareInfo1 = { "OBJ_JN_CMP1", OBJ_JN_CMP1,0,1,1,
                                                    PrintJNSimpleCompareFunction1,
                                                    PrintJNSimpleCompareFunction1,NULL,
                                                    JNSimpleCompareFunction1,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord jNSimpleCompareInfo2 = { "OBJ_JN_CMP2", OBJ_JN_CMP2,0,1,1,
                                                    PrintJNSimpleCompareFunction2,
                                                    PrintJNSimpleCompareFunction2,NULL,
                                                    JNSimpleCompareFunction2,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   struct entityRecord jNSimpleCompareInfo3 = { "OBJ_JN_CMP3", OBJ_JN_CMP3,0,1,1,
                                                    PrintJNSimpleCompareFunction3,
                                                    PrintJNSimpleCompareFunction3,NULL,
                                                    JNSimpleCompareFunction3,
                                                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   AllocateEnvironmentData(theEnv,OBJECT_RETE_DATA,sizeof(struct objectReteData),DeallocateObjectReteData);
   ObjectReteData(theEnv)->CurrentObjectSlotLength = 1;

   memcpy(&ObjectReteData(theEnv)->ObjectGVInfo1,&objectGVInfo1,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->ObjectGVInfo2,&objectGVInfo2,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->ObjectGVPNInfo1,&objectGVPNInfo1,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->ObjectGVPNInfo2,&objectGVPNInfo2,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->ObjectCmpConstantInfo,&objectCmpConstantInfo,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->LengthTestInfo,&lengthTestInfo,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->PNSimpleCompareInfo1,&pNSimpleCompareInfo1,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->PNSimpleCompareInfo2,&pNSimpleCompareInfo2,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->PNSimpleCompareInfo3,&pNSimpleCompareInfo3,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->JNSimpleCompareInfo1,&jNSimpleCompareInfo1,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->JNSimpleCompareInfo2,&jNSimpleCompareInfo2,sizeof(struct entityRecord));
   memcpy(&ObjectReteData(theEnv)->JNSimpleCompareInfo3,&jNSimpleCompareInfo3,sizeof(struct entityRecord));

   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->ObjectGVInfo1,OBJ_GET_SLOT_JNVAR1);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->ObjectGVInfo2,OBJ_GET_SLOT_JNVAR2);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->ObjectGVPNInfo1,OBJ_GET_SLOT_PNVAR1);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->ObjectGVPNInfo2,OBJ_GET_SLOT_PNVAR2);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->ObjectCmpConstantInfo,OBJ_PN_CONSTANT);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->LengthTestInfo,OBJ_SLOT_LENGTH);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->PNSimpleCompareInfo1,OBJ_PN_CMP1);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->PNSimpleCompareInfo2,OBJ_PN_CMP2);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->PNSimpleCompareInfo3,OBJ_PN_CMP3);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->JNSimpleCompareInfo1,OBJ_JN_CMP1);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->JNSimpleCompareInfo2,OBJ_JN_CMP2);
   InstallPrimitive(theEnv,&ObjectReteData(theEnv)->JNSimpleCompareInfo3,OBJ_JN_CMP3);
  }

/*****************************************************/
/* DeallocateObjectReteData: Deallocates environment */
/*    data for the object rete network.              */
/*****************************************************/
static void DeallocateObjectReteData(
  Environment *theEnv)
  {
   OBJECT_PATTERN_NODE *theNetwork;

#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv)) return;
#endif

   theNetwork = ObjectReteData(theEnv)->ObjectPatternNetworkPointer;
   DestroyObjectPatternNetwork(theEnv,theNetwork);
  }

/****************************************************************/
/* DestroyObjectPatternNetwork: Deallocates the data structures */
/*   associated with the object pattern network.                */
/****************************************************************/
static void DestroyObjectPatternNetwork(
  Environment *theEnv,
  OBJECT_PATTERN_NODE *thePattern)
  {
   OBJECT_PATTERN_NODE *patternPtr;

   if (thePattern == NULL) return;

   while (thePattern != NULL)
     {
      patternPtr = thePattern->rightNode;

      DestroyObjectPatternNetwork(theEnv,thePattern->nextLevel);
      DestroyObjectAlphaNodes(theEnv,thePattern->alphaNode);
#if ! RUN_TIME
      rtn_struct(theEnv,objectPatternNode,thePattern);
#endif
      thePattern = patternPtr;
     }
  }

/************************************************************/
/* DestroyObjectAlphaNodes: Deallocates the data structures */
/*   associated with the object alpha nodes.                */
/************************************************************/
static void DestroyObjectAlphaNodes(
  Environment *theEnv,
  OBJECT_ALPHA_NODE *theNode)
  {
   OBJECT_ALPHA_NODE *nodePtr;

   if (theNode == NULL) return;

   while (theNode != NULL)
     {
      nodePtr = theNode->nxtInGroup;

      DestroyAlphaMemory(theEnv,&theNode->header,false);

#if ! RUN_TIME
      rtn_struct(theEnv,objectAlphaNode,theNode);
#endif

      theNode = nodePtr;
     }
  }

/*****************************************************
  NAME         : ObjectCmpConstantFunction
  DESCRIPTION  : Used to compare object slot values
                 against a constant
  INPUTS       : 1) The constant test bitmap
                 2) Data object buffer to hold result
  RETURNS      : True if test successful,
                 false otherwise
  SIDE EFFECTS : Buffer set to symbol TRUE if test
                 successful, false otherwise
  NOTES        : Called directly by
                   EvaluatePatternExpression()
 *****************************************************/
bool ObjectCmpConstantFunction(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectCmpPNConstant *hack;
   UDFValue theVar;
   Expression *constantExp;
   bool rv;
   Multifield *theSegment;

   hack = (struct ObjectCmpPNConstant *) ((CLIPSBitMap *) theValue)->contents;
   if (hack->general)
     {
      EvaluateExpression(theEnv,GetFirstArgument(),&theVar);
      constantExp = GetFirstArgument()->nextArg;
     }
   else
     {
      constantExp = GetFirstArgument();
      if (ObjectReteData(theEnv)->CurrentPatternObjectSlot->type == MULTIFIELD_TYPE)
        {
         theSegment = ObjectReteData(theEnv)->CurrentPatternObjectSlot->multifieldValue;
         if (hack->fromBeginning)
           {
            theVar.value = theSegment->contents[hack->offset].value;
           }
         else
           {
            theVar.value = theSegment->contents[theSegment->length -
                                      (hack->offset + 1)].value;
           }
        }
      else
        {
         theVar.value = ObjectReteData(theEnv)->CurrentPatternObjectSlot->value;
        }
     }
   if (theVar.header->type != constantExp->type)
     rv = hack->fail ? true : false;
   else if (theVar.value != constantExp->value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

static void PrintObjectGetVarJN1(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectMatchVar1 *hack;

   hack = (struct ObjectMatchVar1 *) ((CLIPSBitMap *) theValue)->contents;

   if (hack->objectAddress)
     {
      WriteString(theEnv,logicalName,"(obj-ptr ");
      PrintUnsignedInteger(theEnv,logicalName,hack->whichPattern);
     }
   else if (hack->allFields)
     {
      WriteString(theEnv,logicalName,"(obj-slot-contents ");
      PrintUnsignedInteger(theEnv,logicalName,hack->whichPattern);
      WriteString(theEnv,logicalName," ");
      WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->whichSlot)->contents);
     }
   else
     {
      WriteString(theEnv,logicalName,"(obj-slot-var ");
      PrintUnsignedInteger(theEnv,logicalName,hack->whichPattern);
      WriteString(theEnv,logicalName," ");
      WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->whichSlot)->contents);
      WriteString(theEnv,logicalName," ");
      PrintUnsignedInteger(theEnv,logicalName,hack->whichField);
     }
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool ObjectGetVarJNFunction1(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectMatchVar1 *hack;
   Instance *theInstance;
   struct multifieldMarker *theMarks;

   hack = (struct ObjectMatchVar1 *) ((CLIPSBitMap *) theValue)->contents;
   GetPatternObjectAndMarks(theEnv,hack->whichPattern,hack->lhs,hack->rhs,&theInstance,&theMarks);
   GetObjectValueGeneral(theEnv,theResult,theInstance,theMarks,hack);
   return true;
  }

static void PrintObjectGetVarJN2(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectMatchVar2 *hack;

   hack = (struct ObjectMatchVar2 *) ((CLIPSBitMap *) theValue)->contents;
   WriteString(theEnv,logicalName,"(obj-slot-quick-var ");
   PrintUnsignedInteger(theEnv,logicalName,hack->whichPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->whichSlot)->contents);
   if (hack->fromBeginning)
     {
      WriteString(theEnv,logicalName," B");
      PrintUnsignedInteger(theEnv,logicalName,hack->beginningOffset + 1);
     }
   if (hack->fromEnd)
     {
      WriteString(theEnv,logicalName," E");
      PrintUnsignedInteger(theEnv,logicalName,hack->endOffset + 1);
     }
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool ObjectGetVarJNFunction2(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectMatchVar2 *hack;
   Instance *theInstance;
   struct multifieldMarker *theMarks;

   hack = (struct ObjectMatchVar2 *) ((CLIPSBitMap *) theValue)->contents;
   GetPatternObjectAndMarks(theEnv,hack->whichPattern,hack->lhs,hack->rhs,&theInstance,&theMarks);
   GetObjectValueSimple(theEnv,theResult,theInstance,hack);
   return true;
  }

static void PrintObjectGetVarPN1(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectMatchVar1 *hack;

   hack = (struct ObjectMatchVar1 *) ((CLIPSBitMap *) theValue)->contents;

   if (hack->objectAddress)
     WriteString(theEnv,logicalName,"(ptn-obj-ptr ");
   else if (hack->allFields)
     {
      WriteString(theEnv,logicalName,"(ptn-obj-slot-contents ");
      WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->whichSlot)->contents);
     }
   else
     {
      WriteString(theEnv,logicalName,"(ptn-obj-slot-var ");
      WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->whichSlot)->contents);
      WriteString(theEnv,logicalName," ");
      PrintUnsignedInteger(theEnv,logicalName,hack->whichField);
     }
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool ObjectGetVarPNFunction1(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectMatchVar1 *hack;

   hack = (struct ObjectMatchVar1 *) ((CLIPSBitMap *) theValue)->contents;
   GetObjectValueGeneral(theEnv,theResult,ObjectReteData(theEnv)->CurrentPatternObject,ObjectReteData(theEnv)->CurrentPatternObjectMarks,hack);
   return true;
  }

static void PrintObjectGetVarPN2(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectMatchVar2 *hack;

   hack = (struct ObjectMatchVar2 *) ((CLIPSBitMap *) theValue)->contents;
   WriteString(theEnv,logicalName,"(ptn-obj-slot-quick-var ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->whichSlot)->contents);
   if (hack->fromBeginning)
     {
      WriteString(theEnv,logicalName," B");
      PrintUnsignedInteger(theEnv,logicalName,(hack->beginningOffset + 1));
     }
   if (hack->fromEnd)
     {
      WriteString(theEnv,logicalName," E");
      PrintUnsignedInteger(theEnv,logicalName,(hack->endOffset + 1));
     }
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool ObjectGetVarPNFunction2(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectMatchVar2 *hack;

   hack = (struct ObjectMatchVar2 *) ((CLIPSBitMap *) theValue)->contents;
   GetObjectValueSimple(theEnv,theResult,ObjectReteData(theEnv)->CurrentPatternObject,hack);
   return true;
  }

static void PrintObjectCmpConstant(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpPNConstant *hack;

   hack = (struct ObjectCmpPNConstant *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(obj-const ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   if (hack->general)
     PrintExpression(theEnv,logicalName,GetFirstArgument());
   else
     {
      WriteString(theEnv,logicalName,hack->fromBeginning ? "B" : "E");
      PrintUnsignedInteger(theEnv,logicalName,hack->offset);
      WriteString(theEnv,logicalName," ");
      PrintExpression(theEnv,logicalName,GetFirstArgument());
     }
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static void PrintSlotLengthTest(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectMatchLength *hack;

   hack = (struct ObjectMatchLength *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(obj-slot-len ");
   if (hack->exactly)
     WriteString(theEnv,logicalName,"= ");
   else
     WriteString(theEnv,logicalName,">= ");
   PrintUnsignedInteger(theEnv,logicalName,hack->minLength);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool SlotLengthTestFunction(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectMatchLength *hack;

   theResult->value = FalseSymbol(theEnv);
   hack = (struct ObjectMatchLength *) ((CLIPSBitMap *) theValue)->contents;
   if (ObjectReteData(theEnv)->CurrentObjectSlotLength < hack->minLength)
     return false;
   if (hack->exactly && (ObjectReteData(theEnv)->CurrentObjectSlotLength > hack->minLength))
     return false;
   theResult->value = TrueSymbol(theEnv);
   return true;
  }

static void PrintPNSimpleCompareFunction1(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpPNSingleSlotVars1 *hack;

   hack = (struct ObjectCmpPNSingleSlotVars1 *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(pslot-cmp1 ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->firstSlot)->contents);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->secondSlot)->contents);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool PNSimpleCompareFunction1(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectCmpPNSingleSlotVars1 *hack;
   InstanceSlot *is1,*is2;
   bool rv;

   hack = (struct ObjectCmpPNSingleSlotVars1 *) ((CLIPSBitMap *) theValue)->contents;
   is1 = GetInsSlot(ObjectReteData(theEnv)->CurrentPatternObject,hack->firstSlot);
   is2 = GetInsSlot(ObjectReteData(theEnv)->CurrentPatternObject,hack->secondSlot);
   if (is1->type != is2->type)
     rv = hack->fail;
   else if (is1->value != is2->value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

static void PrintPNSimpleCompareFunction2(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpPNSingleSlotVars2 *hack;

   hack = (struct ObjectCmpPNSingleSlotVars2 *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(pslot-cmp2 ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->firstSlot)->contents);
   WriteString(theEnv,logicalName,hack->fromBeginning ? " B" : " E");
   PrintUnsignedInteger(theEnv,logicalName,hack->offset);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->secondSlot)->contents);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool PNSimpleCompareFunction2(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectCmpPNSingleSlotVars2 *hack;
   bool rv;
   CLIPSValue f1;
   InstanceSlot *is2;

   hack = (struct ObjectCmpPNSingleSlotVars2 *) ((CLIPSBitMap *) theValue)->contents;
   GetInsMultiSlotField(&f1,ObjectReteData(theEnv)->CurrentPatternObject,hack->firstSlot,
                        hack->fromBeginning,hack->offset);
   is2 = GetInsSlot(ObjectReteData(theEnv)->CurrentPatternObject,hack->secondSlot);
   if (f1.value != is2->value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

static void PrintPNSimpleCompareFunction3(
  Environment*theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpPNSingleSlotVars3 *hack;

   hack = (struct ObjectCmpPNSingleSlotVars3 *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(pslot-cmp3 ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->firstSlot)->contents);
   WriteString(theEnv,logicalName,hack->firstFromBeginning ? " B" : " E");
   PrintUnsignedInteger(theEnv,logicalName,hack->firstOffset);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->secondSlot)->contents);
   WriteString(theEnv,logicalName,hack->secondFromBeginning ? " B" : " E");
   PrintUnsignedInteger(theEnv,logicalName,hack->secondOffset);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool PNSimpleCompareFunction3(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   struct ObjectCmpPNSingleSlotVars3 *hack;
   bool rv;
   CLIPSValue f1, f2;

   hack = (struct ObjectCmpPNSingleSlotVars3 *) ((CLIPSBitMap *) theValue)->contents;
   GetInsMultiSlotField(&f1,ObjectReteData(theEnv)->CurrentPatternObject,hack->firstSlot,
                        hack->firstFromBeginning,hack->firstOffset);
   GetInsMultiSlotField(&f2,ObjectReteData(theEnv)->CurrentPatternObject,hack->secondSlot,
                        hack->secondFromBeginning,hack->secondOffset);
   if (f1.value != f2.value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

static void PrintJNSimpleCompareFunction1(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpJoinSingleSlotVars1 *hack;

   hack = (struct ObjectCmpJoinSingleSlotVars1 *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(jslot-cmp1 ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   PrintUnsignedInteger(theEnv,logicalName,hack->firstPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->firstSlot)->contents);
   WriteString(theEnv,logicalName," ");
   PrintUnsignedInteger(theEnv,logicalName,hack->secondPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->secondSlot)->contents);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool JNSimpleCompareFunction1(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   Instance *ins1,*ins2;
   struct multifieldMarker *theMarks;
   struct ObjectCmpJoinSingleSlotVars1 *hack;
   bool rv;
   InstanceSlot *is1,*is2;

   hack = (struct ObjectCmpJoinSingleSlotVars1 *) ((CLIPSBitMap *) theValue)->contents;
   GetPatternObjectAndMarks(theEnv,hack->firstPattern,hack->firstPatternLHS,hack->firstPatternRHS,&ins1,&theMarks);
   is1 = GetInsSlot(ins1,hack->firstSlot);
   GetPatternObjectAndMarks(theEnv,hack->secondPattern,hack->secondPatternLHS,hack->secondPatternRHS,&ins2,&theMarks);
   is2 = GetInsSlot(ins2,hack->secondSlot);
   if (is1->type != is2->type)
     rv = hack->fail;
   else if (is1->value != is2->value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

static void PrintJNSimpleCompareFunction2(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpJoinSingleSlotVars2 *hack;

   hack = (struct ObjectCmpJoinSingleSlotVars2 *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(jslot-cmp2 ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   PrintUnsignedInteger(theEnv,logicalName,hack->firstPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->firstSlot)->contents);
   WriteString(theEnv,logicalName,hack->fromBeginning ? " B" : " E");
   PrintUnsignedInteger(theEnv,logicalName,hack->offset);
   WriteString(theEnv,logicalName," ");
   PrintUnsignedInteger(theEnv,logicalName,hack->secondPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->secondSlot)->contents);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool JNSimpleCompareFunction2(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   Instance *ins1,*ins2;
   struct multifieldMarker *theMarks;
   struct ObjectCmpJoinSingleSlotVars2 *hack;
   bool rv;
   CLIPSValue f1;
   InstanceSlot *is2;

   hack = (struct ObjectCmpJoinSingleSlotVars2 *) ((CLIPSBitMap *) theValue)->contents;
   GetPatternObjectAndMarks(theEnv,hack->firstPattern,hack->firstPatternLHS,hack->firstPatternRHS,&ins1,&theMarks);
   GetInsMultiSlotField(&f1,ins1,hack->firstSlot,
                        hack->fromBeginning,hack->offset);
   GetPatternObjectAndMarks(theEnv,hack->secondPattern,hack->secondPatternLHS,hack->secondPatternRHS,&ins2,&theMarks);
   is2 = GetInsSlot(ins2,hack->secondSlot);
   if (f1.value != is2->value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

static void PrintJNSimpleCompareFunction3(
  Environment *theEnv,
  const char *logicalName,
  void *theValue)
  {
#if DEVELOPER
   struct ObjectCmpJoinSingleSlotVars3 *hack;

   hack = (struct ObjectCmpJoinSingleSlotVars3 *) ((CLIPSBitMap *) theValue)->contents;

   WriteString(theEnv,logicalName,"(jslot-cmp3 ");
   WriteString(theEnv,logicalName,hack->pass ? "p " : "n ");
   PrintUnsignedInteger(theEnv,logicalName,hack->firstPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->firstSlot)->contents);
   WriteString(theEnv,logicalName,hack->firstFromBeginning ? " B" : " E");
   PrintUnsignedInteger(theEnv,logicalName,hack->firstOffset);
   WriteString(theEnv,logicalName," ");
   PrintUnsignedInteger(theEnv,logicalName,hack->secondPattern);
   WriteString(theEnv,logicalName," ");
   WriteString(theEnv,logicalName,FindIDSlotName(theEnv,hack->secondSlot)->contents);
   WriteString(theEnv,logicalName,hack->secondFromBeginning ? " B" : " E");
   PrintUnsignedInteger(theEnv,logicalName,hack->secondOffset);
   WriteString(theEnv,logicalName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
  }

static bool JNSimpleCompareFunction3(
  Environment *theEnv,
  void *theValue,
  UDFValue *theResult)
  {
   Instance *ins1,*ins2;
   struct multifieldMarker *theMarks;
   struct ObjectCmpJoinSingleSlotVars3 *hack;
   bool rv;
   CLIPSValue f1,f2;

   hack = (struct ObjectCmpJoinSingleSlotVars3 *) ((CLIPSBitMap *) theValue)->contents;
   GetPatternObjectAndMarks(theEnv,hack->firstPattern,hack->firstPatternLHS,hack->firstPatternRHS,&ins1,&theMarks);
   GetInsMultiSlotField(&f1,ins1,hack->firstSlot,
                        hack->firstFromBeginning,
                        hack->firstOffset);
   GetPatternObjectAndMarks(theEnv,hack->secondPattern,hack->secondPatternLHS,hack->secondPatternRHS,&ins2,&theMarks);
   GetInsMultiSlotField(&f2,ins2,hack->secondSlot,
                        hack->secondFromBeginning,
                        hack->secondOffset);
   if (f1.value != f2.value)
     rv = hack->fail ? true : false;
   else
     rv = hack->pass ? true : false;
   theResult->value = rv ? TrueSymbol(theEnv) : FalseSymbol(theEnv);
   return rv;
  }

/****************************************************
  NAME         : GetPatternObjectAndMarks
  DESCRIPTION  : Finds the instance and multfiield
                 markers corresponding to a specified
                 pattern in the join network
  INPUTS       : 1) The index of the desired pattern
                 2) A buffer to hold the instance
                    address
                 3) A buffer to hold the list of
                    multifield markers
  RETURNS      : Nothing useful
  SIDE EFFECTS : Buffers set
  NOTES        : None
 ****************************************************/
static void GetPatternObjectAndMarks(
  Environment *theEnv,
  unsigned short pattern,
  bool lhs,
  bool rhs,
  Instance **theInstance,
  struct multifieldMarker **theMarkers)
  {
   if (lhs)
     {
      *theInstance = (Instance *)
        get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,pattern)->matchingItem;
      *theMarkers =
        get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,pattern)->markers;
     }
   else if (rhs)
     {
      *theInstance = (Instance *)
        get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,pattern)->matchingItem;
      *theMarkers =
        get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,pattern)->markers;
     }
   else if (EngineData(theEnv)->GlobalRHSBinds == NULL)
     {
      *theInstance = (Instance *)
        get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,pattern)->matchingItem;
      *theMarkers =
        get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,pattern)->markers;
     }
   else if ((EngineData(theEnv)->GlobalJoin->depth - 1) == pattern)
     {
      *theInstance = (Instance *)
        get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,0)->matchingItem;
      *theMarkers = get_nth_pm_match(EngineData(theEnv)->GlobalRHSBinds,0)->markers;
     }
   else
     {
      *theInstance = (Instance *)
        get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,pattern)->matchingItem;
      *theMarkers =
        get_nth_pm_match(EngineData(theEnv)->GlobalLHSBinds,pattern)->markers;
     }
  }

/***************************************************
  NAME         : GetObjectValueGeneral
  DESCRIPTION  : Access function for getting
                 pattern variable values within the
                 object pattern and join networks
  INPUTS       : 1) The result data object buffer
                 2) The instance to access
                 3) The list of multifield markers
                    for the pattern
                 4) Data for variable reference
  RETURNS      : Nothing useful
  SIDE EFFECTS : Data object is filled with the
                 values of the pattern variable
  NOTES        : None
 ***************************************************/
static void GetObjectValueGeneral(
  Environment *theEnv,
  UDFValue *returnValue,
  Instance *theInstance,
  struct multifieldMarker *theMarks,
  struct ObjectMatchVar1 *matchVar)
  {
   size_t field;
   size_t extent;
   InstanceSlot **insSlot,*basisSlot;

   if (matchVar->objectAddress)
     {
      returnValue->value = theInstance;
      return;
     }
   if (matchVar->whichSlot == ISA_ID)
     {
      returnValue->value = GetDefclassNamePointer(theInstance->cls);
      return;
     }
   if (matchVar->whichSlot == NAME_ID)
     {
      returnValue->value = theInstance->name;
      return;
     }
   insSlot =
     &theInstance->slotAddresses
     [theInstance->cls->slotNameMap[matchVar->whichSlot] - 1];

   /* =========================================
      We need to reference the basis slots if
      the slot of this object has changed while
      the RHS was executing

      However, if the reference is being done
      by the LHS of a rule (as a consequence of
      an RHS action), give the pattern matcher
      the real value of the slot
      ========================================= */
   if ((theInstance->basisSlots != NULL) &&
       (! EngineData(theEnv)->JoinOperationInProgress))
     {
      basisSlot = theInstance->basisSlots + (insSlot - theInstance->slotAddresses);
      if (basisSlot->value != NULL)
        insSlot = &basisSlot;
     }

   /* ==================================================
      If we know we are accessing the entire slot,
      the don't bother with searching multifield markers
      or calculating offsets
      ================================================== */
   if (matchVar->allFields)
     {
      returnValue->value = (*insSlot)->value;
      if (returnValue->header->type == MULTIFIELD_TYPE)
        {
         returnValue->begin = 0;
         returnValue->range = (*insSlot)->multifieldValue->length;
        }
      return;
     }

   /* =============================================
      Access a general field in a slot pattern with
      two or more multifield variables
      ============================================= */
      
   extent = SIZE_MAX;
   field = CalculateSlotField(theMarks,*insSlot,matchVar->whichField,&extent);
   if (extent == SIZE_MAX)
     {
      if ((*insSlot)->desc->multiple)
        { returnValue->value = (*insSlot)->multifieldValue->contents[field-1].value; }
      else
        { returnValue->value = (*insSlot)->value; }
     }
   else
     {
      returnValue->value = (*insSlot)->value;
      returnValue->begin = field - 1;
      returnValue->range = extent;
     }
  }

/***************************************************
  NAME         : GetObjectValueSimple
  DESCRIPTION  : Access function for getting
                 pattern variable values within the
                 object pattern and join networks
  INPUTS       : 1) The result data object buffer
                 2) The instance to access
                 3) Data for variable reference
  RETURNS      : Nothing useful
  SIDE EFFECTS : Data object is filled with the
                 values of the pattern variable
  NOTES        : None
 ***************************************************/
static void GetObjectValueSimple(
  Environment *theEnv,
  UDFValue *returnValue,
  Instance *theInstance,
  struct ObjectMatchVar2 *matchVar)
  {
   InstanceSlot **insSlot,*basisSlot;
   Multifield *segmentPtr;
   CLIPSValue *fieldPtr;

   insSlot =
     &theInstance->slotAddresses
     [theInstance->cls->slotNameMap[matchVar->whichSlot] - 1];

   /* =========================================
      We need to reference the basis slots if
      the slot of this object has changed while
      the RHS was executing

      However, if the reference is being done
      by the LHS of a rule (as a consequence of
      an RHS action), give the pattern matcher
      the real value of the slot
      ========================================= */
   if ((theInstance->basisSlots != NULL) &&
       (! EngineData(theEnv)->JoinOperationInProgress))
     {
      basisSlot = theInstance->basisSlots + (insSlot - theInstance->slotAddresses);
      if (basisSlot->value != NULL)
        insSlot = &basisSlot;
     }

   if ((*insSlot)->desc->multiple)
     {
      segmentPtr = (*insSlot)->multifieldValue;
      if (matchVar->fromBeginning)
        {
         if (matchVar->fromEnd)
           {
            returnValue->value = segmentPtr;
            returnValue->begin = matchVar->beginningOffset;
            returnValue->range = segmentPtr->length - (matchVar->endOffset + matchVar->beginningOffset);
           }
         else
           {
            fieldPtr = &segmentPtr->contents[matchVar->beginningOffset];
            returnValue->value = fieldPtr->value;
           }
        }
      else
        {
         fieldPtr = &segmentPtr->contents[segmentPtr->length -
                                           (matchVar->endOffset + 1)];
         returnValue->value = fieldPtr->value;
        }
     }
   else
     {
      returnValue->value = (*insSlot)->value;
     }
  }

/****************************************************
  NAME         : CalculateSlotField
  DESCRIPTION  : Determines the actual index into the
                 an object slot for a given pattern
                 variable
  INPUTS       : 1) The list of markers to examine
                 2) The instance slot (can be NULL)
                 3) The pattern index of the variable
                 4) A buffer in which to store the
                    extent of the pattern variable
                    (-1 for single-field vars)
  RETURNS      : The actual index
  SIDE EFFECTS : None
  NOTES        : None
 ****************************************************/
static size_t CalculateSlotField(
  struct multifieldMarker *theMarkers,
  InstanceSlot *theSlot,
  size_t theIndex,
  size_t *extent)
  {
   size_t actualIndex;
   void *theSlotName;

   actualIndex = theIndex;
   *extent = SIZE_MAX;
      
   if (theSlot == NULL)
     { return actualIndex; }
     
   theSlotName = theSlot->desc->slotName->name;
   while (theMarkers != NULL)
     {
      if (theMarkers->where.whichSlot == theSlotName)
        break;
      theMarkers = theMarkers->next;
     }
   
   while ((theMarkers != NULL) ? (theMarkers->where.whichSlot == theSlotName) : false)
     {
      if (theMarkers->whichField == theIndex)
        {
         *extent = theMarkers->range;

         return actualIndex;
        }

      if (theMarkers->whichField > theIndex)
        { return actualIndex; }
        
      actualIndex = actualIndex + theMarkers->range - 1;
      theMarkers = theMarkers->next;
     }
      
   return actualIndex;
  }

/****************************************************
  NAME         : GetInsMultiSlotField
  DESCRIPTION  : Gets the values of simple single
                 field references in multifield
                 slots for Rete comparisons
  INPUTS       : 1) A multifield field structure
                    to store the type and value in
                 2) The instance
                 3) The id of the slot
                 4) A flag indicating if offset is
                    from beginning or end of
                    multifield slot
                 5) The offset
  RETURNS      : The multifield field
  SIDE EFFECTS : None
  NOTES        : Should only be used to access
                 single-field reference in multifield
                 slots for pattern and join network
                 comparisons
 ****************************************************/
static void GetInsMultiSlotField(
  CLIPSValue *theField,
  Instance *theInstance,
  unsigned theSlotID,
  unsigned fromBeginning,
  unsigned offset)
  {
   InstanceSlot * insSlot;
   Multifield *theSegment;
   CLIPSValue *tmpField;

   insSlot = theInstance->slotAddresses
               [theInstance->cls->slotNameMap[theSlotID] - 1];

   /* Bug fix for 6.05 */

   if (insSlot->desc->multiple)
     {
      theSegment = insSlot->multifieldValue;
      if (fromBeginning)
        tmpField = &theSegment->contents[offset];
      else
        tmpField = &theSegment->contents[theSegment->length - offset - 1];
      theField->value = tmpField->value;
     }
   else
     {
      theField->value = insSlot->value;
     }
  }

#endif

