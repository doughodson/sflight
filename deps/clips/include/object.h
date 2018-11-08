   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*               CLIPS Version 6.40  02/03/18          */
   /*                                                     */
   /*                OBJECT SYSTEM DEFINITIONS            */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*      6.31: Optimization for marking relevant alpha nodes  */
/*            in the object pattern network.                 */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*************************************************************/

#ifndef _H_object

#pragma once

#define _H_object

typedef struct defclassModule DEFCLASS_MODULE;
typedef struct defclass Defclass;
typedef struct packedClassLinks PACKED_CLASS_LINKS;
typedef struct classLink CLASS_LINK;
typedef struct slotName SLOT_NAME;
typedef struct slotDescriptor SlotDescriptor;
typedef struct defmessageHandler DefmessageHandler;

typedef struct instanceSlot InstanceSlot;

typedef struct instanceBuilder InstanceBuilder;
typedef struct instanceModifier InstanceModifier;

/* Maximum # of simultaneous class hierarchy traversals
   should be a multiple of BITS_PER_BYTE and less than MAX_INT      */

#define MAX_TRAVERSALS  256
#define TRAVERSAL_BYTES 32       /* (MAX_TRAVERSALS / BITS_PER_BYTE) */

#define VALUE_REQUIRED     0
#define VALUE_PROHIBITED   1
#define VALUE_NOT_REQUIRED 2

#include "entities.h"
#include "constrct.h"
#include "constrnt.h"
#include "moduldef.h"
#include "evaluatn.h"
#include "expressn.h"
#include "multifld.h"
#include "symbol.h"
#include "match.h"

#if DEFRULE_CONSTRUCT
#include "objrtmch.h"
#endif

struct packedClassLinks
  {
   unsigned long classCount;
   Defclass **classArray;
  };

struct defclassModule
  {
   struct defmoduleItemHeader header;
  };

struct defclass
  {
   ConstructHeader header;
   unsigned installed      : 1;
   unsigned system         : 1;
   unsigned abstract       : 1;
   unsigned reactive       : 1;
   unsigned traceInstances : 1;
   unsigned traceSlots     : 1;
   unsigned short id;
   unsigned busy;
   unsigned hashTableIndex;
   PACKED_CLASS_LINKS directSuperclasses;
   PACKED_CLASS_LINKS directSubclasses;
   PACKED_CLASS_LINKS allSuperclasses;
   SlotDescriptor *slots;
   SlotDescriptor **instanceTemplate;
   unsigned *slotNameMap;
   unsigned short slotCount;
   unsigned short localInstanceSlotCount;
   unsigned short instanceSlotCount;
   unsigned short maxSlotNameID;
   Instance *instanceList;
   Instance *instanceListBottom;
   DefmessageHandler *handlers;
   unsigned *handlerOrderMap;
   unsigned short handlerCount;
   Defclass *nxtHash;
   CLIPSBitMap *scopeMap;

   /*
    * Links this defclass to each of the terminal alpha nodes which could be
    * affected by a modification to an instance of it. This saves having to
    * iterate through every single terminal alpha for every single modification
    * to an instance of a defclass.
    */
#if DEFRULE_CONSTRUCT
   CLASS_ALPHA_LINK *relevant_terminal_alpha_nodes;
#endif

   char traversalRecord[TRAVERSAL_BYTES];
  };

struct classLink
  {
   Defclass *cls;
   struct classLink *nxt;
  };

struct slotName
  {
   unsigned hashTableIndex;
   unsigned use;
   unsigned short id;
   CLIPSLexeme *name;
   CLIPSLexeme *putHandlerName;
   struct slotName *nxt;
   unsigned long bsaveIndex;
  };

struct instanceSlot
  {
   SlotDescriptor *desc;
   unsigned valueRequired : 1;
   unsigned override      : 1;
   unsigned short type;
   union
     {
      void *value;
      TypeHeader *header;
      CLIPSLexeme *lexemeValue;
      CLIPSFloat *floatValue;
      CLIPSInteger *integerValue;
      CLIPSVoid *voidValue;
      Fact *factValue;
      Instance *instanceValue;
      Multifield *multifieldValue;
      CLIPSExternalAddress *externalAddressValue;
     };
  };

struct slotDescriptor
  {
   unsigned shared                   : 1;
   unsigned multiple                 : 1;
   unsigned composite                : 1;
   unsigned noInherit                : 1;
   unsigned noWrite                  : 1;
   unsigned initializeOnly           : 1;
   unsigned dynamicDefault           : 1;
   unsigned defaultSpecified         : 1;
   unsigned noDefault                : 1;
   unsigned reactive                 : 1;
   unsigned publicVisibility         : 1;
   unsigned createReadAccessor       : 1;
   unsigned createWriteAccessor      : 1;
   unsigned overrideMessageSpecified : 1;
   Defclass *cls;
   SLOT_NAME *slotName;
   CLIPSLexeme *overrideMessage;
   void *defaultValue;
   CONSTRAINT_RECORD *constraint;
   unsigned sharedCount;
   unsigned long bsaveIndex;
   InstanceSlot sharedValue;
  };

struct instance
  {
   union
     {
      struct patternEntity patternHeader;
      TypeHeader header;
     };
   void *partialMatchList;
   InstanceSlot *basisSlots;
   unsigned installed            : 1;
   unsigned garbage              : 1;
   unsigned initSlotsCalled      : 1;
   unsigned initializeInProgress : 1;
   unsigned reteSynchronized     : 1;
   CLIPSLexeme *name;
   unsigned hashTableIndex;
   unsigned busy;
   Defclass *cls;
   Instance *prvClass,*nxtClass,
                 *prvHash,*nxtHash,
                 *prvList,*nxtList;
   InstanceSlot **slotAddresses,
                 *slots;
  };

struct defmessageHandler
  {
   ConstructHeader header;
   unsigned system         : 1;
   unsigned type           : 2;
   unsigned mark           : 1;
   unsigned trace          : 1;
   unsigned busy;
   Defclass *cls;
   unsigned short minParams;
   unsigned short maxParams;
   unsigned short localVarCount;
   Expression *actions;
  };

struct instanceBuilder
  {
   Environment *ibEnv;
   Defclass *ibDefclass;
   CLIPSValue *ibValueArray;
  };

struct instanceModifier
  {
   Environment *imEnv;
   Instance *imOldInstance;
   CLIPSValue *imValueArray;
   char *changeMap;
  };

#endif /* _H_object */





