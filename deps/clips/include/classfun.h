   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  01/21/18            */
   /*                                                     */
   /*             CLASS FUNCTIONS HEADER FILE             */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Borland C (IBM_TBC) and Metrowerks CodeWarrior */
/*            (MAC_MCW, IBM_MCW) are no longer supported.    */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Used genstrcpy and genstrcat instead of strcpy */
/*            and strcat.                                    */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.31: Optimization of slot ID creation previously    */
/*            provided by NewSlotNameID function.            */
/*                                                           */
/*      6.40: Removed LOCALE definition.                     */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            Removed initial-object support.                */
/*                                                           */
/*************************************************************/

#ifndef _H_classfun

#pragma once

#define _H_classfun

#include "object.h"
#include "scanner.h"

#define TestTraversalID(traversalRecord,id) TestBitMap(traversalRecord,id)
#define SetTraversalID(traversalRecord,id) SetBitMap(traversalRecord,id)
#define ClearTraversalID(traversalRecord,id) ClearBitMap(traversalRecord,id)

#define CLASS_TABLE_HASH_SIZE     167 // TBD Larger?
#define SLOT_NAME_TABLE_HASH_SIZE 167 // TBD Larger?

#define ISA_ID  0
#define NAME_ID 1

#define SLOT_NAME_NOT_FOUND USHRT_MAX

   void                           IncrementDefclassBusyCount(Environment *,Defclass *);
   void                           DecrementDefclassBusyCount(Environment *,Defclass *);
   bool                           InstancesPurge(Environment *,void *);

#if ! RUN_TIME
   void                           InitializeClasses(Environment *);
#endif
   SlotDescriptor                *FindClassSlot(Defclass *,CLIPSLexeme *);
   void                           ClassExistError(Environment *,const char *,const char *);
   void                           DeleteClassLinks(Environment *,CLASS_LINK *);
   void                           PrintClassName(Environment *,const char *,Defclass *,bool,bool);

#if DEBUGGING_FUNCTIONS || ((! BLOAD_ONLY) && (! RUN_TIME))
   void                           PrintPackedClassLinks(Environment *,const char *,const char *,PACKED_CLASS_LINKS *);
#endif

#if ! RUN_TIME
   void                           PutClassInTable(Environment *,Defclass *);
   void                           RemoveClassFromTable(Environment *,Defclass *);
   void                           AddClassLink(Environment *,PACKED_CLASS_LINKS *,Defclass *,bool,unsigned int);
   void                           DeleteSubclassLink(Environment *,Defclass *,Defclass *);
   void                           DeleteSuperclassLink(Environment *,Defclass *,Defclass *);
   Defclass                      *NewClass(Environment *,CLIPSLexeme *);
   void                           DeletePackedClassLinks(Environment *,PACKED_CLASS_LINKS *,bool);
   void                           AssignClassID(Environment *,Defclass *);
   SLOT_NAME                     *AddSlotName(Environment *,CLIPSLexeme *,unsigned short,bool);
   void                           DeleteSlotName(Environment *,SLOT_NAME *);
   void                           RemoveDefclass(Environment *,Defclass *);
   void                           InstallClass(Environment *,Defclass *,bool);
#endif
   void                           DestroyDefclass(Environment *,Defclass *);

#if (! BLOAD_ONLY) && (! RUN_TIME)
   bool                           IsClassBeingUsed(Defclass *);
   bool                           RemoveAllUserClasses(Environment *);
   bool                           DeleteClassUAG(Environment *,Defclass *);
   void                           MarkBitMapSubclasses(char *,Defclass *,int);
#endif

   unsigned short                 FindSlotNameID(Environment *,CLIPSLexeme *);
   CLIPSLexeme                   *FindIDSlotName(Environment *,unsigned short);
   SLOT_NAME                     *FindIDSlotNameHash(Environment *,unsigned short); 
   int                            GetTraversalID(Environment *);
   void                           ReleaseTraversalID(Environment *);
   unsigned int                   HashClass(CLIPSLexeme *);

#define DEFCLASS_DATA 21

#define PRIMITIVE_CLASSES 9

#include "classcom.h"

struct defclassData
  {
   Construct *DefclassConstruct;
   unsigned DefclassModuleIndex;
   EntityRecord DefclassEntityRecord;
   Defclass *PrimitiveClassMap[PRIMITIVE_CLASSES];
   Defclass **ClassIDMap;
   Defclass **ClassTable;
   unsigned short MaxClassID;
   unsigned short AvailClassID;
   SLOT_NAME **SlotNameTable;
   CLIPSLexeme *ISA_SYMBOL;
   CLIPSLexeme *NAME_SYMBOL;
#if DEBUGGING_FUNCTIONS
   bool WatchInstances;
   bool WatchSlots;
#endif
   unsigned short CTID;
   struct token ObjectParseToken;
   ClassDefaultsMode ClassDefaultsModeValue;
   int newSlotID;
  };

#define DefclassData(theEnv) ((struct defclassData *) GetEnvironmentData(theEnv,DEFCLASS_DATA))

#endif









