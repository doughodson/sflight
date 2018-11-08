   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  02/03/18             */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose: Binary Load/Save Functions for Classes and their */
/*             message-handlers                              */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed IMPERATIVE_MESSAGE_HANDLERS and        */
/*            AUXILIARY_MESSAGE_HANDLERS compilation flags.  */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*      6.31: Optimization for marking relevant alpha nodes  */
/*            in the object pattern network.                 */
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

#include <stdlib.h>

#include "setup.h"

#if OBJECT_SYSTEM && (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE)

#include "bload.h"
#include "bsave.h"
#include "classcom.h"
#include "classfun.h"
#include "classini.h"
#include "cstrcbin.h"
#include "cstrnbin.h"
#include "envrnmnt.h"
#include "insfun.h"
#include "memalloc.h"
#include "modulbin.h"
#include "msgcom.h"
#include "msgfun.h"
#include "prntutil.h"
#include "router.h"

#if DEFRULE_CONSTRUCT
#include "objrtbin.h"
#endif

#include "objbin.h"

/* =========================================
   *****************************************
               MACROS AND TYPES
   =========================================
   ***************************************** */

#define SlotIndex(p)             (((p) != NULL) ? (p)->bsaveIndex : ULONG_MAX)
#define SlotNameIndex(p)         (p)->bsaveIndex

#define LinkPointer(i)           (((i) == ULONG_MAX) ? NULL : (Defclass **) &ObjectBinaryData(theEnv)->LinkArray[i])
#define SlotPointer(i)           (((i) == UINT_MAX) ? NULL : (SlotDescriptor *) &ObjectBinaryData(theEnv)->SlotArray[i])
#define TemplateSlotPointer(i)   (((i) == ULONG_MAX) ? NULL : (SlotDescriptor **) &ObjectBinaryData(theEnv)->TmpslotArray[i])
#define OrderedSlotPointer(i)    (((i) == ULONG_MAX) ? NULL : (unsigned *) &ObjectBinaryData(theEnv)->MapslotArray[i])
#define SlotNamePointer(i)       ((SLOT_NAME *) &ObjectBinaryData(theEnv)->SlotNameArray[i])
#define HandlerPointer(i)        (((i) == ULONG_MAX) ? NULL : &ObjectBinaryData(theEnv)->HandlerArray[i])
#define OrderedHandlerPointer(i) (((i) == ULONG_MAX) ? NULL : (unsigned *) &ObjectBinaryData(theEnv)->MaphandlerArray[i])

typedef struct bsaveDefclassModule
  {
   struct bsaveDefmoduleItemHeader header;
  } BSAVE_DEFCLASS_MODULE;

typedef struct bsavePackedClassLinks
  {
   unsigned long classCount;
   unsigned long classArray;
  } BSAVE_PACKED_CLASS_LINKS;

typedef struct bsaveDefclass
  {
   struct bsaveConstructHeader header;
   unsigned abstract : 1;
   unsigned reactive : 1;
   unsigned system   : 1;
   unsigned short id;
   BSAVE_PACKED_CLASS_LINKS directSuperclasses;
   BSAVE_PACKED_CLASS_LINKS directSubclasses;
   BSAVE_PACKED_CLASS_LINKS allSuperclasses;
   unsigned short slotCount;
   unsigned short localInstanceSlotCount;
   unsigned short instanceSlotCount;
   unsigned short maxSlotNameID;
   unsigned short handlerCount;
   unsigned long slots;
   unsigned long instanceTemplate;
   unsigned long slotNameMap;
   unsigned long handlers;
   unsigned long scopeMap;
#if DEFRULE_CONSTRUCT
   unsigned long relevant_terminal_alpha_nodes;
#endif
  } BSAVE_DEFCLASS;

typedef struct bsaveSlotName
  {
   unsigned short id;
   unsigned hashTableIndex;
   unsigned long name;
   unsigned long putHandlerName;
  } BSAVE_SLOT_NAME;

typedef struct bsaveSlotDescriptor
  {
   unsigned shared              : 1;
   unsigned multiple            : 1;
   unsigned composite           : 1;
   unsigned noInherit           : 1;
   unsigned noWrite             : 1;
   unsigned initializeOnly      : 1;
   unsigned dynamicDefault      : 1;
   unsigned noDefault           : 1;
   unsigned reactive            : 1;
   unsigned publicVisibility    : 1;
   unsigned createReadAccessor  : 1;
   unsigned createWriteAccessor : 1;
   unsigned long cls;
   unsigned long slotName,
        defaultValue,
        constraint,
        overrideMessage;
  } BSAVE_SLOT_DESC;

typedef struct bsaveMessageHandler
  {
   struct bsaveConstructHeader header;
   unsigned system : 1;
   unsigned type   : 2;
   unsigned short minParams;
   unsigned short maxParams;
   unsigned short localVarCount;
   unsigned long cls;
   unsigned long actions;
  } BSAVE_HANDLER;

typedef struct handlerBsaveInfo
  {
   DefmessageHandler *handlers;
   unsigned *handlerOrderMap;
   unsigned handlerCount;
  } HANDLER_BSAVE_INFO;

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if BLOAD_AND_BSAVE

   static void                    BsaveObjectsFind(Environment *);
   static void                    MarkDefclassItems(Environment *,ConstructHeader *,void *);
   static void                    BsaveObjectsExpressions(Environment *,FILE *);
   static void                    BsaveDefaultSlotExpressions(Environment *,ConstructHeader *,void *);
   static void                    BsaveHandlerActionExpressions(Environment *,ConstructHeader *,void *);
   static void                    BsaveStorageObjects(Environment *,FILE *);
   static void                    BsaveObjects(Environment *,FILE *);
   static void                    BsaveDefclass(Environment *,ConstructHeader *,void *);
   static void                    BsaveClassLinks(Environment *,ConstructHeader *,void *);
   static void                    BsaveSlots(Environment *,ConstructHeader *,void *);
   static void                    BsaveTemplateSlots(Environment *,ConstructHeader *,void *);
   static void                    BsaveSlotMap(Environment *,ConstructHeader *,void *);
   static void                    BsaveHandlers(Environment *,ConstructHeader *,void *);
   static void                    BsaveHandlerMap(Environment *,ConstructHeader *,void *);

#endif

   static void                    BloadStorageObjects(Environment *);
   static void                    BloadObjects(Environment *);
   static void                    UpdatePrimitiveClassesMap(Environment *);
   static void                    UpdateDefclassModule(Environment *,void *,unsigned long);
   static void                    UpdateDefclass(Environment *,void *,unsigned long);
   static void                    UpdateLink(Environment *,void *,unsigned long);
   static void                    UpdateSlot(Environment *,void *,unsigned long);
   static void                    UpdateSlotName(Environment *,void *,unsigned long);
   static void                    UpdateTemplateSlot(Environment *,void *,unsigned long);
   static void                    UpdateHandler(Environment *,void *,unsigned long);
   static void                    ClearBloadObjects(Environment *);
   static void                    DeallocateObjectBinaryData(Environment *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************
  NAME         : SetupObjectsBload
  DESCRIPTION  : Initializes data structures and
                   routines for binary loads of
                   generic function constructs
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Routines defined and structures initialized
  NOTES        : None
 ***********************************************************/
void SetupObjectsBload(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,OBJECTBIN_DATA,sizeof(struct objectBinaryData),DeallocateObjectBinaryData);

   AddAbortBloadFunction(theEnv,"defclass",CreateSystemClasses,0,NULL);

#if BLOAD_AND_BSAVE
   AddBinaryItem(theEnv,"defclass",0,BsaveObjectsFind,BsaveObjectsExpressions,
                             BsaveStorageObjects,BsaveObjects,
                             BloadStorageObjects,BloadObjects,
                             ClearBloadObjects);
#endif
#if BLOAD || BLOAD_ONLY
   AddBinaryItem(theEnv,"defclass",0,NULL,NULL,NULL,NULL,
                             BloadStorageObjects,BloadObjects,
                             ClearBloadObjects);
#endif

  }

/*******************************************************/
/* DeallocateObjectBinaryData: Deallocates environment */
/*    data for object binary functionality.            */
/*******************************************************/
static void DeallocateObjectBinaryData(
  Environment *theEnv)
  {
   size_t space;
   unsigned long i;

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE) && (! RUN_TIME)

   space = (sizeof(DEFCLASS_MODULE) * ObjectBinaryData(theEnv)->ModuleCount);
   if (space != 0) genfree(theEnv,ObjectBinaryData(theEnv)->ModuleArray,space);

   if (ObjectBinaryData(theEnv)->ClassCount != 0)
     {
      if (DefclassData(theEnv)->ClassIDMap != NULL)
        { rm(theEnv,DefclassData(theEnv)->ClassIDMap,(sizeof(Defclass *) * DefclassData(theEnv)->AvailClassID)); }

      for (i = 0L ; i < ObjectBinaryData(theEnv)->SlotCount ; i++)
        {
         if ((ObjectBinaryData(theEnv)->SlotArray[i].defaultValue != NULL) && (ObjectBinaryData(theEnv)->SlotArray[i].dynamicDefault == 0))
           { rtn_struct(theEnv,udfValue,ObjectBinaryData(theEnv)->SlotArray[i].defaultValue); }
        }

      space = (sizeof(Defclass) * ObjectBinaryData(theEnv)->ClassCount);
      if (space != 0L)
        { genfree(theEnv,ObjectBinaryData(theEnv)->DefclassArray,space); }

      space = (sizeof(Defclass *) * ObjectBinaryData(theEnv)->LinkCount);
      if (space != 0L)
        { genfree(theEnv,ObjectBinaryData(theEnv)->LinkArray,space); }

      space = (sizeof(SlotDescriptor) * ObjectBinaryData(theEnv)->SlotCount);
      if (space != 0L)
        { genfree(theEnv,ObjectBinaryData(theEnv)->SlotArray,space); }

      space = (sizeof(SLOT_NAME) * ObjectBinaryData(theEnv)->SlotNameCount);
      if (space != 0L)
        { genfree(theEnv,ObjectBinaryData(theEnv)->SlotNameArray,space); }

      space = (sizeof(SlotDescriptor *) * ObjectBinaryData(theEnv)->TemplateSlotCount);
      if (space != 0L)
        { genfree(theEnv,ObjectBinaryData(theEnv)->TmpslotArray,space); }

      space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->SlotNameMapCount);
      if (space != 0L)
        { genfree(theEnv,ObjectBinaryData(theEnv)->MapslotArray,space); }
     }

   if (ObjectBinaryData(theEnv)->HandlerCount != 0L)
     {
      space = (sizeof(DefmessageHandler) * ObjectBinaryData(theEnv)->HandlerCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->HandlerArray,space);
         space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->HandlerCount);
         genfree(theEnv,ObjectBinaryData(theEnv)->MaphandlerArray,space);
        }
     }
#endif
  }

/***************************************************
  NAME         : BloadDefclassModuleReference
  DESCRIPTION  : Returns a pointer to the
                 appropriate defclass module
  INPUTS       : The index of the module
  RETURNS      : A pointer to the module
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
void *BloadDefclassModuleReference(
  Environment *theEnv,
  unsigned long theIndex)
  {
   return ((void *) &ObjectBinaryData(theEnv)->ModuleArray[theIndex]);
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

#if BLOAD_AND_BSAVE

/***************************************************************************
  NAME         : BsaveObjectsFind
  DESCRIPTION  : For all classes and their message-handlers, this routine
                   marks all the needed symbols and system functions.
                 Also, it also counts the number of expression structures
                   needed.
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : ExpressionCount (a global from BSAVE.C) is incremented
                   for every expression needed
                 Symbols are marked in their structures
  NOTES        : Also sets bsaveIndex for each class (assumes classes
                   will be bsaved in order of binary list)
 ***************************************************************************/
static void BsaveObjectsFind(
  Environment *theEnv)
  {
   unsigned i;
   SLOT_NAME *snp;

   /* ========================================================
      The counts need to be saved in case a bload is in effect
      ======================================================== */
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->ModuleCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->ClassCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->LinkCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->SlotNameCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->SlotCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->TemplateSlotCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->SlotNameMapCount);
      SaveBloadCount(theEnv,ObjectBinaryData(theEnv)->HandlerCount);

   ObjectBinaryData(theEnv)->ModuleCount= 0L;
   ObjectBinaryData(theEnv)->ClassCount = 0L;
   ObjectBinaryData(theEnv)->SlotCount = 0L;
   ObjectBinaryData(theEnv)->SlotNameCount = 0L;
   ObjectBinaryData(theEnv)->LinkCount = 0L;
   ObjectBinaryData(theEnv)->TemplateSlotCount = 0L;
   ObjectBinaryData(theEnv)->SlotNameMapCount = 0L;
   ObjectBinaryData(theEnv)->HandlerCount = 0L;

   /* ==============================================
      Mark items needed by defclasses in all modules
      ============================================== */
      
   ObjectBinaryData(theEnv)->ModuleCount = GetNumberOfDefmodules(theEnv);
   
   DoForAllConstructs(theEnv,MarkDefclassItems,
                      DefclassData(theEnv)->DefclassModuleIndex,
                      false,NULL);

   /* =============================================
      Mark items needed by canonicalized slot names
      ============================================= */
   for (i = 0 ; i < SLOT_NAME_TABLE_HASH_SIZE ; i++)
     for (snp = DefclassData(theEnv)->SlotNameTable[i] ; snp != NULL ; snp = snp->nxt)
       {
        if ((snp->id != ISA_ID) && (snp->id != NAME_ID))
          {
           snp->bsaveIndex = ObjectBinaryData(theEnv)->SlotNameCount++;
           snp->name->neededSymbol = true;
           snp->putHandlerName->neededSymbol = true;
          }
       }
  }

/***************************************************
  NAME         : MarkDefclassItems
  DESCRIPTION  : Marks needed items for a defclass
  INPUTS       : 1) The defclass
                 2) User buffer (ignored)
  RETURNS      : Nothing useful
  SIDE EFFECTS : Bsave indices set and needed
                 ephemerals marked
  NOTES        : None
 ***************************************************/
static void MarkDefclassItems(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
#if MAC_XCD
#pragma unused(buf)
#endif
   Defclass *cls = (Defclass *) theDefclass;
   long i;
   Expression *tmpexp;

   MarkConstructHeaderNeededItems(&cls->header,ObjectBinaryData(theEnv)->ClassCount++);
   ObjectBinaryData(theEnv)->LinkCount += cls->directSuperclasses.classCount +
                cls->directSubclasses.classCount +
                cls->allSuperclasses.classCount;

#if DEFMODULE_CONSTRUCT
   cls->scopeMap->neededBitMap = true;
#endif

   /* ===================================================
      Mark items needed by slot default value expressions
      =================================================== */
   for (i = 0 ; i < cls->slotCount ; i++)
     {
      cls->slots[i].bsaveIndex = ObjectBinaryData(theEnv)->SlotCount++;
      cls->slots[i].overrideMessage->neededSymbol = true;
      if (cls->slots[i].defaultValue != NULL)
        {
         if (cls->slots[i].dynamicDefault)
           {
            ExpressionData(theEnv)->ExpressionCount +=
              ExpressionSize((Expression *) cls->slots[i].defaultValue);
            MarkNeededItems(theEnv,(Expression *) cls->slots[i].defaultValue);
           }
         else
           {
            /* =================================================
               Static default values are stotred as data objects
               and must be converted into expressions
               ================================================= */
            tmpexp =
              ConvertValueToExpression(theEnv,(UDFValue *) cls->slots[i].defaultValue);
            ExpressionData(theEnv)->ExpressionCount += ExpressionSize(tmpexp);
            MarkNeededItems(theEnv,tmpexp);
            ReturnExpression(theEnv,tmpexp);
           }
        }
     }

   /* ========================================
      Count canonical slots needed by defclass
      ======================================== */
   ObjectBinaryData(theEnv)->TemplateSlotCount += cls->instanceSlotCount;
   if (cls->instanceSlotCount != 0)
     ObjectBinaryData(theEnv)->SlotNameMapCount += cls->maxSlotNameID + 1;

   /* ===============================================
      Mark items needed by defmessage-handler actions
      =============================================== */
   for (i = 0 ; i < cls->handlerCount ; i++)
     {
      cls->handlers[i].header.name->neededSymbol = true;
      ExpressionData(theEnv)->ExpressionCount += ExpressionSize(cls->handlers[i].actions);
      MarkNeededItems(theEnv,cls->handlers[i].actions);
     }
   ObjectBinaryData(theEnv)->HandlerCount += cls->handlerCount;
  }

/***************************************************
  NAME         : BsaveObjectsExpressions
  DESCRIPTION  : Writes out all expressions needed
                   by classes and handlers
  INPUTS       : The file pointer of the binary file
  RETURNS      : Nothing useful
  SIDE EFFECTS : File updated
  NOTES        : None
 ***************************************************/
static void BsaveObjectsExpressions(
  Environment *theEnv,
  FILE *fp)
  {
   if ((ObjectBinaryData(theEnv)->ClassCount == 0L) && (ObjectBinaryData(theEnv)->HandlerCount == 0L))
     return;

   /* ================================================
      Save the defclass slot default value expressions
      ================================================ */
   DoForAllConstructs(theEnv,BsaveDefaultSlotExpressions,DefclassData(theEnv)->DefclassModuleIndex,
                      false,fp);

   /* ==============================================
      Save the defmessage-handler action expressions
      ============================================== */
   DoForAllConstructs(theEnv,BsaveHandlerActionExpressions,DefclassData(theEnv)->DefclassModuleIndex,
                      false,fp);
  }

/***************************************************
  NAME         : BsaveDefaultSlotExpressions
  DESCRIPTION  : Writes expressions for default
                  slot values to binary file
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Slot value expressions written
  NOTES        : None
 ***************************************************/
static void BsaveDefaultSlotExpressions(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   long i;
   Expression *tmpexp;

   for (i = 0 ; i < cls->slotCount ; i++)
     {
      if (cls->slots[i].defaultValue != NULL)
        {
         if (cls->slots[i].dynamicDefault)
           BsaveExpression(theEnv,(Expression *) cls->slots[i].defaultValue,(FILE *) buf);
         else
           {
            /* =================================================
               Static default values are stotred as data objects
               and must be converted into expressions
               ================================================= */
            tmpexp =
              ConvertValueToExpression(theEnv,(UDFValue *) cls->slots[i].defaultValue);
            BsaveExpression(theEnv,tmpexp,(FILE *) buf);
            ReturnExpression(theEnv,tmpexp);
           }
        }
     }
  }

/***************************************************
  NAME         : BsaveHandlerActionExpressions
  DESCRIPTION  : Writes expressions for handler
                  actions to binary file
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Handler actions expressions written
  NOTES        : None
 ***************************************************/
static void BsaveHandlerActionExpressions(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   long i;

   for (i = 0 ; i < cls->handlerCount ; i++)
     BsaveExpression(theEnv,cls->handlers[i].actions,(FILE *) buf);
  }

/*************************************************************************************
  NAME         : BsaveStorageObjects
  DESCRIPTION  : Writes out number of each type of structure required for COOL
                 Space required for counts (unsigned long)
                 Number of class modules (long)
                 Number of classes (long)
                 Number of links to classes (long)
                 Number of slots (long)
                 Number of instance template slots (long)
                 Number of handlers (long)
                 Number of definstances (long)
  INPUTS       : File pointer of binary file
  RETURNS      : Nothing useful
  SIDE EFFECTS : Binary file adjusted
  NOTES        : None
 *************************************************************************************/
static void BsaveStorageObjects(
  Environment *theEnv,
  FILE *fp)
  {
   size_t space;
   long maxClassID;// TBD unsigned short

   if ((ObjectBinaryData(theEnv)->ClassCount == 0L) && (ObjectBinaryData(theEnv)->HandlerCount == 0L))
     {
      space = 0L;
      GenWrite(&space,sizeof(size_t),fp);
      return;
     }
   space = sizeof(long) * 9;
   GenWrite(&space,sizeof(size_t),fp); // 64-bit issue changed long to size_t
   GenWrite(&ObjectBinaryData(theEnv)->ModuleCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->ClassCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->LinkCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->SlotNameCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->SlotCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->TemplateSlotCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->SlotNameMapCount,sizeof(long),fp);
   GenWrite(&ObjectBinaryData(theEnv)->HandlerCount,sizeof(long),fp);
   maxClassID = DefclassData(theEnv)->MaxClassID;
   GenWrite(&maxClassID,sizeof(long),fp);
  }

/*************************************************************************************
  NAME         : BsaveObjects
  DESCRIPTION  : Writes out classes and message-handlers in binary format
                 Space required (unsigned long)
                 Followed by the data structures in order
  INPUTS       : File pointer of binary file
  RETURNS      : Nothing useful
  SIDE EFFECTS : Binary file adjusted
  NOTES        : None
 *************************************************************************************/
static void BsaveObjects(
  Environment *theEnv,
  FILE *fp)
  {
   size_t space;
   Defmodule *theModule;
   DEFCLASS_MODULE *theModuleItem;
   BSAVE_DEFCLASS_MODULE dummy_mitem;
   BSAVE_SLOT_NAME dummy_slot_name;
   SLOT_NAME *snp;
   unsigned i;

   if ((ObjectBinaryData(theEnv)->ClassCount == 0L) && (ObjectBinaryData(theEnv)->HandlerCount == 0L))
     {
      space = 0L;
      GenWrite(&space,sizeof(size_t),fp);
      return;
     }
   space = (ObjectBinaryData(theEnv)->ModuleCount * sizeof(BSAVE_DEFCLASS_MODULE)) +
           (ObjectBinaryData(theEnv)->ClassCount * sizeof(BSAVE_DEFCLASS)) +
           (ObjectBinaryData(theEnv)->LinkCount * sizeof(long)) +
           (ObjectBinaryData(theEnv)->SlotCount * sizeof(BSAVE_SLOT_DESC)) +
           (ObjectBinaryData(theEnv)->SlotNameCount * sizeof(BSAVE_SLOT_NAME)) +
           (ObjectBinaryData(theEnv)->TemplateSlotCount * sizeof(long)) +
           (ObjectBinaryData(theEnv)->SlotNameMapCount * sizeof(unsigned)) +
           (ObjectBinaryData(theEnv)->HandlerCount * sizeof(BSAVE_HANDLER)) +
           (ObjectBinaryData(theEnv)->HandlerCount * sizeof(unsigned));
   GenWrite(&space,sizeof(size_t),fp);

   ObjectBinaryData(theEnv)->ClassCount = 0L;
   ObjectBinaryData(theEnv)->LinkCount = 0L;
   ObjectBinaryData(theEnv)->SlotCount = 0L;
   ObjectBinaryData(theEnv)->SlotNameCount = 0L;
   ObjectBinaryData(theEnv)->TemplateSlotCount = 0L;
   ObjectBinaryData(theEnv)->SlotNameMapCount = 0L;
   ObjectBinaryData(theEnv)->HandlerCount = 0L;

   /* =================================
      Write out each defclass module
      ================================= */
   theModule = GetNextDefmodule(theEnv,NULL);
   while (theModule != NULL)
     {
      theModuleItem = (DEFCLASS_MODULE *)
                      GetModuleItem(theEnv,theModule,FindModuleItem(theEnv,"defclass")->moduleIndex);
      AssignBsaveDefmdlItemHdrVals(&dummy_mitem.header,&theModuleItem->header);
      GenWrite(&dummy_mitem,sizeof(BSAVE_DEFCLASS_MODULE),fp);
      theModule = GetNextDefmodule(theEnv,theModule);
     }

   /* =====================
      Write out the classes
      ===================== */
   DoForAllConstructs(theEnv,BsaveDefclass,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

   /* =========================
      Write out the class links
      ========================= */
   ObjectBinaryData(theEnv)->LinkCount = 0L;
   DoForAllConstructs(theEnv,BsaveClassLinks,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

   /* ===============================
      Write out the slot name entries
      =============================== */
   for (i = 0 ; i < SLOT_NAME_TABLE_HASH_SIZE ; i++)
     for (snp = DefclassData(theEnv)->SlotNameTable[i] ; snp != NULL ; snp = snp->nxt)
     {
      if ((snp->id != ISA_ID) && (snp->id != NAME_ID))
        {
         dummy_slot_name.id = snp->id;
         dummy_slot_name.hashTableIndex = snp->hashTableIndex;
         dummy_slot_name.name = snp->name->bucket;
         dummy_slot_name.putHandlerName = snp->putHandlerName->bucket;
         GenWrite(&dummy_slot_name,sizeof(BSAVE_SLOT_NAME),fp);
        }
     }

   /* ===================
      Write out the slots
      =================== */
   DoForAllConstructs(theEnv,BsaveSlots,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

   /* =====================================
      Write out the template instance slots
      ===================================== */
   DoForAllConstructs(theEnv,BsaveTemplateSlots,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

   /* =============================================
      Write out the ordered instance slot name maps
      ============================================= */
   DoForAllConstructs(theEnv,BsaveSlotMap,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

   /* ==============================
      Write out the message-handlers
      ============================== */
   DoForAllConstructs(theEnv,BsaveHandlers,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

   /* ==========================================
      Write out the ordered message-handler maps
      ========================================== */
   DoForAllConstructs(theEnv,BsaveHandlerMap,DefclassData(theEnv)->DefclassModuleIndex,false,fp);

      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->ModuleCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->ClassCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->LinkCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->SlotCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->SlotNameCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->TemplateSlotCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->SlotNameMapCount);
      RestoreBloadCount(theEnv,&ObjectBinaryData(theEnv)->HandlerCount);
  }

/***************************************************
  NAME         : BsaveDefclass
  DESCRIPTION  : Writes defclass binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass binary data written
  NOTES        : None
 ***************************************************/
static void BsaveDefclass(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   BSAVE_DEFCLASS dummy_class;

   AssignBsaveConstructHeaderVals(&dummy_class.header,&cls->header);
   dummy_class.abstract = cls->abstract;
   dummy_class.reactive = cls->reactive;
   dummy_class.system = cls->system;
   dummy_class.id = cls->id;
   dummy_class.slotCount = cls->slotCount;
   dummy_class.instanceSlotCount = cls->instanceSlotCount;
   dummy_class.localInstanceSlotCount = cls->localInstanceSlotCount;
   dummy_class.maxSlotNameID = cls->maxSlotNameID;
   dummy_class.handlerCount = cls->handlerCount;
   dummy_class.directSuperclasses.classCount = cls->directSuperclasses.classCount;
   dummy_class.directSubclasses.classCount = cls->directSubclasses.classCount;
   dummy_class.allSuperclasses.classCount = cls->allSuperclasses.classCount;
   if (cls->directSuperclasses.classCount != 0)
     {
      dummy_class.directSuperclasses.classArray = ObjectBinaryData(theEnv)->LinkCount;
      ObjectBinaryData(theEnv)->LinkCount += cls->directSuperclasses.classCount;
     }
   else
     dummy_class.directSuperclasses.classArray = ULONG_MAX;
   if (cls->directSubclasses.classCount != 0)
     {
      dummy_class.directSubclasses.classArray = ObjectBinaryData(theEnv)->LinkCount;
      ObjectBinaryData(theEnv)->LinkCount += cls->directSubclasses.classCount;
     }
   else
     dummy_class.directSubclasses.classArray = ULONG_MAX;
   if (cls->allSuperclasses.classCount != 0)
     {
      dummy_class.allSuperclasses.classArray = ObjectBinaryData(theEnv)->LinkCount;
      ObjectBinaryData(theEnv)->LinkCount += cls->allSuperclasses.classCount;
     }
   else
     dummy_class.allSuperclasses.classArray = ULONG_MAX;
   if (cls->slots != NULL)
     {
      dummy_class.slots = ObjectBinaryData(theEnv)->SlotCount;
      ObjectBinaryData(theEnv)->SlotCount += cls->slotCount;
     }
   else
     dummy_class.slots = ULONG_MAX;
   if (cls->instanceTemplate != NULL)
     {
      dummy_class.instanceTemplate = ObjectBinaryData(theEnv)->TemplateSlotCount;
      ObjectBinaryData(theEnv)->TemplateSlotCount += cls->instanceSlotCount;
      dummy_class.slotNameMap = ObjectBinaryData(theEnv)->SlotNameMapCount;
      ObjectBinaryData(theEnv)->SlotNameMapCount += cls->maxSlotNameID + 1;
     }
   else
     {
      dummy_class.instanceTemplate = ULONG_MAX;
      dummy_class.slotNameMap = ULONG_MAX;
     }
   if (cls->handlers != NULL)
     {
      dummy_class.handlers = ObjectBinaryData(theEnv)->HandlerCount;
      ObjectBinaryData(theEnv)->HandlerCount += cls->handlerCount;
     }
   else
     dummy_class.handlers = ULONG_MAX;

#if DEFMODULE_CONSTRUCT
   dummy_class.scopeMap = cls->scopeMap->bucket;
#else
   dummy_class.scopeMap = ULONG_MAX;
#endif

#if DEFRULE_CONSTRUCT
   if (cls->relevant_terminal_alpha_nodes != NULL)
     { dummy_class.relevant_terminal_alpha_nodes = cls->relevant_terminal_alpha_nodes->bsaveID; }
   else
     { dummy_class.relevant_terminal_alpha_nodes = ULONG_MAX; }
#endif

   GenWrite(&dummy_class,sizeof(BSAVE_DEFCLASS),(FILE *) buf);
  }

/***************************************************
  NAME         : BsaveClassLinks
  DESCRIPTION  : Writes class links binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass links binary data written
  NOTES        : None
 ***************************************************/
static void BsaveClassLinks(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   unsigned long i;
   unsigned long dummy_class_index;
   for (i = 0 ;  i < cls->directSuperclasses.classCount ; i++)
     {
      dummy_class_index = DefclassIndex(cls->directSuperclasses.classArray[i]);
      GenWrite(&dummy_class_index,sizeof(long),(FILE *) buf);
     }
   ObjectBinaryData(theEnv)->LinkCount += cls->directSuperclasses.classCount;
   for (i = 0 ;  i < cls->directSubclasses.classCount ; i++)
     {
      dummy_class_index = DefclassIndex(cls->directSubclasses.classArray[i]);
      GenWrite(&dummy_class_index,sizeof(long),(FILE *) buf);
     }
   ObjectBinaryData(theEnv)->LinkCount += cls->directSubclasses.classCount;
   for (i = 0 ;  i < cls->allSuperclasses.classCount ; i++)
     {
      dummy_class_index = DefclassIndex(cls->allSuperclasses.classArray[i]);
      GenWrite(&dummy_class_index,sizeof(long),(FILE *) buf);
     }
   ObjectBinaryData(theEnv)->LinkCount += cls->allSuperclasses.classCount;
  }

/***************************************************
  NAME         : BsaveSlots
  DESCRIPTION  : Writes class slots binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass slots binary data written
  NOTES        : None
 ***************************************************/
static void BsaveSlots(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   long i;
   BSAVE_SLOT_DESC dummy_slot;
   SlotDescriptor *sp;
   Expression *tmpexp;

   for (i = 0 ; i < cls->slotCount ; i++)
     {
      sp = &cls->slots[i];
      dummy_slot.dynamicDefault = sp->dynamicDefault;
      dummy_slot.noDefault = sp->noDefault;
      dummy_slot.shared = sp->shared;
      dummy_slot.multiple = sp->multiple;
      dummy_slot.composite = sp->composite;
      dummy_slot.noInherit = sp->noInherit;
      dummy_slot.noWrite = sp->noWrite;
      dummy_slot.initializeOnly = sp->initializeOnly;
      dummy_slot.reactive = sp->reactive;
      dummy_slot.publicVisibility = sp->publicVisibility;
      dummy_slot.createReadAccessor = sp->createReadAccessor;
      dummy_slot.createWriteAccessor = sp->createWriteAccessor;
      dummy_slot.cls = DefclassIndex(sp->cls);
      dummy_slot.slotName = SlotNameIndex(sp->slotName);
      dummy_slot.overrideMessage = sp->overrideMessage->bucket;
      if (sp->defaultValue != NULL)
        {
         dummy_slot.defaultValue = ExpressionData(theEnv)->ExpressionCount;
         if (sp->dynamicDefault)
           ExpressionData(theEnv)->ExpressionCount += ExpressionSize((Expression *) sp->defaultValue);
         else
           {
            tmpexp = ConvertValueToExpression(theEnv,(UDFValue *) sp->defaultValue);
            ExpressionData(theEnv)->ExpressionCount += ExpressionSize(tmpexp);
            ReturnExpression(theEnv,tmpexp);
           }
        }
      else
        dummy_slot.defaultValue = ULONG_MAX;
      dummy_slot.constraint = ConstraintIndex(sp->constraint);
      GenWrite(&dummy_slot,sizeof(BSAVE_SLOT_DESC),(FILE *) buf);
     }
  }

/**************************************************************
  NAME         : BsaveTemplateSlots
  DESCRIPTION  : Writes class instance template binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass instance template binary data written
  NOTES        : None
 **************************************************************/
static void BsaveTemplateSlots(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   unsigned long i;
   unsigned long tsp;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   for (i = 0 ; i < cls->instanceSlotCount ; i++)
     {
      tsp = SlotIndex(cls->instanceTemplate[i]);
      GenWrite(&tsp,sizeof(unsigned long),(FILE *) buf);
     }
  }

/***************************************************************
  NAME         : BsaveSlotMap
  DESCRIPTION  : Writes class canonical slot map binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass canonical slot map binary data written
  NOTES        : None
 ***************************************************************/
static void BsaveSlotMap(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   if (cls->instanceSlotCount != 0)
     GenWrite(cls->slotNameMap,
              (sizeof(unsigned) * (cls->maxSlotNameID + 1)),(FILE *) buf);
  }

/************************************************************
  NAME         : BsaveHandlers
  DESCRIPTION  : Writes class message-handlers binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass message-handler binary data written
  NOTES        : None
 ************************************************************/
static void BsaveHandlers(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
   unsigned long i;
   BSAVE_HANDLER dummy_handler;
   DefmessageHandler *hnd;

   for (i = 0 ; i < cls->handlerCount ; i++)
     {
      hnd = &cls->handlers[i];
      
      AssignBsaveConstructHeaderVals(&dummy_handler.header,&hnd->header);

      dummy_handler.system = hnd->system;
      dummy_handler.type = hnd->type;
      dummy_handler.minParams = hnd->minParams;
      dummy_handler.maxParams = hnd->maxParams;
      dummy_handler.localVarCount = hnd->localVarCount;
      dummy_handler.cls = DefclassIndex(hnd->cls);
      if (hnd->actions != NULL)
        {
         dummy_handler.actions = ExpressionData(theEnv)->ExpressionCount;
         ExpressionData(theEnv)->ExpressionCount += ExpressionSize(hnd->actions);
        }
      else
        dummy_handler.actions = ULONG_MAX;
      GenWrite(&dummy_handler,sizeof(BSAVE_HANDLER),(FILE *) buf);
     }
  }

/****************************************************************
  NAME         : BsaveHandlerMap
  DESCRIPTION  : Writes class message-handler map binary data
  INPUTS       : 1) The defclass
                 2) The binary file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defclass message-handler map binary data written
  NOTES        : None
 ****************************************************************/
static void BsaveHandlerMap(
  Environment *theEnv,
  ConstructHeader *theDefclass,
  void *buf)
  {
   Defclass *cls = (Defclass *) theDefclass;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   GenWrite(cls->handlerOrderMap,
            (sizeof(unsigned) * cls->handlerCount),(FILE *) buf);
  }

#endif

/***********************************************************************
  NAME         : BloadStorageObjects
  DESCRIPTION  : This routine reads class and handler information from
                 a binary file in five chunks:
                 Class count
                 Handler count
                 Class array
                 Handler array
  INPUTS       : Notthing
  RETURNS      : Nothing useful
  SIDE EFFECTS : Arrays allocated and set
  NOTES        : This routine makes no attempt to reset any pointers
                   within the structures
                 Bload fails if there are still classes in the system!!
 ***********************************************************************/
static void BloadStorageObjects(
  Environment *theEnv)
  {
   size_t space;
   unsigned long counts[9];

   if ((DefclassData(theEnv)->ClassIDMap != NULL) || (DefclassData(theEnv)->MaxClassID != 0))
     {
      SystemError(theEnv,"OBJBIN",1);
      ExitRouter(theEnv,EXIT_FAILURE);
     }
   GenReadBinary(theEnv,&space,sizeof(size_t));
   if (space == 0L)
     {
      ObjectBinaryData(theEnv)->ClassCount = ObjectBinaryData(theEnv)->HandlerCount = 0L;
      return;
     }
   GenReadBinary(theEnv,counts,space);
   ObjectBinaryData(theEnv)->ModuleCount = counts[0];
   ObjectBinaryData(theEnv)->ClassCount = counts[1];
   ObjectBinaryData(theEnv)->LinkCount = counts[2];
   ObjectBinaryData(theEnv)->SlotNameCount = counts[3];
   ObjectBinaryData(theEnv)->SlotCount = counts[4];
   ObjectBinaryData(theEnv)->TemplateSlotCount = counts[5];
   ObjectBinaryData(theEnv)->SlotNameMapCount = counts[6];
   ObjectBinaryData(theEnv)->HandlerCount = counts[7];
   DefclassData(theEnv)->MaxClassID = (unsigned short) counts[8];
   DefclassData(theEnv)->AvailClassID = (unsigned short) counts[8];
   if (ObjectBinaryData(theEnv)->ModuleCount != 0L)
     {
      space = (sizeof(DEFCLASS_MODULE) * ObjectBinaryData(theEnv)->ModuleCount);
      ObjectBinaryData(theEnv)->ModuleArray = (DEFCLASS_MODULE *) genalloc(theEnv,space);
     }
   if (ObjectBinaryData(theEnv)->ClassCount != 0L)
     {
      space = (sizeof(Defclass) * ObjectBinaryData(theEnv)->ClassCount);
      ObjectBinaryData(theEnv)->DefclassArray = (Defclass *) genalloc(theEnv,space);
      DefclassData(theEnv)->ClassIDMap = (Defclass **) gm2(theEnv,(sizeof(Defclass *) * DefclassData(theEnv)->MaxClassID));
     }
   if (ObjectBinaryData(theEnv)->LinkCount != 0L)
     {
      space = (sizeof(Defclass *) * ObjectBinaryData(theEnv)->LinkCount);
      ObjectBinaryData(theEnv)->LinkArray = (Defclass **) genalloc(theEnv,space);
     }
   if (ObjectBinaryData(theEnv)->SlotCount != 0L)
     {
      space = (sizeof(SlotDescriptor) * ObjectBinaryData(theEnv)->SlotCount);
      ObjectBinaryData(theEnv)->SlotArray = (SlotDescriptor *) genalloc(theEnv,space);
     }
   if (ObjectBinaryData(theEnv)->SlotNameCount != 0L)
     {
      space = (sizeof(SLOT_NAME) * ObjectBinaryData(theEnv)->SlotNameCount);
      ObjectBinaryData(theEnv)->SlotNameArray = (SLOT_NAME *) genalloc(theEnv,space);
     }
   if (ObjectBinaryData(theEnv)->TemplateSlotCount != 0L)
     {
      space = (sizeof(SlotDescriptor *) * ObjectBinaryData(theEnv)->TemplateSlotCount);
      ObjectBinaryData(theEnv)->TmpslotArray = (SlotDescriptor **) genalloc(theEnv,space);
     }
   if (ObjectBinaryData(theEnv)->SlotNameMapCount != 0L)
     {
      space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->SlotNameMapCount);
      ObjectBinaryData(theEnv)->MapslotArray = (unsigned *) genalloc(theEnv,space);
     }
   if (ObjectBinaryData(theEnv)->HandlerCount != 0L)
     {
      space = (sizeof(DefmessageHandler) * ObjectBinaryData(theEnv)->HandlerCount);
      ObjectBinaryData(theEnv)->HandlerArray = (DefmessageHandler *) genalloc(theEnv,space);
      space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->HandlerCount);
      ObjectBinaryData(theEnv)->MaphandlerArray = (unsigned *) genalloc(theEnv,space);
     }
  }

/***************************************************************
  NAME         : BloadObjects
  DESCRIPTION  : This routine moves through the class and handler
                   binary arrays updating pointers
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Pointers reset from array indices
  NOTES        : Assumes all loading is finished
 **************************************************************/
static void BloadObjects(
  Environment *theEnv)
  {
   size_t space;

   GenReadBinary(theEnv,&space,sizeof(size_t));
   if (space == 0L)
     return;
   if (ObjectBinaryData(theEnv)->ModuleCount != 0L)
     BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->ModuleCount,sizeof(BSAVE_DEFCLASS_MODULE),UpdateDefclassModule);
   if (ObjectBinaryData(theEnv)->ClassCount != 0L)
     {
      BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->ClassCount,sizeof(BSAVE_DEFCLASS),UpdateDefclass);
      BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->LinkCount,sizeof(long),UpdateLink); // 64-bit bug fix: Defclass * replaced with long
      BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->SlotNameCount,sizeof(BSAVE_SLOT_NAME),UpdateSlotName);
      BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->SlotCount,sizeof(BSAVE_SLOT_DESC),UpdateSlot);
      if (ObjectBinaryData(theEnv)->TemplateSlotCount != 0L)
        BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->TemplateSlotCount,sizeof(long),UpdateTemplateSlot);
      if (ObjectBinaryData(theEnv)->SlotNameMapCount != 0L)
        {
         space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->SlotNameMapCount);
         GenReadBinary(theEnv,ObjectBinaryData(theEnv)->MapslotArray,space);
        }
      if (ObjectBinaryData(theEnv)->HandlerCount != 0L)
        {
         BloadandRefresh(theEnv,ObjectBinaryData(theEnv)->HandlerCount,sizeof(BSAVE_HANDLER),UpdateHandler);
         space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->HandlerCount);
         GenReadBinary(theEnv,ObjectBinaryData(theEnv)->MaphandlerArray,space);
        }
      UpdatePrimitiveClassesMap(theEnv);
     }
  }

/***************************************************
  NAME         : UpdatePrimitiveClassesMap
  DESCRIPTION  : Resets the pointers for the global
                 primitive classes map
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : PrimitiveClassMap pointers set
                 into bload array
  NOTES        : Looks at first nine primitive type
                 codes in the source file CONSTANT.H
 ***************************************************/
static void UpdatePrimitiveClassesMap(
  Environment *theEnv)
  {
   unsigned i;

   for (i = 0 ; i < OBJECT_TYPE_CODE ; i++)
     DefclassData(theEnv)->PrimitiveClassMap[i] = (Defclass *) &ObjectBinaryData(theEnv)->DefclassArray[i];
  }

/*********************************************************
  Refresh update routines for bsaved COOL structures
 *********************************************************/
static void UpdateDefclassModule(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   BSAVE_DEFCLASS_MODULE *bdptr;

   bdptr = (BSAVE_DEFCLASS_MODULE *) buf;
   UpdateDefmoduleItemHeader(theEnv,&bdptr->header,&ObjectBinaryData(theEnv)->ModuleArray[obji].header,
                             sizeof(Defclass),ObjectBinaryData(theEnv)->DefclassArray);
  }

static void UpdateDefclass(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   BSAVE_DEFCLASS *bcls;
   Defclass *cls;

   bcls = (BSAVE_DEFCLASS *) buf;
   cls = &ObjectBinaryData(theEnv)->DefclassArray[obji];

   UpdateConstructHeader(theEnv,&bcls->header,&cls->header,DEFCLASS,
                         sizeof(DEFCLASS_MODULE),ObjectBinaryData(theEnv)->ModuleArray,
                         sizeof(Defclass),ObjectBinaryData(theEnv)->DefclassArray);
   cls->abstract = bcls->abstract;
   cls->reactive = bcls->reactive;
   cls->system = bcls->system;
   cls->id = bcls->id;
   DefclassData(theEnv)->ClassIDMap[cls->id] = cls;
#if DEBUGGING_FUNCTIONS
   cls->traceInstances = DefclassData(theEnv)->WatchInstances;
   cls->traceSlots = DefclassData(theEnv)->WatchSlots;
#endif
   cls->slotCount = bcls->slotCount;
   cls->instanceSlotCount = bcls->instanceSlotCount;
   cls->localInstanceSlotCount = bcls->localInstanceSlotCount;
   cls->maxSlotNameID = bcls->maxSlotNameID;
   cls->handlerCount = bcls->handlerCount;
   cls->directSuperclasses.classCount = bcls->directSuperclasses.classCount;
   cls->directSuperclasses.classArray = LinkPointer(bcls->directSuperclasses.classArray);
   cls->directSubclasses.classCount = bcls->directSubclasses.classCount;
   cls->directSubclasses.classArray = LinkPointer(bcls->directSubclasses.classArray);
   cls->allSuperclasses.classCount = bcls->allSuperclasses.classCount;
   cls->allSuperclasses.classArray = LinkPointer(bcls->allSuperclasses.classArray);
   cls->slots = SlotPointer(bcls->slots);
   cls->instanceTemplate = TemplateSlotPointer(bcls->instanceTemplate);
   cls->slotNameMap = OrderedSlotPointer(bcls->slotNameMap);
   cls->instanceList = NULL;
   cls->handlers = HandlerPointer(bcls->handlers);
   cls->handlerOrderMap = OrderedHandlerPointer(bcls->handlers);
   cls->installed = 1;
   cls->busy = 0;
   cls->instanceList = NULL;
   cls->instanceListBottom = NULL;
   cls->relevant_terminal_alpha_nodes = ClassAlphaPointer(bcls->relevant_terminal_alpha_nodes);
#if DEFMODULE_CONSTRUCT
   cls->scopeMap = BitMapPointer(bcls->scopeMap);
   IncrementBitMapCount(cls->scopeMap);
#else
   cls->scopeMap = NULL;
#endif
   PutClassInTable(theEnv,cls);
  }

static void UpdateLink(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   unsigned long *blink;

   blink = (unsigned long *) buf;
   ObjectBinaryData(theEnv)->LinkArray[obji] = DefclassPointer(*blink);
  }

static void UpdateSlot(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   SlotDescriptor *sp;
   BSAVE_SLOT_DESC *bsp;

   sp = (SlotDescriptor *) &ObjectBinaryData(theEnv)->SlotArray[obji];
   bsp = (BSAVE_SLOT_DESC *) buf;
   sp->dynamicDefault = bsp->dynamicDefault;
   sp->noDefault = bsp->noDefault;
   sp->shared = bsp->shared;
   sp->multiple = bsp->multiple;
   sp->composite = bsp->composite;
   sp->noInherit = bsp->noInherit;
   sp->noWrite = bsp->noWrite;
   sp->initializeOnly = bsp->initializeOnly;
   sp->reactive = bsp->reactive;
   sp->publicVisibility = bsp->publicVisibility;
   sp->createReadAccessor = bsp->createReadAccessor;
   sp->createWriteAccessor = bsp->createWriteAccessor;
   sp->cls = DefclassPointer(bsp->cls);
   sp->slotName = SlotNamePointer(bsp->slotName);
   sp->overrideMessage = SymbolPointer(bsp->overrideMessage);
   IncrementLexemeCount(sp->overrideMessage);
   if (bsp->defaultValue != ULONG_MAX)
     {
      if (sp->dynamicDefault)
        sp->defaultValue = ExpressionPointer(bsp->defaultValue);
      else
        {
         sp->defaultValue = get_struct(theEnv,udfValue);
         EvaluateAndStoreInDataObject(theEnv,sp->multiple,ExpressionPointer(bsp->defaultValue),
                                      (UDFValue *) sp->defaultValue,true);
         RetainUDFV(theEnv,(UDFValue *) sp->defaultValue);
        }
     }
   else
     sp->defaultValue = NULL;
   sp->constraint = ConstraintPointer(bsp->constraint);
   sp->sharedCount = 0;
   sp->sharedValue.value = NULL;
   sp->bsaveIndex = 0L;
   if (sp->shared)
     {
      sp->sharedValue.desc = sp;
      sp->sharedValue.value = NULL;
     }
  }

static void UpdateSlotName(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   SLOT_NAME *snp;
   BSAVE_SLOT_NAME *bsnp;

   bsnp = (BSAVE_SLOT_NAME *) buf;
   snp = (SLOT_NAME *) &ObjectBinaryData(theEnv)->SlotNameArray[obji];
   snp->id = bsnp->id;
   snp->name = SymbolPointer(bsnp->name);
   IncrementLexemeCount(snp->name);
   snp->putHandlerName = SymbolPointer(bsnp->putHandlerName);
   IncrementLexemeCount(snp->putHandlerName);
   snp->hashTableIndex = bsnp->hashTableIndex;
   snp->nxt = DefclassData(theEnv)->SlotNameTable[snp->hashTableIndex];
   DefclassData(theEnv)->SlotNameTable[snp->hashTableIndex] = snp;
  }

static void UpdateTemplateSlot(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   ObjectBinaryData(theEnv)->TmpslotArray[obji] = SlotPointer(* (long *) buf);
  }

static void UpdateHandler(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   DefmessageHandler *hnd;
   BSAVE_HANDLER *bhnd;

   hnd = &ObjectBinaryData(theEnv)->HandlerArray[obji];
   bhnd = (BSAVE_HANDLER *) buf;
   hnd->system = bhnd->system;
   hnd->type = bhnd->type;

   UpdateConstructHeader(theEnv,&bhnd->header,&hnd->header,DEFMESSAGE_HANDLER,
                         sizeof(DEFCLASS_MODULE),ObjectBinaryData(theEnv)->ModuleArray,
                         sizeof(DefmessageHandler),ObjectBinaryData(theEnv)->HandlerArray);

   hnd->minParams = bhnd->minParams;
   hnd->maxParams = bhnd->maxParams;
   hnd->localVarCount = bhnd->localVarCount;
   hnd->cls = DefclassPointer(bhnd->cls);
   //IncrementLexemeCount(hnd->header.name);
   hnd->actions = ExpressionPointer(bhnd->actions);
   hnd->header.ppForm = NULL;
   hnd->busy = 0;
   hnd->mark = 0;
   hnd->header.usrData = NULL;
#if DEBUGGING_FUNCTIONS
   hnd->trace = MessageHandlerData(theEnv)->WatchHandlers;
#endif
  }

/***************************************************************
  NAME         : ClearBloadObjects
  DESCRIPTION  : Release all binary-loaded class and handler
                   structure arrays (and others)
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Memory cleared
  NOTES        : None
 ***************************************************************/
static void ClearBloadObjects(
  Environment *theEnv)
  {
   unsigned long i;
   size_t space;

   space = (sizeof(DEFCLASS_MODULE) * ObjectBinaryData(theEnv)->ModuleCount);
   if (space == 0L)
     return;
   genfree(theEnv,ObjectBinaryData(theEnv)->ModuleArray,space);
   ObjectBinaryData(theEnv)->ModuleArray = NULL;
   ObjectBinaryData(theEnv)->ModuleCount = 0L;

   if (ObjectBinaryData(theEnv)->ClassCount != 0L)
     {
      rm(theEnv,DefclassData(theEnv)->ClassIDMap,(sizeof(Defclass *) * DefclassData(theEnv)->AvailClassID));
      DefclassData(theEnv)->ClassIDMap = NULL;
      DefclassData(theEnv)->MaxClassID = 0;
      DefclassData(theEnv)->AvailClassID = 0;
      for (i = 0 ; i < ObjectBinaryData(theEnv)->ClassCount ; i++)
        {
         UnmarkConstructHeader(theEnv,&ObjectBinaryData(theEnv)->DefclassArray[i].header);
#if DEFMODULE_CONSTRUCT
         DecrementBitMapReferenceCount(theEnv,ObjectBinaryData(theEnv)->DefclassArray[i].scopeMap);
#endif
         RemoveClassFromTable(theEnv,&ObjectBinaryData(theEnv)->DefclassArray[i]);
        }
      for (i = 0 ; i < ObjectBinaryData(theEnv)->SlotCount ; i++)
        {
         ReleaseLexeme(theEnv,ObjectBinaryData(theEnv)->SlotArray[i].overrideMessage);
         if ((ObjectBinaryData(theEnv)->SlotArray[i].defaultValue != NULL) && (ObjectBinaryData(theEnv)->SlotArray[i].dynamicDefault == 0))
           {
            ReleaseUDFV(theEnv,(UDFValue *) ObjectBinaryData(theEnv)->SlotArray[i].defaultValue);
            rtn_struct(theEnv,udfValue,ObjectBinaryData(theEnv)->SlotArray[i].defaultValue);
           }
        }
      for (i = 0 ; i < ObjectBinaryData(theEnv)->SlotNameCount ; i++)
        {
         DefclassData(theEnv)->SlotNameTable[ObjectBinaryData(theEnv)->SlotNameArray[i].hashTableIndex] = NULL;
         ReleaseLexeme(theEnv,ObjectBinaryData(theEnv)->SlotNameArray[i].name);
         ReleaseLexeme(theEnv,ObjectBinaryData(theEnv)->SlotNameArray[i].putHandlerName);
        }

      space = (sizeof(Defclass) * ObjectBinaryData(theEnv)->ClassCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->DefclassArray,space);
         ObjectBinaryData(theEnv)->DefclassArray = NULL;
         ObjectBinaryData(theEnv)->ClassCount = 0L;
        }

      space = (sizeof(Defclass *) * ObjectBinaryData(theEnv)->LinkCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->LinkArray,space);
         ObjectBinaryData(theEnv)->LinkArray = NULL;
         ObjectBinaryData(theEnv)->LinkCount = 0L;
        }

      space = (sizeof(SlotDescriptor) * ObjectBinaryData(theEnv)->SlotCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->SlotArray,space);
         ObjectBinaryData(theEnv)->SlotArray = NULL;
         ObjectBinaryData(theEnv)->SlotCount = 0L;
        }

      space = (sizeof(SLOT_NAME) * ObjectBinaryData(theEnv)->SlotNameCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->SlotNameArray,space);
         ObjectBinaryData(theEnv)->SlotNameArray = NULL;
         ObjectBinaryData(theEnv)->SlotNameCount = 0L;
        }

      space = (sizeof(SlotDescriptor *) * ObjectBinaryData(theEnv)->TemplateSlotCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->TmpslotArray,space);
         ObjectBinaryData(theEnv)->TmpslotArray = NULL;
         ObjectBinaryData(theEnv)->TemplateSlotCount = 0L;
        }

      space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->SlotNameMapCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->MapslotArray,space);
         ObjectBinaryData(theEnv)->MapslotArray = NULL;
         ObjectBinaryData(theEnv)->SlotNameMapCount = 0L;
        }
     }

   if (ObjectBinaryData(theEnv)->HandlerCount != 0L)
     {
      for (i = 0L ; i < ObjectBinaryData(theEnv)->HandlerCount ; i++)
        ReleaseLexeme(theEnv,ObjectBinaryData(theEnv)->HandlerArray[i].header.name);

      space = (sizeof(DefmessageHandler) * ObjectBinaryData(theEnv)->HandlerCount);
      if (space != 0L)
        {
         genfree(theEnv,ObjectBinaryData(theEnv)->HandlerArray,space);
         ObjectBinaryData(theEnv)->HandlerArray = NULL;
         space = (sizeof(unsigned) * ObjectBinaryData(theEnv)->HandlerCount);
         genfree(theEnv,ObjectBinaryData(theEnv)->MaphandlerArray,space);
         ObjectBinaryData(theEnv)->MaphandlerArray = NULL;
         ObjectBinaryData(theEnv)->HandlerCount = 0L;
        }
     }
  }

#endif

