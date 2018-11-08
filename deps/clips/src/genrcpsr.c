   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  11/01/16             */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose: Generic Functions Parsing Routines               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            If the last construct in a loaded file is a    */
/*            deffunction or defmethod with no closing right */
/*            parenthesis, an error should be issued, but is */
/*            not. DR0872                                    */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            GetConstructNameAndComment API change.         */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Used gensprintf instead of sprintf.            */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Fixed linkage issue when BLOAD_AND_SAVE        */
/*            compiler flag is set to 0.                     */
/*                                                           */
/*            Fixed typing issue when OBJECT_SYSTEM          */
/*            compiler flag is set to 0.                     */
/*                                                           */
/*            Changed find construct functionality so that   */
/*            imported modules are search when locating a    */
/*            named construct.                               */
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

#if DEFGENERIC_CONSTRUCT && (! BLOAD_ONLY) && (! RUN_TIME)

#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif

#if DEFFUNCTION_CONSTRUCT
#include "dffnxfun.h"
#endif

#if OBJECT_SYSTEM
#include "classfun.h"
#include "classcom.h"
#endif

#include "cstrccom.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "genrccom.h"
#include "immthpsr.h"
#include "memalloc.h"
#include "modulutl.h"
#include "pprint.h"
#include "prcdrpsr.h"
#include "prccode.h"
#include "prntutil.h"
#include "router.h"
#include "scanner.h"
#include "sysdep.h"

#include "genrcpsr.h"

/* =========================================
   *****************************************
                   CONSTANTS
   =========================================
   ***************************************** */
#define HIGHER_PRECEDENCE -1
#define IDENTICAL          0
#define LOWER_PRECEDENCE   1

#define CURR_ARG_VAR "current-argument"

#define PARAMETER_ERROR USHRT_MAX

/* =========================================
   *****************************************
      INTERNALLY VISIBLE FUNCTION HEADERS
   =========================================
   ***************************************** */

   static bool                    ValidGenericName(Environment *,const char *);
   static CLIPSLexeme            *ParseMethodNameAndIndex(Environment *,const char *,unsigned short *,struct token *);

#if DEBUGGING_FUNCTIONS
   static void                    CreateDefaultGenericPPForm(Environment *,Defgeneric *);
#endif

   static unsigned short          ParseMethodParameters(Environment *,const char *,Expression **,CLIPSLexeme **,struct token *);
   static RESTRICTION            *ParseRestriction(Environment *,const char *);
   static void                    ReplaceCurrentArgRefs(Environment *,Expression *);
   static bool                    DuplicateParameters(Environment *,Expression *,Expression **,CLIPSLexeme *);
   static Expression             *AddParameter(Environment *,Expression *,Expression *,CLIPSLexeme *,RESTRICTION *);
   static Expression             *ValidType(Environment *,CLIPSLexeme *);
   static bool                    RedundantClasses(Environment *,void *,void *);
   static Defgeneric             *AddGeneric(Environment *,CLIPSLexeme *,bool *);
   static Defmethod              *AddGenericMethod(Environment *,Defgeneric *,int,unsigned short);
   static int                     RestrictionsCompare(Expression *,int,int,int,Defmethod *);
   static int                     TypeListCompare(RESTRICTION *,RESTRICTION *);
   static Defgeneric             *NewGeneric(Environment *,CLIPSLexeme *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************************************
  NAME         : ParseDefgeneric
  DESCRIPTION  : Parses the defgeneric construct
  INPUTS       : The input logical name
  RETURNS      : False if successful parse, true otherwise
  SIDE EFFECTS : Inserts valid generic function defn into generic entry
  NOTES        : H/L Syntax :
                 (defgeneric <name> [<comment>])
 ***************************************************************************/
bool ParseDefgeneric(
  Environment *theEnv,
  const char *readSource)
  {
   CLIPSLexeme *gname;
   Defgeneric *gfunc;
   bool newGeneric;
   struct token genericInputToken;

   SetPPBufferStatus(theEnv,true);
   FlushPPBuffer(theEnv);
   SavePPBuffer(theEnv,"(defgeneric ");
   SetIndentDepth(theEnv,3);

#if BLOAD || BLOAD_AND_BSAVE
   if ((Bloaded(theEnv) == true) && (! ConstructData(theEnv)->CheckSyntaxMode))
     {
      CannotLoadWithBloadMessage(theEnv,"defgeneric");
      return true;
     }
#endif

   gname = GetConstructNameAndComment(theEnv,readSource,&genericInputToken,"defgeneric",
                                      (FindConstructFunction *) FindDefgenericInModule,
                                      NULL,"^",true,true,true,false);
   if (gname == NULL)
     return true;

   if (ValidGenericName(theEnv,gname->contents) == false)
     return true;

   if (genericInputToken.tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      PrintErrorID(theEnv,"GENRCPSR",1,false);
      WriteString(theEnv,STDERR,"Expected ')' to complete defgeneric.\n");
      return true;
     }
   SavePPBuffer(theEnv,"\n");

   /* ========================================================
      If we're only checking syntax, don't add the
      successfully parsed deffacts to the KB.
      ======================================================== */

   if (ConstructData(theEnv)->CheckSyntaxMode)
     { return false; }

   gfunc = AddGeneric(theEnv,gname,&newGeneric);

#if DEBUGGING_FUNCTIONS
   SetDefgenericPPForm(theEnv,gfunc,GetConserveMemory(theEnv) ? NULL : CopyPPBuffer(theEnv));
#endif
   return false;
  }

/***************************************************************************
  NAME         : ParseDefmethod
  DESCRIPTION  : Parses the defmethod construct
  INPUTS       : The input logical name
  RETURNS      : False if successful parse, true otherwise
  SIDE EFFECTS : Inserts valid method definition into generic entry
  NOTES        : H/L Syntax :
                 (defmethod <name> [<index>] [<comment>]
                    (<restriction>* [<wildcard>])
                    <action>*)
                 <restriction> :== ?<name> |
                                   (?<name> <type>* [<restriction-query>])
                 <wildcard>    :== $?<name> |
                                   ($?<name> <type>* [<restriction-query>])
 ***************************************************************************/
bool ParseDefmethod(
  Environment *theEnv,
  const char *readSource)
  {
   CLIPSLexeme *gname;
   unsigned short rcnt;
   int mposn;
   unsigned short mi;
   unsigned short lvars;
   bool newMethod;
   bool mnew = false;
   bool error;
   Expression *params,*actions,*tmp;
   CLIPSLexeme *wildcard;
   Defmethod *meth;
   Defgeneric *gfunc;
   unsigned short theIndex;
   struct token genericInputToken;

   SetPPBufferStatus(theEnv,true);
   FlushPPBuffer(theEnv);
   SetIndentDepth(theEnv,3);
   SavePPBuffer(theEnv,"(defmethod ");

#if BLOAD || BLOAD_AND_BSAVE
   if ((Bloaded(theEnv) == true) && (! ConstructData(theEnv)->CheckSyntaxMode))
     {
      CannotLoadWithBloadMessage(theEnv,"defmethod");
      return true;
     }
#endif

   gname = ParseMethodNameAndIndex(theEnv,readSource,&theIndex,&genericInputToken);
   if (gname == NULL)
     return true;

   if (ValidGenericName(theEnv,gname->contents) == false)
     return true;

   /* ========================================================
      Go ahead and add the header so that the generic function
      can be called recursively
      ======================================================== */
   gfunc = AddGeneric(theEnv,gname,&newMethod);

#if DEBUGGING_FUNCTIONS
   if (newMethod && (! ConstructData(theEnv)->CheckSyntaxMode))
      CreateDefaultGenericPPForm(theEnv,gfunc);
#endif

   IncrementIndentDepth(theEnv,1);
   rcnt = ParseMethodParameters(theEnv,readSource,&params,&wildcard,&genericInputToken);
   DecrementIndentDepth(theEnv,1);
   if (rcnt == PARAMETER_ERROR)
     goto DefmethodParseError;
   PPCRAndIndent(theEnv);
   for (tmp = params ; tmp != NULL ; tmp = tmp->nextArg)
     {
      ReplaceCurrentArgRefs(theEnv,((RESTRICTION *) tmp->argList)->query);
      if (ReplaceProcVars(theEnv,"method",((RESTRICTION *) tmp->argList)->query,
                                  params,wildcard,NULL,NULL))
        {
         DeleteTempRestricts(theEnv,params);
         goto DefmethodParseError;
        }
     }
   meth = FindMethodByRestrictions(gfunc,params,rcnt,wildcard,&mposn);
   error = false;
   if (meth != NULL)
     {
      if (meth->system)
        {
         PrintErrorID(theEnv,"GENRCPSR",17,false);
         WriteString(theEnv,STDERR,"Cannot replace the implicit system method #");
         PrintUnsignedInteger(theEnv,STDERR,meth->index);
         WriteString(theEnv,STDERR,".\n");
         error = true;
        }
      else if ((theIndex != 0) && (theIndex != meth->index))
        {
         PrintErrorID(theEnv,"GENRCPSR",2,false);
         WriteString(theEnv,STDERR,"New method #");
         PrintUnsignedInteger(theEnv,STDERR,theIndex);
         WriteString(theEnv,STDERR," would be indistinguishable from method #");
         PrintUnsignedInteger(theEnv,STDERR,meth->index);
         WriteString(theEnv,STDERR,".\n");
         error = true;
        }
     }
   else if (theIndex != 0)
     {
      mi = FindMethodByIndex(gfunc,theIndex);
      if (mi == METHOD_NOT_FOUND)
        mnew = true;
      else if (gfunc->methods[mi].system)
        {
         PrintErrorID(theEnv,"GENRCPSR",17,false);
         WriteString(theEnv,STDERR,"Cannot replace the implicit system method #");
         PrintUnsignedInteger(theEnv,STDERR,theIndex);
         WriteString(theEnv,STDERR,".\n");
         error = true;
        }
     }
   else
     mnew = true;
   if (error)
     {
      DeleteTempRestricts(theEnv,params);
      goto DefmethodParseError;
     }
   ExpressionData(theEnv)->ReturnContext = true;
   actions = ParseProcActions(theEnv,"method",readSource,
                              &genericInputToken,params,wildcard,
                              NULL,NULL,&lvars,NULL);

   /*===========================================================*/
   /* Check for the closing right parenthesis of the defmethod. */
   /*===========================================================*/

   if ((genericInputToken.tknType != RIGHT_PARENTHESIS_TOKEN) &&  /* DR0872 */
       (actions != NULL))
     {
      SyntaxErrorMessage(theEnv,"defmethod");
      DeleteTempRestricts(theEnv,params);
      ReturnPackedExpression(theEnv,actions);
      goto DefmethodParseError;
     }

   if (actions == NULL)
     {
      DeleteTempRestricts(theEnv,params);
      goto DefmethodParseError;
     }

   /*==============================================*/
   /* If we're only checking syntax, don't add the */
   /* successfully parsed deffunction to the KB.   */
   /*==============================================*/

   if (ConstructData(theEnv)->CheckSyntaxMode)
     {
      DeleteTempRestricts(theEnv,params);
      ReturnPackedExpression(theEnv,actions);
      if (newMethod)
        {
         RemoveConstructFromModule(theEnv,&gfunc->header);
         RemoveDefgeneric(theEnv,gfunc);
        }
      return false;
     }

   PPBackup(theEnv);
   PPBackup(theEnv);
   SavePPBuffer(theEnv,genericInputToken.printForm);
   SavePPBuffer(theEnv,"\n");

#if DEBUGGING_FUNCTIONS
   meth = AddMethod(theEnv,gfunc,meth,mposn,theIndex,params,rcnt,lvars,wildcard,actions,
             GetConserveMemory(theEnv) ? NULL : CopyPPBuffer(theEnv),false);
#else
   meth = AddMethod(theEnv,gfunc,meth,mposn,theIndex,params,rcnt,lvars,wildcard,actions,NULL,false);
#endif
   DeleteTempRestricts(theEnv,params);
   if (GetPrintWhileLoading(theEnv) && GetCompilationsWatch(theEnv) &&
       (! ConstructData(theEnv)->CheckSyntaxMode))
     {
      const char *outRouter = STDOUT;

      if (mnew)
        {
         WriteString(theEnv,outRouter,"   Method #");
         PrintUnsignedInteger(theEnv,outRouter,meth->index);
         WriteString(theEnv,outRouter," defined.\n");
        }
      else
        {
         outRouter = STDWRN;
         PrintWarningID(theEnv,"CSTRCPSR",1,true);
         WriteString(theEnv,outRouter,"Method #");
         PrintUnsignedInteger(theEnv,outRouter,meth->index);
         WriteString(theEnv,outRouter," redefined.\n");
        }
     }
   return false;

DefmethodParseError:
   if (newMethod)
     {
      RemoveConstructFromModule(theEnv,&gfunc->header);
      RemoveDefgeneric(theEnv,gfunc);
     }
   return true;
  }

/************************************************************************
  NAME         : AddMethod
  DESCRIPTION  : (Re)defines a new method for a generic
                 If method already exists, deletes old information
                    before proceeding.
  INPUTS       : 1) The generic address
                 2) The old method address (can be NULL)
                 3) The old method array position (can be -1)
                 4) The method index to assign (0 if don't care)
                 5) The parameter expression-list
                    (restrictions attached to argList pointers)
                 6) The number of restrictions
                 7) The number of locals vars reqd
                 8) The wildcard symbol (NULL if none)
                 9) Method actions
                 10) Method pretty-print form
                 11) A flag indicating whether to copy the
                     restriction types or just use the pointers
  RETURNS      : The new (old) method address
  SIDE EFFECTS : Method added to (or changed in) method array for generic
                 Restrictions repacked into new method
                 Actions and pretty-print form attached
  NOTES        : Assumes if a method is being redefined, its busy
                   count is 0!!
                 IMPORTANT: Expects that FindMethodByRestrictions() has
                   previously been called to determine if this method
                   is already present or not.  Arguments #1 and #2
                   should be the values obtained from FindMethod...().
 ************************************************************************/
Defmethod *AddMethod(
  Environment *theEnv,
  Defgeneric *gfunc,
  Defmethod *meth,
  int mposn,
  unsigned short mi,
  Expression *params,
  unsigned short rcnt,
  unsigned short lvars,
  CLIPSLexeme *wildcard,
  Expression *actions,
  char *ppForm,
  bool copyRestricts)
  {
   RESTRICTION *rptr, *rtmp;
   int i,j;
   unsigned short mai;

   SaveBusyCount(gfunc);
   if (meth == NULL)
     {
      mai = (mi != 0) ? FindMethodByIndex(gfunc,mi) : METHOD_NOT_FOUND;
      if (mai == METHOD_NOT_FOUND)
        meth = AddGenericMethod(theEnv,gfunc,mposn,mi);
      else
        {
         DeleteMethodInfo(theEnv,gfunc,&gfunc->methods[mai]);
         if (mai < mposn)
           {
            mposn--;
            for (i = mai+1 ; i <= mposn ; i++)
              GenCopyMemory(Defmethod,1,&gfunc->methods[i-1],&gfunc->methods[i]);
           }
         else
           {
            for (i = mai-1 ; i >= mposn ; i--)
              GenCopyMemory(Defmethod,1,&gfunc->methods[i+1],&gfunc->methods[i]);
           }
         meth = &gfunc->methods[mposn];
         meth->index = mi;
        }
     }
   else
     {
      /* ================================
         The old trace state is preserved
         ================================ */
      ExpressionDeinstall(theEnv,meth->actions);
      ReturnPackedExpression(theEnv,meth->actions);
      if (meth->header.ppForm != NULL)
        rm(theEnv,(void *) meth->header.ppForm,(sizeof(char) * (strlen(meth->header.ppForm)+1)));
     }
   meth->system = 0;
   meth->actions = actions;
   ExpressionInstall(theEnv,meth->actions);
   meth->header.ppForm = ppForm;
   if (mposn == -1)
     {
      RestoreBusyCount(gfunc);
      return(meth);
     }

   meth->localVarCount = lvars;
   meth->restrictionCount = rcnt;
      
   if (wildcard != NULL)
     {
      if (rcnt == 0)
        { meth->minRestrictions = RESTRICTIONS_UNBOUNDED; }
      else
        { meth->minRestrictions = rcnt - 1; }
      meth->maxRestrictions = RESTRICTIONS_UNBOUNDED;
     }
   else
     meth->minRestrictions = meth->maxRestrictions = rcnt;
   if (rcnt != 0)
     meth->restrictions = (RESTRICTION *)
                          gm2(theEnv,(sizeof(RESTRICTION) * rcnt));
   else
     meth->restrictions = NULL;
   for (i = 0 ; i < rcnt ; i++)
     {
      rptr = &meth->restrictions[i];
      rtmp = (RESTRICTION *) params->argList;
      rptr->query = PackExpression(theEnv,rtmp->query);
      rptr->tcnt = rtmp->tcnt;
      if (copyRestricts)
        {
         if (rtmp->types != NULL)
           {
            rptr->types = (void **) gm2(theEnv,(rptr->tcnt * sizeof(void *)));
            GenCopyMemory(void *,rptr->tcnt,rptr->types,rtmp->types);
           }
         else
           rptr->types = NULL;
        }
      else
        {
         rptr->types = rtmp->types;

         /* =====================================================
            Make sure the types-array is not deallocated when the
              temporary restriction nodes are
            ===================================================== */
         rtmp->tcnt = 0;
         rtmp->types = NULL;
        }
      ExpressionInstall(theEnv,rptr->query);
      for (j = 0 ; j < rptr->tcnt ; j++)
#if OBJECT_SYSTEM
        IncrementDefclassBusyCount(theEnv,(Defclass *) rptr->types[j]);
#else
        IncrementIntegerCount((CLIPSInteger *) rptr->types[j]);
#endif
      params = params->nextArg;
     }
   RestoreBusyCount(gfunc);
   return(meth);
  }

/*****************************************************
  NAME         : PackRestrictionTypes
  DESCRIPTION  : Takes the restriction type list
                   and packs it into a contiguous
                   array of void *.
  INPUTS       : 1) The restriction structure
                 2) The types expression list
  RETURNS      : Nothing useful
  SIDE EFFECTS : Array allocated & expressions freed
  NOTES        : None
 *****************************************************/
void PackRestrictionTypes(
  Environment *theEnv,
  RESTRICTION *rptr,
  Expression *types)
  {
   Expression *tmp;
   long i;

   rptr->tcnt = 0;
   for (tmp = types ; tmp != NULL ; tmp = tmp->nextArg)
     rptr->tcnt++;
   if (rptr->tcnt != 0)
     rptr->types = (void **) gm2(theEnv,(sizeof(void *) * rptr->tcnt));
   else
     rptr->types = NULL;
   for (i = 0 , tmp = types ; i < rptr->tcnt ; i++ , tmp = tmp->nextArg)
     rptr->types[i] = tmp->value;
   ReturnExpression(theEnv,types);
  }

/***************************************************
  NAME         : DeleteTempRestricts
  DESCRIPTION  : Deallocates the method
                   temporary parameter list
  INPUTS       : The head of the list
  RETURNS      : Nothing useful
  SIDE EFFECTS : List deallocated
  NOTES        : None
 ***************************************************/
void DeleteTempRestricts(
  Environment *theEnv,
  Expression *phead)
  {
   Expression *ptmp;
   RESTRICTION *rtmp;

   while (phead != NULL)
     {
      ptmp = phead;
      phead = phead->nextArg;
      rtmp = (RESTRICTION *) ptmp->argList;
      rtn_struct(theEnv,expr,ptmp);
      ReturnExpression(theEnv,rtmp->query);
      if (rtmp->tcnt != 0)
        rm(theEnv,rtmp->types,(sizeof(void *) * rtmp->tcnt));
      rtn_struct(theEnv,restriction,rtmp);
     }
  }

/**********************************************************
  NAME         : FindMethodByRestrictions
  DESCRIPTION  : See if a method for the specified
                   generic satsifies the given restrictions
  INPUTS       : 1) Generic function
                 2) Parameter/restriction expression list
                 3) Number of restrictions
                 4) Wildcard symbol (can be NULL)
                 5) Caller's buffer for holding array posn
                      of where to add new generic method
                      (-1 if method already present)
  RETURNS      : The address of the found method, NULL if
                    not found
  SIDE EFFECTS : Sets the caller's buffer to the index of
                   where to place the new method, -1 if
                   already present
  NOTES        : None
 **********************************************************/
Defmethod *FindMethodByRestrictions(
  Defgeneric *gfunc,
  Expression *params,
  int rcnt,
  CLIPSLexeme *wildcard,
  int *posn)
  {
   int i,cmp;
   int min,max;

   if (wildcard != NULL)
     {
      min = rcnt-1;
      max = -1;
     }
   else
     min = max = rcnt;
   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      cmp = RestrictionsCompare(params,rcnt,min,max,&gfunc->methods[i]);
      if (cmp == IDENTICAL)
        {
         *posn = -1;
         return(&gfunc->methods[i]);
        }
      else if (cmp == HIGHER_PRECEDENCE)
        {
         *posn = i;
         return NULL;
        }
     }
   *posn = i;
   return NULL;
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************
  NAME         : ValidGenericName
  DESCRIPTION  : Determines if a particular function name
                    can be overloaded
  INPUTS       : The name
  RETURNS      : True if OK, false otherwise
  SIDE EFFECTS : Error message printed
  NOTES        : GetConstructNameAndComment() (called before
                 this function) ensures that the defgeneric
                 name does not conflict with one from
                 another module
 ***********************************************************/
static bool ValidGenericName(
  Environment *theEnv,
  const char *theDefgenericName)
  {
   Defgeneric *theDefgeneric;
#if DEFFUNCTION_CONSTRUCT
   Defmodule *theModule;
   Deffunction *theDeffunction;
#endif
   struct functionDefinition *systemFunction;

   /*==============================================*/
   /* A defgeneric cannot be named the same as a   */
   /* construct type, e.g, defclass, defrule, etc. */
   /*==============================================*/

   if (FindConstruct(theEnv,theDefgenericName) != NULL)
     {
      PrintErrorID(theEnv,"GENRCPSR",3,false);
      WriteString(theEnv,STDERR,"Defgenerics are not allowed to replace constructs.\n");
      return false;
     }

#if DEFFUNCTION_CONSTRUCT
   /* ========================================
      A defgeneric cannot be named the same as
      a defffunction (either in this module or
      imported from another)
      ======================================== */
   theDeffunction = LookupDeffunctionInScope(theEnv,theDefgenericName);
   if (theDeffunction != NULL)
     {
      theModule = GetConstructModuleItem(&theDeffunction->header)->theModule;
      if (theModule != GetCurrentModule(theEnv))
        {
         PrintErrorID(theEnv,"GENRCPSR",4,false);
         WriteString(theEnv,STDERR,"Deffunction '");
         WriteString(theEnv,STDERR,DeffunctionName(theDeffunction));
         WriteString(theEnv,STDERR,"' imported from module '");
         WriteString(theEnv,STDERR,DefmoduleName(theModule));
         WriteString(theEnv,STDERR,"' conflicts with this defgeneric.\n");
         return false;
        }
      else
        {
         PrintErrorID(theEnv,"GENRCPSR",5,false);
         WriteString(theEnv,STDERR,"Defgenerics are not allowed to replace deffunctions.\n");
        }
      return false;
     }
#endif

   /*===========================================*/
   /* See if the defgeneric already exists in   */
   /* this module (or is imported from another) */
   /*===========================================*/

   theDefgeneric = FindDefgenericInModule(theEnv,theDefgenericName);
   if (theDefgeneric != NULL)
     {
      /* ===========================================
         And the redefinition of a defgeneric in
         the current module is only valid if none
         of its methods are executing
         =========================================== */
      if (MethodsExecuting(theDefgeneric))
        {
         MethodAlterError(theEnv,theDefgeneric);
         return false;
        }
     }

   /* =======================================
      Only certain specific system functions
      may be overloaded by generic functions
      ======================================= */
   systemFunction = FindFunction(theEnv,theDefgenericName);
   if ((systemFunction != NULL) ?
       (systemFunction->overloadable == false) : false)
     {
      PrintErrorID(theEnv,"GENRCPSR",16,false);
      WriteString(theEnv,STDERR,"The system function '");
      WriteString(theEnv,STDERR,theDefgenericName);
      WriteString(theEnv,STDERR,"' cannot be overloaded.\n");
      return false;
     }
   return true;
  }

#if DEBUGGING_FUNCTIONS

/***************************************************
  NAME         : CreateDefaultGenericPPForm
  DESCRIPTION  : Adds a default pretty-print form
                 for a gneric function when it is
                 impliciylt created by the defn
                 of its first method
  INPUTS       : The generic function
  RETURNS      : Nothing useful
  SIDE EFFECTS : Pretty-print form created and
                 attached.
  NOTES        : None
 ***************************************************/
static void CreateDefaultGenericPPForm(
  Environment *theEnv,
  Defgeneric *gfunc)
  {
   const char *moduleName, *genericName;
   char *buf;

   moduleName = DefmoduleName(GetCurrentModule(theEnv));
   genericName = DefgenericName(gfunc);
   buf = (char *) gm2(theEnv,(sizeof(char) * (strlen(moduleName) + strlen(genericName) + 17)));
   gensprintf(buf,"(defgeneric %s::%s)\n",moduleName,genericName);
   SetDefgenericPPForm(theEnv,gfunc,buf);
  }

#endif

/*******************************************************
  NAME         : ParseMethodNameAndIndex
  DESCRIPTION  : Parses the name of the method and
                   optional method index
  INPUTS       : 1) The logical name of the input source
                 2) Caller's buffer for method index
                    (0 if not specified)
  RETURNS      : The symbolic name of the method
  SIDE EFFECTS : None
  NOTES        : Assumes "(defmethod " already parsed
 *******************************************************/
static CLIPSLexeme *ParseMethodNameAndIndex(
  Environment *theEnv,
  const char *readSource,
  unsigned short *theIndex,
  struct token *genericInputToken)
  {
   CLIPSLexeme *gname;

   *theIndex = 0;
   gname = GetConstructNameAndComment(theEnv,readSource,genericInputToken,"defgeneric",
                                      (FindConstructFunction *) FindDefgenericInModule,
                                      NULL,"&",true,false,true,true);
   if (gname == NULL)
     return NULL;
   if (genericInputToken->tknType == INTEGER_TOKEN)
     {
      unsigned short tmp;

      PPBackup(theEnv);
      PPBackup(theEnv);
      SavePPBuffer(theEnv," ");
      SavePPBuffer(theEnv,genericInputToken->printForm);
      tmp = (unsigned short) genericInputToken->integerValue->contents;
      if (tmp < 1)
        {
         PrintErrorID(theEnv,"GENRCPSR",6,false);
         WriteString(theEnv,STDERR,"Method index out of range.\n");
         return NULL;
        }
      *theIndex = tmp;
      PPCRAndIndent(theEnv);
      GetToken(theEnv,readSource,genericInputToken);
     }
   if (genericInputToken->tknType == STRING_TOKEN)
     {
      PPBackup(theEnv);
      PPBackup(theEnv);
      SavePPBuffer(theEnv," ");
      SavePPBuffer(theEnv,genericInputToken->printForm);
      PPCRAndIndent(theEnv);
      GetToken(theEnv,readSource,genericInputToken);
     }
   return(gname);
  }

/************************************************************************
  NAME         : ParseMethodParameters
  DESCRIPTION  : Parses method restrictions
                   (parameter names with class and expression specifiers)
  INPUTS       : 1) The logical name of the input source
                 2) Caller's buffer for the parameter name list
                    (Restriction structures are attached to
                     argList pointers of parameter nodes)
                 3) Caller's buffer for wildcard symbol (if any)
  RETURNS      : The number of parameters, or -1 on errors
  SIDE EFFECTS : Memory allocated for parameters and restrictions
                 Parameter names stored in expression list
                 Parameter restrictions stored in contiguous array
  NOTES        : Any memory allocated is freed on errors
                 Assumes first opening parenthesis has been scanned
 ************************************************************************/
static unsigned short ParseMethodParameters(
  Environment *theEnv,
  const char *readSource,
  Expression **params,
  CLIPSLexeme **wildcard,
  struct token *genericInputToken)
  {
   Expression *phead = NULL,*pprv;
   CLIPSLexeme *pname;
   RESTRICTION *rtmp;
   unsigned short rcnt = 0;

   *wildcard = NULL;
   *params = NULL;
   if (genericInputToken->tknType != LEFT_PARENTHESIS_TOKEN)
     {
      PrintErrorID(theEnv,"GENRCPSR",7,false);
      WriteString(theEnv,STDERR,"Expected a '(' to begin method parameter restrictions.\n");
      return PARAMETER_ERROR;
     }
   GetToken(theEnv,readSource,genericInputToken);
   while (genericInputToken->tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      if (*wildcard != NULL)
        {
         DeleteTempRestricts(theEnv,phead);
         PrintErrorID(theEnv,"PRCCODE",8,false);
         WriteString(theEnv,STDERR,"No parameters allowed after wildcard parameter.\n");
         return PARAMETER_ERROR;
        }
      if ((genericInputToken->tknType == SF_VARIABLE_TOKEN) ||
          (genericInputToken->tknType == MF_VARIABLE_TOKEN))
        {
         pname = genericInputToken->lexemeValue;
         if (DuplicateParameters(theEnv,phead,&pprv,pname))
           {
            DeleteTempRestricts(theEnv,phead);
            return PARAMETER_ERROR;
           }
         if (genericInputToken->tknType == MF_VARIABLE_TOKEN)
           *wildcard = pname;
         rtmp = get_struct(theEnv,restriction);
         PackRestrictionTypes(theEnv,rtmp,NULL);
         rtmp->query = NULL;
         phead = AddParameter(theEnv,phead,pprv,pname,rtmp);
         rcnt++;
        }
      else if (genericInputToken->tknType == LEFT_PARENTHESIS_TOKEN)
        {
         GetToken(theEnv,readSource,genericInputToken);
         if ((genericInputToken->tknType != SF_VARIABLE_TOKEN) &&
             (genericInputToken->tknType != MF_VARIABLE_TOKEN))
           {
            DeleteTempRestricts(theEnv,phead);
            PrintErrorID(theEnv,"GENRCPSR",8,false);
            WriteString(theEnv,STDERR,"Expected a variable for parameter specification.\n");
            return PARAMETER_ERROR;
           }
         pname = genericInputToken->lexemeValue;
         if (DuplicateParameters(theEnv,phead,&pprv,pname))
           {
            DeleteTempRestricts(theEnv,phead);
            return PARAMETER_ERROR;
           }
         if (genericInputToken->tknType == MF_VARIABLE_TOKEN)
           *wildcard = pname;
         SavePPBuffer(theEnv," ");
         rtmp = ParseRestriction(theEnv,readSource);
         if (rtmp == NULL)
           {
            DeleteTempRestricts(theEnv,phead);
            return PARAMETER_ERROR;
           }
         phead = AddParameter(theEnv,phead,pprv,pname,rtmp);
         rcnt++;
        }
      else
        {
         DeleteTempRestricts(theEnv,phead);
         PrintErrorID(theEnv,"GENRCPSR",9,false);
         WriteString(theEnv,STDERR,"Expected a variable or '(' for parameter specification.\n");
         return PARAMETER_ERROR;
        }
      PPCRAndIndent(theEnv);
      GetToken(theEnv,readSource,genericInputToken);
     }
   if (rcnt != 0)
     {
      PPBackup(theEnv);
      PPBackup(theEnv);
      SavePPBuffer(theEnv,")");
     }
   *params = phead;
   return(rcnt);
  }

/************************************************************
  NAME         : ParseRestriction
  DESCRIPTION  : Parses the restriction for a parameter of a
                   method
                 This restriction is comprised of:
                   1) A list of classes (or types) that are
                      allowed for the parameter (None
                      if no type restriction)
                   2) And an optional restriction-query
                      expression
  INPUTS       : The logical name of the input source
  RETURNS      : The address of a RESTRICTION node, NULL on
                   errors
  SIDE EFFECTS : RESTRICTION node allocated
                   Types are in a contiguous array of void *
                   Query is an expression
  NOTES        : Assumes "(?<var> " has already been parsed
                 H/L Syntax: <type>* [<query>])
 ************************************************************/
static RESTRICTION *ParseRestriction(
  Environment *theEnv,
  const char *readSource)
  {
   Expression *types = NULL,*new_types,
              *typesbot,*tmp,*tmp2,
              *query = NULL;
   RESTRICTION *rptr;
   struct token genericInputToken;

   GetToken(theEnv,readSource,&genericInputToken);
   while (genericInputToken.tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      if (query != NULL)
        {
         PrintErrorID(theEnv,"GENRCPSR",10,false);
         WriteString(theEnv,STDERR,"Query must be last in parameter restriction.\n");
         ReturnExpression(theEnv,query);
         ReturnExpression(theEnv,types);
         return NULL;
        }
      if (genericInputToken.tknType == SYMBOL_TOKEN)
        {
         new_types = ValidType(theEnv,genericInputToken.lexemeValue);
         if (new_types == NULL)
           {
            ReturnExpression(theEnv,types);
            ReturnExpression(theEnv,query);
            return NULL;
           }
         if (types == NULL)
           types = new_types;
         else
           {
            for (typesbot = tmp = types ; tmp != NULL ; tmp = tmp->nextArg)
              {
               for (tmp2 = new_types ; tmp2 != NULL ; tmp2 = tmp2->nextArg)
                 {
                  if (tmp->value == tmp2->value)
                    {
                     PrintErrorID(theEnv,"GENRCPSR",11,false);
#if OBJECT_SYSTEM
                     WriteString(theEnv,STDERR,"Duplicate classes not allowed in parameter restriction.\n");
#else
                     WriteString(theEnv,STDERR,"Duplicate types not allowed in parameter restriction.\n");
#endif
                     ReturnExpression(theEnv,query);
                     ReturnExpression(theEnv,types);
                     ReturnExpression(theEnv,new_types);
                     return NULL;
                    }
                  if (RedundantClasses(theEnv,tmp->value,tmp2->value))
                    {
                     ReturnExpression(theEnv,query);
                     ReturnExpression(theEnv,types);
                     ReturnExpression(theEnv,new_types);
                     return NULL;
                    }
                 }
               typesbot = tmp;
              }
            typesbot->nextArg = new_types;
           }
        }
      else if (genericInputToken.tknType == LEFT_PARENTHESIS_TOKEN)
        {
         query = Function1Parse(theEnv,readSource);
         if (query == NULL)
           {
            ReturnExpression(theEnv,types);
            return NULL;
           }
         if (GetParsedBindNames(theEnv) != NULL)
           {
            PrintErrorID(theEnv,"GENRCPSR",12,false);
            WriteString(theEnv,STDERR,"Binds are not allowed in query expressions.\n");
            ReturnExpression(theEnv,query);
            ReturnExpression(theEnv,types);
            return NULL;
           }
        }
#if DEFGLOBAL_CONSTRUCT
      else if (genericInputToken.tknType == GBL_VARIABLE_TOKEN)
        query = GenConstant(theEnv,GBL_VARIABLE,genericInputToken.value);
#endif
      else
        {
         PrintErrorID(theEnv,"GENRCPSR",13,false);
#if OBJECT_SYSTEM
         WriteString(theEnv,STDERR,"Expected a valid class name or query.\n");
#else
         WriteString(theEnv,STDERR,"Expected a valid type name or query.\n");
#endif
         ReturnExpression(theEnv,query);
         ReturnExpression(theEnv,types);
         return NULL;
        }
      SavePPBuffer(theEnv," ");
      GetToken(theEnv,readSource,&genericInputToken);
     }
   PPBackup(theEnv);
   PPBackup(theEnv);
   SavePPBuffer(theEnv,")");
   if ((types == NULL) && (query == NULL))
     {
      PrintErrorID(theEnv,"GENRCPSR",13,false);
#if OBJECT_SYSTEM
      WriteString(theEnv,STDERR,"Expected a valid class name or query.\n");
#else
      WriteString(theEnv,STDERR,"Expected a valid type name or query.\n");
#endif
      return NULL;
     }
   rptr = get_struct(theEnv,restriction);
   rptr->query = query;
   PackRestrictionTypes(theEnv,rptr,types);
   return(rptr);
  }

/*****************************************************************
  NAME         : ReplaceCurrentArgRefs
  DESCRIPTION  : Replaces all references to ?current-argument in
                  method parameter queries with special calls
                  to (gnrc-current-arg)
  INPUTS       : The query expression
  RETURNS      : Nothing useful
  SIDE EFFECTS : Variable references to ?current-argument replaced
  NOTES        : None
 *****************************************************************/
static void ReplaceCurrentArgRefs(
  Environment *theEnv,
  Expression *query)
  {
   while (query != NULL)
     {
      if ((query->type != SF_VARIABLE) ? false :
          (strcmp(query->lexemeValue->contents,CURR_ARG_VAR) == 0))
        {
         query->type = FCALL;
         query->value = FindFunction(theEnv,"(gnrc-current-arg)");
        }
      if (query->argList != NULL)
        ReplaceCurrentArgRefs(theEnv,query->argList);
      query = query->nextArg;
     }
  }

/**********************************************************
  NAME         : DuplicateParameters
  DESCRIPTION  : Examines the parameter expression
                   chain for a method looking duplicates.
  INPUTS       : 1) The parameter chain (can be NULL)
                 2) Caller's buffer for address of
                    last node searched (can be used to
                    later attach new parameter)
                 3) The name of the parameter being checked
  RETURNS      : True if duplicates found, false otherwise
  SIDE EFFECTS : Caller's prv address set
  NOTES        : Assumes all parameter list nodes are WORDS
 **********************************************************/
static bool DuplicateParameters(
  Environment *theEnv,
  Expression *head,
  Expression **prv,
  CLIPSLexeme *name)
  {
   *prv = NULL;
   while (head != NULL)
     {
      if (head->value == (void *) name)
        {
         PrintErrorID(theEnv,"PRCCODE",7,false);
         WriteString(theEnv,STDERR,"Duplicate parameter names not allowed.\n");
         return true;
        }
      *prv = head;
      head = head->nextArg;
     }
   return false;
  }

/*****************************************************************
  NAME         : AddParameter
  DESCRIPTION  : Shoves a new paramter with its restriction
                   onto the list for a method
                 The parameter list is a list of expressions
                   linked by neext_arg pointers, and the
                   argList pointers are used for the restrictions
  INPUTS       : 1) The head of the list
                 2) The bottom of the list
                 3) The parameter name
                 4) The parameter restriction
  RETURNS      : The (new) head of the list
  SIDE EFFECTS : New parameter expression node allocated, set,
                   and attached
  NOTES        : None
 *****************************************************************/
static Expression *AddParameter(
  Environment *theEnv,
  Expression *phead,
  Expression *pprv,
  CLIPSLexeme *pname,
  RESTRICTION *rptr)
  {
   Expression *ptmp;

   ptmp = GenConstant(theEnv,SYMBOL_TYPE,pname);
   if (phead == NULL)
     phead = ptmp;
   else
     pprv->nextArg = ptmp;
   ptmp->argList = (Expression *) rptr;
   return(phead);
  }

/**************************************************************
  NAME         : ValidType
  DESCRIPTION  : Examines the name of a restriction type and
                   forms a list of integer-code expressions
                   corresponding to the primitive types
                 (or a Class address if COOL is installed)
  INPUTS       : The type name
  RETURNS      : The expression chain (NULL on errors)
  SIDE EFFECTS : Expression type chain allocated
                   one or more nodes holding codes for types
                   (or class addresses)
  NOTES        : None
 *************************************************************/
static Expression *ValidType(
  Environment *theEnv,
  CLIPSLexeme *tname)
  {
#if OBJECT_SYSTEM
   Defclass *cls;

   if (FindModuleSeparator(tname->contents))
     IllegalModuleSpecifierMessage(theEnv);
   else
     {
      cls = LookupDefclassInScope(theEnv,tname->contents);
      if (cls == NULL)
        {
         PrintErrorID(theEnv,"GENRCPSR",14,false);
         WriteString(theEnv,STDERR,"Unknown class in method.\n");
         return NULL;
        }
      return(GenConstant(theEnv,DEFCLASS_PTR,cls));
     }
#else
   if (strcmp(tname->contents,INTEGER_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,INTEGER_TYPE)));
   if (strcmp(tname->contents,FLOAT_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,FLOAT_TYPE)));
   if (strcmp(tname->contents,SYMBOL_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,SYMBOL_TYPE)));
   if (strcmp(tname->contents,STRING_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,STRING_TYPE)));
   if (strcmp(tname->contents,MULTIFIELD_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,MULTIFIELD_TYPE)));
   if (strcmp(tname->contents,EXTERNAL_ADDRESS_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,EXTERNAL_ADDRESS_TYPE)));
   if (strcmp(tname->contents,FACT_ADDRESS_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,FACT_ADDRESS_TYPE)));
   if (strcmp(tname->contents,NUMBER_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,NUMBER_TYPE_CODE)));
   if (strcmp(tname->contents,LEXEME_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,LEXEME_TYPE_CODE)));
   if (strcmp(tname->contents,ADDRESS_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,ADDRESS_TYPE_CODE)));
   if (strcmp(tname->contents,PRIMITIVE_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,PRIMITIVE_TYPE_CODE)));
   if (strcmp(tname->contents,OBJECT_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,OBJECT_TYPE_CODE)));
   if (strcmp(tname->contents,INSTANCE_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,INSTANCE_TYPE_CODE)));
   if (strcmp(tname->contents,INSTANCE_NAME_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,INSTANCE_NAME_TYPE)));
   if (strcmp(tname->contents,INSTANCE_ADDRESS_TYPE_NAME) == 0)
     return(GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,INSTANCE_ADDRESS_TYPE)));

   PrintErrorID(theEnv,"GENRCPSR",14,false);
   WriteString(theEnv,STDERR,"Unknown type in method.\n");
#endif
   return NULL;
  }

/*************************************************************
  NAME         : RedundantClasses
  DESCRIPTION  : Determines if one class (type) is
                 subsumes (or is subsumed by) another.
  INPUTS       : Two void pointers which are class pointers
                 if COOL is installed or integer hash nodes
                 for type codes otherwise.
  RETURNS      : True if there is subsumption, false otherwise
  SIDE EFFECTS : An error message is printed, if appropriate.
  NOTES        : None
 *************************************************************/
static bool RedundantClasses(
  Environment *theEnv,
  void *c1,
  void *c2)
  {
   const char *tname;

#if OBJECT_SYSTEM
   if (HasSuperclass((Defclass *) c1,(Defclass *) c2))
     tname = DefclassName((Defclass *) c1);
   else if (HasSuperclass((Defclass *) c2,(Defclass *) c1))
     tname = DefclassName((Defclass *) c2);
#else
   if (SubsumeType(((CLIPSInteger *) c1)->contents,((CLIPSInteger *) c2)->contents))
     tname = TypeName(theEnv,((CLIPSInteger *) c1)->contents);
   else if (SubsumeType(((CLIPSInteger *) c2)->contents,((CLIPSInteger *) c1)->contents))
     tname = TypeName(theEnv,((CLIPSInteger *) c2)->contents);
#endif
   else
     return false;
   PrintErrorID(theEnv,"GENRCPSR",15,false);
   WriteString(theEnv,STDERR,"Class '");
   WriteString(theEnv,STDERR,tname);
   WriteString(theEnv,STDERR,"' is redundant.\n");
   return true;
  }

/*********************************************************
  NAME         : AddGeneric
  DESCRIPTION  : Inserts a new generic function
                   header into the generic list
  INPUTS       : 1) Symbolic name of the new generic
                 2) Caller's input buffer for flag
                      if added generic is new or not
  RETURNS      : The address of the new node, or
                   address of old node if already present
  SIDE EFFECTS : Generic header inserted
                 If the node is already present, it is
                   moved to the end of the list, otherwise
                   the new node is inserted at the end
  NOTES        : None
 *********************************************************/
static Defgeneric *AddGeneric(
  Environment *theEnv,
  CLIPSLexeme *name,
  bool *newGeneric)
  {
   Defgeneric *gfunc;

   gfunc = FindDefgenericInModule(theEnv,name->contents);
   if (gfunc != NULL)
     {
      *newGeneric = false;

      if (ConstructData(theEnv)->CheckSyntaxMode)
        { return(gfunc); }

      /* ================================
         The old trace state is preserved
         ================================ */
      RemoveConstructFromModule(theEnv,&gfunc->header);
     }
   else
     {
      *newGeneric = true;
      gfunc = NewGeneric(theEnv,name);
      IncrementLexemeCount(name);
      AddImplicitMethods(theEnv,gfunc);
     }
   AddConstructToModule(&gfunc->header);
   return(gfunc);
  }

/**********************************************************************
  NAME         : AddGenericMethod
  DESCRIPTION  : Inserts a blank method (with the method-index set)
                   into the specified position of the generic
                   method array
  INPUTS       : 1) The generic function
                 2) The index where to add the method in the array
                 3) The method user-index (0 if don't care)
  RETURNS      : The address of the new method
  SIDE EFFECTS : Fields initialized (index set) and new method inserted
                 Generic function new method-index set to specified
                   by user-index if > current new method-index
  NOTES        : None
 **********************************************************************/
static Defmethod *AddGenericMethod(
  Environment *theEnv,
  Defgeneric *gfunc,
  int mposn,
  unsigned short mi)
  {
   Defmethod *narr;
   long b, e;

   narr = (Defmethod *) gm2(theEnv,(sizeof(Defmethod) * (gfunc->mcnt+1)));
   for (b = e = 0 ; b < gfunc->mcnt ; b++ , e++)
     {
      if (b == mposn)
        e++;
      GenCopyMemory(Defmethod,1,&narr[e],&gfunc->methods[b]);
     }
   if (mi == 0)
     narr[mposn].index = gfunc->new_index++;
   else
     {
      narr[mposn].index = mi;
      if (mi >= gfunc->new_index)
        gfunc->new_index = mi + 1;
     }
   narr[mposn].busy = 0;
#if DEBUGGING_FUNCTIONS
   narr[mposn].trace = DefgenericData(theEnv)->WatchMethods;
#endif
   narr[mposn].minRestrictions = 0;
   narr[mposn].maxRestrictions = 0;
   narr[mposn].restrictionCount = 0;
   narr[mposn].localVarCount = 0;
   narr[mposn].system = 0;
   narr[mposn].restrictions = NULL;
   narr[mposn].actions = NULL;
   narr[mposn].header.name = NULL;
   narr[mposn].header.next = NULL;
   narr[mposn].header.constructType = DEFMETHOD;
   narr[mposn].header.env = theEnv;
   narr[mposn].header.whichModule = gfunc->header.whichModule;
   narr[mposn].header.ppForm = NULL;
   narr[mposn].header.usrData = NULL;

   if (gfunc->mcnt != 0)
     rm(theEnv,gfunc->methods,(sizeof(Defmethod) * gfunc->mcnt));
   gfunc->mcnt++;
   gfunc->methods = narr;
   return(&narr[mposn]);
  }

/****************************************************************
  NAME         : RestrictionsCompare
  DESCRIPTION  : Compares the restriction-expression list
                   with an existing methods restrictions to
                   determine an ordering
  INPUTS       : 1) The parameter/restriction expression list
                 2) The total number of restrictions
                 3) The number of minimum restrictions
                 4) The number of maximum restrictions (-1
                    if unlimited)
                 5) The method with which to compare restrictions
  RETURNS      : A code representing how the method restrictions
                   -1 : New restrictions have higher precedence
                    0 : New restrictions are identical
                    1 : New restrictions have lower precedence
  SIDE EFFECTS : None
  NOTES        : The new restrictions are stored in the argList
                   pointers of the parameter expressions
 ****************************************************************/
static int RestrictionsCompare(
  Expression *params,
  int rcnt,
  int min,
  int max,
  Defmethod *meth)
  {
   int i;
   RESTRICTION *r1,*r2;
   bool diff = false;
   int rtn;

   for (i = 0 ; (i < rcnt) && (i < meth->restrictionCount) ; i++)
     {
      /* =============================================================
         A wildcard parameter always has lower precedence than
         a regular parameter, regardless of the class restriction list
         ============================================================= */
      if ((i == rcnt-1) && (max == -1) &&
          (meth->maxRestrictions != RESTRICTIONS_UNBOUNDED))
        return LOWER_PRECEDENCE;
      if ((i == meth->restrictionCount-1) && (max != -1) &&
          (meth->maxRestrictions == RESTRICTIONS_UNBOUNDED))
        return HIGHER_PRECEDENCE;

      /* =============================================================
         The parameter with the most specific type list has precedence
         ============================================================= */
      r1 = (RESTRICTION *) params->argList;
      r2 = &meth->restrictions[i];
      rtn = TypeListCompare(r1,r2);
      if (rtn != IDENTICAL)
        return rtn;

      /* =====================================================
         The parameter with a query restriction has precedence
         ===================================================== */
      if ((r1->query == NULL) && (r2->query != NULL))
        return LOWER_PRECEDENCE;
      if ((r1->query != NULL) && (r2->query == NULL))
        return HIGHER_PRECEDENCE;

      /* ==========================================================
         Remember if the method restrictions differ at all - query
         expressions must be identical as well for the restrictions
         to be the same
         ========================================================== */
      if (IdenticalExpression(r1->query,r2->query) == false)
        diff = true;
      params = params->nextArg;
     }

   /* =============================================================
      If the methods have the same number of parameters here, they
      are either the same restrictions, or they differ only in
      the query restrictions
      ============================================================= */
   if (rcnt == meth->restrictionCount)
     return(diff ? LOWER_PRECEDENCE : IDENTICAL);

   /* =============================================
      The method with the greater number of regular
      parameters has precedence

      If they require the smae # of reg params,
      then one without a wildcard has precedence
      ============================================= */
   if (min > meth->minRestrictions)
     return HIGHER_PRECEDENCE;
   if (meth->minRestrictions < min)
     return LOWER_PRECEDENCE;
   return((max == - 1) ? LOWER_PRECEDENCE : HIGHER_PRECEDENCE);
  }

/*****************************************************
  NAME         : TypeListCompare
  DESCRIPTION  : Determines the precedence between
                   the class lists on two restrictions
  INPUTS       : 1) Restriction address #1
                 2) Restriction address #2
  RETURNS      : -1 : r1 precedes r2
                  0 : Identical classes
                  1 : r2 precedes r1
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************/
static int TypeListCompare(
  RESTRICTION *r1,
  RESTRICTION *r2)
  {
   long i;
   bool diff = false;

   if ((r1->tcnt == 0) && (r2->tcnt == 0))
     return(IDENTICAL);
   if (r1->tcnt == 0)
     return(LOWER_PRECEDENCE);
   if (r2->tcnt == 0)
     return(HIGHER_PRECEDENCE);
   for (i = 0 ; (i < r1->tcnt) && (i < r2->tcnt) ; i++)
     {
      if (r1->types[i] != r2->types[i])
        {
         diff = true;
#if OBJECT_SYSTEM
         if (HasSuperclass((Defclass *) r1->types[i],(Defclass *) r2->types[i]))
           return(HIGHER_PRECEDENCE);
         if (HasSuperclass((Defclass *) r2->types[i],(Defclass *) r1->types[i]))
           return(LOWER_PRECEDENCE);
#else
         if (SubsumeType(((CLIPSInteger *) r1->types[i])->contents,((CLIPSInteger *) r2->types[i])->contents))
           return(HIGHER_PRECEDENCE);
         if (SubsumeType(((CLIPSInteger *) r2->types[i])->contents,((CLIPSInteger *) r1->types[i])->contents))
           return(LOWER_PRECEDENCE);
#endif
        }
     }
   if (r1->tcnt < r2->tcnt)
     return(HIGHER_PRECEDENCE);
   if (r1->tcnt > r2->tcnt)
     return(LOWER_PRECEDENCE);
   if (diff)
     return(LOWER_PRECEDENCE);
   return(IDENTICAL);
  }

/***************************************************
  NAME         : NewGeneric
  DESCRIPTION  : Allocates and initializes a new
                   generic function header
  INPUTS       : The name of the new generic
  RETURNS      : The address of the new generic
  SIDE EFFECTS : Generic function  header created
  NOTES        : None
 ***************************************************/
static Defgeneric *NewGeneric(
  Environment *theEnv,
  CLIPSLexeme *gname)
  {
   Defgeneric *ngen;

   ngen = get_struct(theEnv,defgeneric);
   InitializeConstructHeader(theEnv,"defgeneric",DEFGENERIC,&ngen->header,gname);
   ngen->busy = 0;
   ngen->new_index = 1;
   ngen->methods = NULL;
   ngen->mcnt = 0;
#if DEBUGGING_FUNCTIONS
   ngen->trace = DefgenericData(theEnv)->WatchGenerics;
#endif
   return(ngen);
  }

#endif /* DEFGENERIC_CONSTRUCT && (! BLOAD_ONLY) && (! RUN_TIME) */

/***************************************************
  NAME         :
  DESCRIPTION  :
  INPUTS       :
  RETURNS      :
  SIDE EFFECTS :
  NOTES        :
 ***************************************************/
