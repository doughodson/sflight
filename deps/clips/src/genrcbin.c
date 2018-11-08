   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/30/16             */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose: Binary Load/Save Functions for Generic Functions */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Changed integer type/precision.                */
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

#if DEFGENERIC_CONSTRUCT && (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE)

#include "bload.h"
#include "bsave.h"
#include "constant.h"
#include "cstrcbin.h"
#include "cstrccom.h"
#include "envrnmnt.h"
#include "genrccom.h"
#include "memalloc.h"
#include "modulbin.h"
#if OBJECT_SYSTEM
#include "objbin.h"
#else
#include "prntutil.h"
#endif
#include "router.h"

#include "genrcbin.h"

/* =========================================
   *****************************************
               MACROS AND TYPES
   =========================================
   ***************************************** */

#define MethodPointer(i) (((i) == ULONG_MAX) ? NULL : (&DefgenericBinaryData(theEnv)->MethodArray[i]))
#define RestrictionPointer(i) (((i) == ULONG_MAX) ? NULL : (RESTRICTION *) &DefgenericBinaryData(theEnv)->RestrictionArray[i])
#define TypePointer(i) (((i) == ULONG_MAX) ? NULL : (void **) &DefgenericBinaryData(theEnv)->TypeArray[i])

typedef struct bsaveRestriction
  {
   unsigned long types;
   unsigned long query;
   unsigned short tcnt;
  } BSAVE_RESTRICTION;

typedef struct bsaveMethod
  {
   struct bsaveConstructHeader header;
   unsigned short index;
   unsigned short restrictionCount;
   unsigned short minRestrictions, maxRestrictions;
   unsigned short localVarCount;
   unsigned system;
   unsigned long restrictions;
   unsigned long actions;
  } BSAVE_METHOD;

typedef struct bsaveGenericFunc
  {
   struct bsaveConstructHeader header;
   unsigned long methods;
   unsigned short mcnt;
  } BSAVE_GENERIC;

typedef struct bsaveGenericModule
  {
   struct bsaveDefmoduleItemHeader header;
  } BSAVE_DEFGENERIC_MODULE;

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if BLOAD_AND_BSAVE

   static void                    BsaveGenericsFind(Environment *);
   static void                    MarkDefgenericItems(Environment *,ConstructHeader *,void *);
   static void                    BsaveGenericsExpressions(Environment *,FILE *);
   static void                    BsaveMethodExpressions(Environment *,ConstructHeader *,void *);
   static void                    BsaveRestrictionExpressions(Environment *,ConstructHeader *,void *);
   static void                    BsaveGenerics(Environment *,FILE *);
   static void                    BsaveDefgenericHeader(Environment *,ConstructHeader *,void *);
   static void                    BsaveMethods(Environment *,ConstructHeader *,void *);
   static void                    BsaveMethodRestrictions(Environment *,ConstructHeader *,void *);
   static void                    BsaveRestrictionTypes(Environment *,ConstructHeader *,void *);
   static void                    BsaveStorageGenerics(Environment *,FILE *);

#endif

   static void                    BloadStorageGenerics(Environment *);
   static void                    BloadGenerics(Environment *);
   static void                    UpdateGenericModule(Environment *,void *,unsigned long);
   static void                    UpdateGeneric(Environment *,void *,unsigned long);
   static void                    UpdateMethod(Environment *,void *,unsigned long);
   static void                    UpdateRestriction(Environment *,void *,unsigned long);
   static void                    UpdateType(Environment *,void *,unsigned long);
   static void                    ClearBloadGenerics(Environment *);
   static void                    DeallocateDefgenericBinaryData(Environment *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************
  NAME         : SetupGenericsBload
  DESCRIPTION  : Initializes data structures and
                   routines for binary loads of
                   generic function constructs
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Routines defined and structures initialized
  NOTES        : None
 ***********************************************************/
void SetupGenericsBload(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,GENRCBIN_DATA,sizeof(struct defgenericBinaryData),DeallocateDefgenericBinaryData);
#if BLOAD_AND_BSAVE
   AddBinaryItem(theEnv,"generic functions",0,BsaveGenericsFind,BsaveGenericsExpressions,
                             BsaveStorageGenerics,BsaveGenerics,
                             BloadStorageGenerics,BloadGenerics,
                             ClearBloadGenerics);
#endif
#if BLOAD || BLOAD_ONLY
   AddBinaryItem(theEnv,"generic functions",0,NULL,NULL,NULL,NULL,
                             BloadStorageGenerics,BloadGenerics,
                             ClearBloadGenerics);
#endif
  }

/***********************************************************/
/* DeallocateDefgenericBinaryData: Deallocates environment */
/*    data for the defgeneric binary functionality.        */
/***********************************************************/
static void DeallocateDefgenericBinaryData(
  Environment *theEnv)
  {
#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE) && (! RUN_TIME)
   size_t space;

   space = DefgenericBinaryData(theEnv)->GenericCount * sizeof(Defgeneric);
   if (space != 0) genfree(theEnv,DefgenericBinaryData(theEnv)->DefgenericArray,space);

   space = DefgenericBinaryData(theEnv)->MethodCount * sizeof(Defmethod);
   if (space != 0) genfree(theEnv,DefgenericBinaryData(theEnv)->MethodArray,space);

   space = DefgenericBinaryData(theEnv)->RestrictionCount * sizeof(struct restriction);
   if (space != 0) genfree(theEnv,DefgenericBinaryData(theEnv)->RestrictionArray,space);

   space = DefgenericBinaryData(theEnv)->TypeCount * sizeof(void *);
   if (space != 0) genfree(theEnv,DefgenericBinaryData(theEnv)->TypeArray,space);

   space =  DefgenericBinaryData(theEnv)->ModuleCount * sizeof(struct defgenericModule);
   if (space != 0) genfree(theEnv,DefgenericBinaryData(theEnv)->ModuleArray,space);
#endif
  }

/***************************************************
  NAME         : BloadDefgenericModuleReference
  DESCRIPTION  : Returns a pointer to the
                 appropriate defgeneric module
  INPUTS       : The index of the module
  RETURNS      : A pointer to the module
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
void *BloadDefgenericModuleReference(
  Environment *theEnv,
  unsigned long theIndex)
  {
   return ((void *) &DefgenericBinaryData(theEnv)->ModuleArray[theIndex]);
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

#if BLOAD_AND_BSAVE

/***************************************************************************
  NAME         : BsaveGenericsFind
  DESCRIPTION  : For all generic functions and their
                   methods, this routine marks all
                   the needed symbols and system functions.
                 Also, it also counts the number of
                   expression structures needed.
                 Also, counts total number of generics, methods,
                   restrictions and types.
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : ExpressionCount (a global from BSAVE.C) is incremented
                   for every expression needed
                 Symbols and system function are marked in their structures
  NOTES        : Also sets bsaveIndex for each generic function (assumes
                   generic functions will be bsaved in order of binary list)
 ***************************************************************************/
static void BsaveGenericsFind(
  Environment *theEnv)
  {
   SaveBloadCount(theEnv,DefgenericBinaryData(theEnv)->ModuleCount);
   SaveBloadCount(theEnv,DefgenericBinaryData(theEnv)->GenericCount);
   SaveBloadCount(theEnv,DefgenericBinaryData(theEnv)->MethodCount);
   SaveBloadCount(theEnv,DefgenericBinaryData(theEnv)->RestrictionCount);
   SaveBloadCount(theEnv,DefgenericBinaryData(theEnv)->TypeCount);

   DefgenericBinaryData(theEnv)->GenericCount = 0L;
   DefgenericBinaryData(theEnv)->MethodCount = 0L;
   DefgenericBinaryData(theEnv)->RestrictionCount = 0L;
   DefgenericBinaryData(theEnv)->TypeCount = 0L;

   DefgenericBinaryData(theEnv)->ModuleCount = GetNumberOfDefmodules(theEnv);
   
   DoForAllConstructs(theEnv,MarkDefgenericItems,
                      DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,NULL);
  }

/***************************************************
  NAME         : MarkDefgenericItems
  DESCRIPTION  : Marks the needed items for
                 a defgeneric (and methods) bsave
  INPUTS       : 1) The defgeneric
                 2) User data buffer (ignored)
  RETURNS      : Nothing useful
  SIDE EFFECTS : Needed items marked
  NOTES        : None
 ***************************************************/
static void MarkDefgenericItems(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
#if MAC_XCD
#pragma unused(userBuffer)
#endif
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   long i,j;
   Defmethod *meth;
   RESTRICTION *rptr;

   MarkConstructHeaderNeededItems(&gfunc->header,DefgenericBinaryData(theEnv)->GenericCount++);
   DefgenericBinaryData(theEnv)->MethodCount += gfunc->mcnt;
   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      meth = &gfunc->methods[i];
      ExpressionData(theEnv)->ExpressionCount += ExpressionSize(meth->actions);
      MarkNeededItems(theEnv,meth->actions);
      DefgenericBinaryData(theEnv)->RestrictionCount += meth->restrictionCount;
      for (j = 0 ; j < meth->restrictionCount ; j++)
        {
         rptr = &meth->restrictions[j];
         ExpressionData(theEnv)->ExpressionCount += ExpressionSize(rptr->query);
         MarkNeededItems(theEnv,rptr->query);
         DefgenericBinaryData(theEnv)->TypeCount += rptr->tcnt;
        }
     }
  }

/***************************************************
  NAME         : BsaveGenericsExpressions
  DESCRIPTION  : Writes out all expressions needed
                   by generic functions
  INPUTS       : The file pointer of the binary file
  RETURNS      : Nothing useful
  SIDE EFFECTS : File updated
  NOTES        : None
 ***************************************************/
static void BsaveGenericsExpressions(
  Environment *theEnv,
  FILE *fp)
  {
   /*===================================================================*/
   /* Important to save all expressions for methods before any          */
   /* expressions for restrictions, since methods will be stored first. */
   /*===================================================================*/

   DoForAllConstructs(theEnv,BsaveMethodExpressions,DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,fp);

   DoForAllConstructs(theEnv,BsaveRestrictionExpressions,DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,fp);
  }

/***************************************************
  NAME         : BsaveMethodExpressions
  DESCRIPTION  : Saves the needed expressions for
                 a defgeneric methods bsave
  INPUTS       : 1) The defgeneric
                 2) Output data file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Method action expressions saved
  NOTES        : None
 ***************************************************/
static void BsaveMethodExpressions(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   long i;

   for (i = 0 ; i < gfunc->mcnt ; i++)
     BsaveExpression(theEnv,gfunc->methods[i].actions,(FILE *) userBuffer);
  }

/***************************************************
  NAME         : BsaveRestrictionExpressions
  DESCRIPTION  : Saves the needed expressions for
                 a defgeneric method restriction
                 queries bsave
  INPUTS       : 1) The defgeneric
                 2) Output data file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Method restriction query
                 expressions saved
  NOTES        : None
 ***************************************************/
static void BsaveRestrictionExpressions(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   long i,j;
   Defmethod *meth;

   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      meth = &gfunc->methods[i];
      for (j = 0 ; j < meth->restrictionCount ; j++)
        BsaveExpression(theEnv,meth->restrictions[j].query,(FILE *) userBuffer);
     }
  }

/***********************************************************
  NAME         : BsaveStorageGenerics
  DESCRIPTION  : Writes out number of each type of structure
                   required for generics
                 Space required for counts (unsigned long)
  INPUTS       : File pointer of binary file
  RETURNS      : Nothing useful
  SIDE EFFECTS : Binary file adjusted
  NOTES        : None
 ***********************************************************/
static void BsaveStorageGenerics(
  Environment *theEnv,
  FILE *fp)
  {
   size_t space;

   space = sizeof(long) * 5;
   GenWrite(&space,sizeof(size_t),fp);
   GenWrite(&DefgenericBinaryData(theEnv)->ModuleCount,sizeof(long),fp);
   GenWrite(&DefgenericBinaryData(theEnv)->GenericCount,sizeof(long),fp);
   GenWrite(&DefgenericBinaryData(theEnv)->MethodCount,sizeof(long),fp);
   GenWrite(&DefgenericBinaryData(theEnv)->RestrictionCount,sizeof(long),fp);
   GenWrite(&DefgenericBinaryData(theEnv)->TypeCount,sizeof(long),fp);
  }

/****************************************************************************************
  NAME         : BsaveGenerics
  DESCRIPTION  : Writes out generic function in binary format
                 Space required (unsigned long)
                 All generic modules (sizeof(DEFGENERIC_MODULE) * Number of generic modules)
                 All generic headers (sizeof(Defgeneric) * Number of generics)
                 All methods (sizeof(Defmethod) * Number of methods)
                 All method restrictions (sizeof(RESTRICTION) * Number of restriction
                 All restriction type arrays (sizeof(void *) * # of types)
  INPUTS       : File pointer of binary file
  RETURNS      : Nothing useful
  SIDE EFFECTS : Binary file adjusted
  NOTES        : None
 ****************************************************************************************/
static void BsaveGenerics(
  Environment *theEnv,
  FILE *fp)
  {
   Defmodule *theModule;
   DEFGENERIC_MODULE *theModuleItem;
   size_t space;
   BSAVE_DEFGENERIC_MODULE dummy_generic_module;

   /* =====================================================================
      Space is: Sum over all structures(sizeof(structure) * structure-cnt))
      ===================================================================== */
   space = (DefgenericBinaryData(theEnv)->ModuleCount * sizeof(BSAVE_DEFGENERIC_MODULE)) +
           (DefgenericBinaryData(theEnv)->GenericCount * sizeof(BSAVE_GENERIC)) +
           (DefgenericBinaryData(theEnv)->MethodCount * sizeof(BSAVE_METHOD)) +
           (DefgenericBinaryData(theEnv)->RestrictionCount * sizeof(BSAVE_RESTRICTION)) +
           (DefgenericBinaryData(theEnv)->TypeCount * sizeof(unsigned long));

   /* ================================================================
      Write out the total amount of space required:  modules,headers,
      methods, restrictions, types
      ================================================================ */
   GenWrite(&space,sizeof(size_t),fp);

   /* ======================================
      Write out the generic function modules
      ====================================== */
   DefgenericBinaryData(theEnv)->GenericCount = 0L;
   theModule = GetNextDefmodule(theEnv,NULL);
   while (theModule != NULL)
     {
      theModuleItem = (DEFGENERIC_MODULE *)
                      GetModuleItem(theEnv,theModule,FindModuleItem(theEnv,"defgeneric")->moduleIndex);
      AssignBsaveDefmdlItemHdrVals(&dummy_generic_module.header,
                                           &theModuleItem->header);
      GenWrite(&dummy_generic_module,
               sizeof(BSAVE_DEFGENERIC_MODULE),fp);
      theModule = GetNextDefmodule(theEnv,theModule);
     }


   /* ======================================
      Write out the generic function headers
      ====================================== */
   DefgenericBinaryData(theEnv)->MethodCount = 0L;
   DoForAllConstructs(theEnv,BsaveDefgenericHeader,DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,fp);

   /* =====================
      Write out the methods
      ===================== */
   DefgenericBinaryData(theEnv)->RestrictionCount = 0L;
   DoForAllConstructs(theEnv,BsaveMethods,DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,fp);

   /* =================================
      Write out the method restrictions
      ================================= */
   DefgenericBinaryData(theEnv)->TypeCount = 0L;
   DoForAllConstructs(theEnv,BsaveMethodRestrictions,DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,fp);

   /* =============================================================
      Finally, write out the type lists for the method restrictions
      ============================================================= */
   DoForAllConstructs(theEnv,BsaveRestrictionTypes,DefgenericData(theEnv)->DefgenericModuleIndex,
                      false,fp);

   RestoreBloadCount(theEnv,&DefgenericBinaryData(theEnv)->ModuleCount);
   RestoreBloadCount(theEnv,&DefgenericBinaryData(theEnv)->GenericCount);
   RestoreBloadCount(theEnv,&DefgenericBinaryData(theEnv)->MethodCount);
   RestoreBloadCount(theEnv,&DefgenericBinaryData(theEnv)->RestrictionCount);
   RestoreBloadCount(theEnv,&DefgenericBinaryData(theEnv)->TypeCount);
  }

/***************************************************
  NAME         : BsaveDefgenericHeader
  DESCRIPTION  : Bsaves a generic function header
  INPUTS       : 1) The defgeneric
                 2) Output data file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defgeneric header saved
  NOTES        : None
 ***************************************************/
static void BsaveDefgenericHeader(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   BSAVE_GENERIC dummy_generic;

   AssignBsaveConstructHeaderVals(&dummy_generic.header,&gfunc->header);
   dummy_generic.mcnt = gfunc->mcnt;
   if (gfunc->methods != NULL)
     {
      dummy_generic.methods = DefgenericBinaryData(theEnv)->MethodCount;
      DefgenericBinaryData(theEnv)->MethodCount += gfunc->mcnt;
     }
   else
     dummy_generic.methods = ULONG_MAX;
   GenWrite(&dummy_generic,sizeof(BSAVE_GENERIC),(FILE *) userBuffer);
  }

/***************************************************
  NAME         : BsaveMethods
  DESCRIPTION  : Bsaves defgeneric methods
  INPUTS       : 1) The defgeneric
                 2) Output data file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defgeneric methods saved
  NOTES        : None
 ***************************************************/
static void BsaveMethods(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   Defmethod *meth;
   BSAVE_METHOD dummy_method;
   long i;

   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      meth = &gfunc->methods[i];
      
      AssignBsaveConstructHeaderVals(&dummy_method.header,&meth->header);

      dummy_method.index = meth->index;
      dummy_method.restrictionCount = meth->restrictionCount;
      dummy_method.minRestrictions = meth->minRestrictions;
      dummy_method.maxRestrictions = meth->maxRestrictions;
      dummy_method.localVarCount = meth->localVarCount;
      dummy_method.system = meth->system;
      if (meth->restrictions != NULL)
        {
         dummy_method.restrictions = DefgenericBinaryData(theEnv)->RestrictionCount;
         DefgenericBinaryData(theEnv)->RestrictionCount += meth->restrictionCount;
        }
      else
        dummy_method.restrictions = ULONG_MAX;
      if (meth->actions != NULL)
        {
         dummy_method.actions = ExpressionData(theEnv)->ExpressionCount;
         ExpressionData(theEnv)->ExpressionCount += ExpressionSize(meth->actions);
        }
      else
        dummy_method.actions = ULONG_MAX;
      GenWrite(&dummy_method,sizeof(BSAVE_METHOD),(FILE *) userBuffer);
     }
  }

/******************************************************
  NAME         : BsaveMethodRestrictions
  DESCRIPTION  : Bsaves defgeneric methods' retrictions
  INPUTS       : 1) The defgeneric
                 2) Output data file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defgeneric methods' restrictions saved
  NOTES        : None
 ******************************************************/
static void BsaveMethodRestrictions(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   BSAVE_RESTRICTION dummy_restriction;
   RESTRICTION *rptr;
   short i,j;

   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      for (j = 0 ; j < gfunc->methods[i].restrictionCount ; j++)
        {
         rptr = &gfunc->methods[i].restrictions[j];
         dummy_restriction.tcnt = rptr->tcnt;
         if (rptr->types != NULL)
           {
            dummy_restriction.types = DefgenericBinaryData(theEnv)->TypeCount;
            DefgenericBinaryData(theEnv)->TypeCount += rptr->tcnt;
           }
         else
           dummy_restriction.types = ULONG_MAX;
         if (rptr->query != NULL)
           {
            dummy_restriction.query = ExpressionData(theEnv)->ExpressionCount;
            ExpressionData(theEnv)->ExpressionCount += ExpressionSize(rptr->query);
           }
         else
           dummy_restriction.query = ULONG_MAX;
         GenWrite(&dummy_restriction,
                  sizeof(BSAVE_RESTRICTION),(FILE *) userBuffer);
        }
     }
  }

/*************************************************************
  NAME         : BsaveRestrictionTypes
  DESCRIPTION  : Bsaves defgeneric methods' retrictions' types
  INPUTS       : 1) The defgeneric
                 2) Output data file pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defgeneric methods' restrictions' types saved
  NOTES        : None
 *************************************************************/
static void BsaveRestrictionTypes(
  Environment *theEnv,
  ConstructHeader *theDefgeneric,
  void *userBuffer)
  {
   Defgeneric *gfunc = (Defgeneric *) theDefgeneric;
   unsigned long dummy_type;
   RESTRICTION *rptr;
   short i,j,k;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      for (j = 0 ; j < gfunc->methods[i].restrictionCount ; j++)
        {
         rptr = &gfunc->methods[i].restrictions[j];
         for (k = 0 ; k < rptr->tcnt ; k++)
           {
#if OBJECT_SYSTEM
            dummy_type = DefclassIndex(rptr->types[k]);
#else
            dummy_type = (unsigned long) ((CLIPSInteger *) rptr->types[k])->contents;
#endif
            GenWrite(&dummy_type,sizeof(unsigned long),(FILE *) userBuffer);
           }
        }
     }
  }

#endif

/***********************************************************************
  NAME         : BloadStorageGenerics
  DESCRIPTION  : This routine space required for generic function
                   structures and allocates space for them
  INPUTS       : Nothing
  RETURNS      : Nothing useful
  SIDE EFFECTS : Arrays allocated and set
  NOTES        : This routine makes no attempt to reset any pointers
                   within the structures
 ***********************************************************************/
static void BloadStorageGenerics(
  Environment *theEnv)
  {
   size_t space;
   unsigned long counts[5];

   GenReadBinary(theEnv,&space,sizeof(size_t));
   if (space == 0L)
     return;
   GenReadBinary(theEnv,counts,space);
   DefgenericBinaryData(theEnv)->ModuleCount = counts[0];
   DefgenericBinaryData(theEnv)->GenericCount = counts[1];
   DefgenericBinaryData(theEnv)->MethodCount = counts[2];
   DefgenericBinaryData(theEnv)->RestrictionCount = counts[3];
   DefgenericBinaryData(theEnv)->TypeCount = counts[4];
   if (DefgenericBinaryData(theEnv)->ModuleCount != 0L)
     {
      space = (sizeof(DEFGENERIC_MODULE) * DefgenericBinaryData(theEnv)->ModuleCount);
      DefgenericBinaryData(theEnv)->ModuleArray = (DEFGENERIC_MODULE *) genalloc(theEnv,space);
     }
   else
     return;
   if (DefgenericBinaryData(theEnv)->GenericCount != 0L)
     {
      space = (sizeof(Defgeneric) * DefgenericBinaryData(theEnv)->GenericCount);
      DefgenericBinaryData(theEnv)->DefgenericArray = (Defgeneric *) genalloc(theEnv,space);
     }
   else
     return;
   if (DefgenericBinaryData(theEnv)->MethodCount != 0L)
     {
      space = (sizeof(Defmethod) * DefgenericBinaryData(theEnv)->MethodCount);
      DefgenericBinaryData(theEnv)->MethodArray = (Defmethod *) genalloc(theEnv,space);
     }
   else
     return;
   if (DefgenericBinaryData(theEnv)->RestrictionCount != 0L)
     {
      space = (sizeof(RESTRICTION) * DefgenericBinaryData(theEnv)->RestrictionCount);
      DefgenericBinaryData(theEnv)->RestrictionArray = (RESTRICTION *) genalloc(theEnv,space);
     }
   else
     return;
   if (DefgenericBinaryData(theEnv)->TypeCount != 0L)
     {
      space = (sizeof(void *) * DefgenericBinaryData(theEnv)->TypeCount);
      DefgenericBinaryData(theEnv)->TypeArray = (void * *) genalloc(theEnv,space);
     }
  }

/*********************************************************************
  NAME         : BloadGenerics
  DESCRIPTION  : This routine reads generic function information from
                 a binary file in four chunks:
                 Generic-header array
                 Method array
                 Method restrictions array
                 Restriction types array

                 This routine moves through the generic function
                   binary arrays updating pointers
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Pointers reset from array indices
  NOTES        : Assumes all loading is finished
 ********************************************************************/
static void BloadGenerics(
  Environment *theEnv)
  {
   size_t space;

   GenReadBinary(theEnv,&space,sizeof(size_t));
   if (DefgenericBinaryData(theEnv)->ModuleCount == 0L)
     return;
   BloadandRefresh(theEnv,DefgenericBinaryData(theEnv)->ModuleCount,sizeof(BSAVE_DEFGENERIC_MODULE),UpdateGenericModule);
   if (DefgenericBinaryData(theEnv)->GenericCount == 0L)
     return;
   BloadandRefresh(theEnv,DefgenericBinaryData(theEnv)->GenericCount,sizeof(BSAVE_GENERIC),UpdateGeneric);
   BloadandRefresh(theEnv,DefgenericBinaryData(theEnv)->MethodCount,sizeof(BSAVE_METHOD),UpdateMethod);
   BloadandRefresh(theEnv,DefgenericBinaryData(theEnv)->RestrictionCount,sizeof(BSAVE_RESTRICTION),UpdateRestriction);
   BloadandRefresh(theEnv,DefgenericBinaryData(theEnv)->TypeCount,sizeof(long),UpdateType);
  }

/*********************************************
  Bload update routines for generic structures
 *********************************************/
static void UpdateGenericModule(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   BSAVE_DEFGENERIC_MODULE *bdptr;

   bdptr = (BSAVE_DEFGENERIC_MODULE *) buf;
   UpdateDefmoduleItemHeader(theEnv,&bdptr->header,&DefgenericBinaryData(theEnv)->ModuleArray[obji].header,
                             sizeof(Defgeneric),DefgenericBinaryData(theEnv)->DefgenericArray);
  }

static void UpdateGeneric(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   BSAVE_GENERIC *bgp;
   Defgeneric *gp;

   bgp = (BSAVE_GENERIC *) buf;
   gp = &DefgenericBinaryData(theEnv)->DefgenericArray[obji];

   UpdateConstructHeader(theEnv,&bgp->header,&gp->header,DEFGENERIC,
                         sizeof(DEFGENERIC_MODULE),DefgenericBinaryData(theEnv)->ModuleArray,
                         sizeof(Defgeneric),DefgenericBinaryData(theEnv)->DefgenericArray);
   DefgenericBinaryData(theEnv)->DefgenericArray[obji].busy = 0;
#if DEBUGGING_FUNCTIONS
   DefgenericBinaryData(theEnv)->DefgenericArray[obji].trace = DefgenericData(theEnv)->WatchGenerics;
#endif
   DefgenericBinaryData(theEnv)->DefgenericArray[obji].methods = MethodPointer(bgp->methods);
   DefgenericBinaryData(theEnv)->DefgenericArray[obji].mcnt = bgp->mcnt;
   DefgenericBinaryData(theEnv)->DefgenericArray[obji].new_index = 0;
  }

static void UpdateMethod(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   BSAVE_METHOD *bmth;

   bmth = (BSAVE_METHOD *) buf;
   DefgenericBinaryData(theEnv)->MethodArray[obji].index = bmth->index;
   DefgenericBinaryData(theEnv)->MethodArray[obji].busy = 0;
#if DEBUGGING_FUNCTIONS
   DefgenericBinaryData(theEnv)->MethodArray[obji].trace = DefgenericData(theEnv)->WatchMethods;
#endif
   DefgenericBinaryData(theEnv)->MethodArray[obji].restrictionCount = bmth->restrictionCount;
   DefgenericBinaryData(theEnv)->MethodArray[obji].minRestrictions = bmth->minRestrictions;
   DefgenericBinaryData(theEnv)->MethodArray[obji].maxRestrictions = bmth->maxRestrictions;
   DefgenericBinaryData(theEnv)->MethodArray[obji].localVarCount = bmth->localVarCount;
   DefgenericBinaryData(theEnv)->MethodArray[obji].system = bmth->system;
   DefgenericBinaryData(theEnv)->MethodArray[obji].restrictions = RestrictionPointer(bmth->restrictions);
   DefgenericBinaryData(theEnv)->MethodArray[obji].actions = ExpressionPointer(bmth->actions);
   
   UpdateConstructHeader(theEnv,&bmth->header,&DefgenericBinaryData(theEnv)->MethodArray[obji].header,DEFMETHOD,
                         sizeof(DEFGENERIC_MODULE),DefgenericBinaryData(theEnv)->ModuleArray,
                         sizeof(Defmethod),DefgenericBinaryData(theEnv)->MethodArray);
  }

static void UpdateRestriction(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
   BSAVE_RESTRICTION *brp;

   brp = (BSAVE_RESTRICTION *) buf;
   DefgenericBinaryData(theEnv)->RestrictionArray[obji].tcnt = brp->tcnt;
   DefgenericBinaryData(theEnv)->RestrictionArray[obji].types = TypePointer(brp->types);
   DefgenericBinaryData(theEnv)->RestrictionArray[obji].query = ExpressionPointer(brp->query);
  }

static void UpdateType(
  Environment *theEnv,
  void *buf,
  unsigned long obji)
  {
#if OBJECT_SYSTEM
   DefgenericBinaryData(theEnv)->TypeArray[obji] = DefclassPointer(* (unsigned long *) buf);
#else
   if ((* (long *) buf) > INSTANCE_TYPE_CODE)
     {
      PrintWarningID(theEnv,"GENRCBIN",1,false);
      WriteString(theEnv,STDWRN,"COOL not installed!  User-defined class\n");
      WriteString(theEnv,STDWRN,"  in method restriction substituted with OBJECT.\n");
      DefgenericBinaryData(theEnv)->TypeArray[obji] = CreateInteger(theEnv,OBJECT_TYPE_CODE);
     }
   else
     DefgenericBinaryData(theEnv)->TypeArray[obji] = CreateInteger(theEnv,* (long *) buf);
   IncrementIntegerCount((CLIPSInteger *) DefgenericBinaryData(theEnv)->TypeArray[obji]);
#endif
  }

/***************************************************************
  NAME         : ClearBloadGenerics
  DESCRIPTION  : Release all binary-loaded generic function
                   structure arrays
                 Resets generic function list to NULL
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Memory cleared
  NOTES        : Generic function name symbol counts decremented
 ***************************************************************/
static void ClearBloadGenerics(
  Environment *theEnv)
  {
   unsigned long i;
   size_t space;

   space = (sizeof(DEFGENERIC_MODULE) * DefgenericBinaryData(theEnv)->ModuleCount);
   if (space == 0L)
     return;
   genfree(theEnv,DefgenericBinaryData(theEnv)->ModuleArray,space);
   DefgenericBinaryData(theEnv)->ModuleArray = NULL;
   DefgenericBinaryData(theEnv)->ModuleCount = 0L;

   for (i = 0 ; i < DefgenericBinaryData(theEnv)->GenericCount ; i++)
     UnmarkConstructHeader(theEnv,&DefgenericBinaryData(theEnv)->DefgenericArray[i].header);

   space = (sizeof(Defgeneric) * DefgenericBinaryData(theEnv)->GenericCount);
   if (space == 0L)
     return;
   genfree(theEnv,DefgenericBinaryData(theEnv)->DefgenericArray,space);
   DefgenericBinaryData(theEnv)->DefgenericArray = NULL;
   DefgenericBinaryData(theEnv)->GenericCount = 0L;

   space = (sizeof(Defmethod) * DefgenericBinaryData(theEnv)->MethodCount);
   if (space == 0L)
     return;
   genfree(theEnv,DefgenericBinaryData(theEnv)->MethodArray,space);
   DefgenericBinaryData(theEnv)->MethodArray = NULL;
   DefgenericBinaryData(theEnv)->MethodCount = 0L;

   space = (sizeof(RESTRICTION) * DefgenericBinaryData(theEnv)->RestrictionCount);
   if (space == 0L)
     return;
   genfree(theEnv,DefgenericBinaryData(theEnv)->RestrictionArray,space);
   DefgenericBinaryData(theEnv)->RestrictionArray = NULL;
   DefgenericBinaryData(theEnv)->RestrictionCount = 0L;

#if ! OBJECT_SYSTEM
   for (i = 0 ; i < DefgenericBinaryData(theEnv)->TypeCount ; i++)
     DecrementIntegerCount(theEnv,(CLIPSInteger *) DefgenericBinaryData(theEnv)->TypeArray[i]);
#endif
   space = (sizeof(void *) * DefgenericBinaryData(theEnv)->TypeCount);
   if (space == 0L)
     return;
   genfree(theEnv,DefgenericBinaryData(theEnv)->TypeArray,space);
   DefgenericBinaryData(theEnv)->TypeArray = NULL;
   DefgenericBinaryData(theEnv)->TypeCount = 0L;
  }

#endif

