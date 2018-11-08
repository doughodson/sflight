   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  11/01/16            */
   /*                                                     */
   /*               DEFTEMPLATE HEADER FILE               */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Added support for templates maintaining their  */
/*            own list of facts.                             */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Corrected code to remove run-time program      */
/*            compiler warnings.                             */
/*                                                           */
/*      6.30: Added code for deftemplate run time            */
/*            initialization of hashed comparisons to        */
/*            constants.                                     */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Support for deftemplate slot facets.           */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Changed find construct functionality so that   */
/*            imported modules are search when locating a    */
/*            named construct.                               */
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
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*************************************************************/

#ifndef _H_tmpltdef

#pragma once

#define _H_tmpltdef

typedef struct deftemplate Deftemplate;

struct templateSlot;
struct deftemplateModule;

#include "entities.h"

#include "constrct.h"
#include "conscomp.h"
#include "evaluatn.h"
#include "moduldef.h"

struct deftemplateModule
  {
   struct defmoduleItemHeader header;
  };

#include "constrnt.h"
#include "factbld.h"

struct deftemplate
  {
   ConstructHeader header;
   struct templateSlot *slotList;
   unsigned int implied       : 1;
   unsigned int watch         : 1;
   unsigned int inScope       : 1;
   unsigned short numberOfSlots;
   long busyCount;
   struct factPatternNode *patternNetwork;
   Fact *factList;
   Fact *lastFact;
  };

struct templateSlot
  {
   CLIPSLexeme *slotName;
   unsigned int multislot : 1;
   unsigned int noDefault : 1;
   unsigned int defaultPresent : 1;
   unsigned int defaultDynamic : 1;
   CONSTRAINT_RECORD *constraints;
   Expression *defaultList;
   Expression *facetList;
   struct templateSlot *next;
  };


#define DEFTEMPLATE_DATA 5

struct deftemplateData
  {
   Construct *DeftemplateConstruct;
   unsigned int DeftemplateModuleIndex;
   struct entityRecord DeftemplatePtrRecord;
#if DEBUGGING_FUNCTIONS
   int DeletedTemplateDebugFlags;
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
   struct CodeGeneratorItem *DeftemplateCodeItem;
#endif
#if (! RUN_TIME) && (! BLOAD_ONLY)
   bool DeftemplateError;
#endif
  };

#define DeftemplateData(theEnv) ((struct deftemplateData *) GetEnvironmentData(theEnv,DEFTEMPLATE_DATA))

   void                           InitializeDeftemplates(Environment *);
   Deftemplate                   *FindDeftemplate(Environment *,const char *);
   Deftemplate                   *FindDeftemplateInModule(Environment *,const char *);
   Deftemplate                   *GetNextDeftemplate(Environment *,Deftemplate *);
   bool                           DeftemplateIsDeletable(Deftemplate *);
   Fact                          *GetNextFactInTemplate(Deftemplate *,Fact *);
   struct deftemplateModule      *GetDeftemplateModuleItem(Environment *,Defmodule *);
   void                           ReturnSlots(Environment *,struct templateSlot *);
   void                           IncrementDeftemplateBusyCount(Environment *,Deftemplate *);
   void                           DecrementDeftemplateBusyCount(Environment *,Deftemplate *);
   void                          *CreateDeftemplateScopeMap(Environment *,Deftemplate *);
#if RUN_TIME
   void                           DeftemplateRunTimeInitialize(Environment *);
#endif
   const char                    *DeftemplateModule(Deftemplate *);
   const char                    *DeftemplateName(Deftemplate *);
   const char                    *DeftemplatePPForm(Deftemplate *);

#endif /* _H_tmpltdef */


