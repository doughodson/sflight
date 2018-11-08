   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/01/16             */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose: Generic Function Execution Routines              */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Removed IMPERATIVE_METHODS compilation flag.   */
/*                                                           */
/*      6.30: Changed garbage collection algorithm.          */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.40: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Added GCBlockStart and GCBlockEnd functions    */
/*            for garbage collection blocks.                 */
/*                                                           */
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if DEFGENERIC_CONSTRUCT

#include <string.h>

#if OBJECT_SYSTEM
#include "classcom.h"
#include "classfun.h"
#include "insfun.h"
#endif

#include "argacces.h"
#include "constrct.h"
#include "envrnmnt.h"
#include "genrccom.h"
#include "prcdrfun.h"
#include "prccode.h"
#include "prntutil.h"
#include "proflfun.h"
#include "router.h"
#include "utility.h"

#include "genrcexe.h"

/* =========================================
   *****************************************
                   CONSTANTS
   =========================================
   ***************************************** */

#define BEGIN_TRACE     ">>"
#define END_TRACE       "<<"

/* =========================================
   *****************************************
      INTERNALLY VISIBLE FUNCTION HEADERS
   =========================================
   ***************************************** */

   static Defmethod              *FindApplicableMethod(Environment *,Defgeneric *,Defmethod *);

#if DEBUGGING_FUNCTIONS
   static void                    WatchGeneric(Environment *,const char *);
   static void                    WatchMethod(Environment *,const char *);
#endif

#if OBJECT_SYSTEM
   static Defclass               *DetermineRestrictionClass(Environment *,UDFValue *);
#endif

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************************************
  NAME         : GenericDispatch
  DESCRIPTION  : Executes the most specific applicable method
  INPUTS       : 1) The generic function
                 2) The method to start after in the search for an applicable
                    method (ignored if arg #3 is not NULL).
                 3) A specific method to call (NULL if want highest precedence
                    method to be called)
                 4) The generic function argument expressions
                 5) The caller's result value buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Any side-effects of evaluating the generic function arguments
                 Any side-effects of evaluating query functions on method parameter
                   restrictions when determining the core (see warning #1)
                 Any side-effects of actual execution of methods (see warning #2)
                 Caller's buffer set to the result of the generic function call

                 In case of errors, the result is false, otherwise it is the
                   result returned by the most specific method (which can choose
                   to ignore or return the values of more general methods)
  NOTES        : WARNING #1: Query functions on method parameter restrictions
                    should not have side-effects, for they might be evaluated even
                    for methods that aren't applicable to the generic function call.
                 WARNING #2: Side-effects of method execution should not always rely
                    on only being executed once per generic function call.  Every
                    time a method calls (shadow-call) the same next-most-specific
                    method is executed.  Thus, it is possible for a method to be
                    executed multiple times per generic function call.
 ***********************************************************************************/
void GenericDispatch(
  Environment *theEnv,
  Defgeneric *gfunc,
  Defmethod *prevmeth,
  Defmethod *meth,
  Expression *params,
  UDFValue *returnValue)
  {
   Defgeneric *previousGeneric;
   Defmethod *previousMethod;
   bool oldce;
#if PROFILING_FUNCTIONS
   struct profileFrameInfo profileFrame;
#endif
   GCBlock gcb;

   returnValue->value = FalseSymbol(theEnv);
   EvaluationData(theEnv)->EvaluationError = false;
   if (EvaluationData(theEnv)->HaltExecution)
     return;

   GCBlockStart(theEnv,&gcb);

   oldce = ExecutingConstruct(theEnv);
   SetExecutingConstruct(theEnv,true);
   previousGeneric = DefgenericData(theEnv)->CurrentGeneric;
   previousMethod = DefgenericData(theEnv)->CurrentMethod;
   DefgenericData(theEnv)->CurrentGeneric = gfunc;
   EvaluationData(theEnv)->CurrentEvaluationDepth++;
   gfunc->busy++;
   PushProcParameters(theEnv,params,CountArguments(params),
                      DefgenericName(gfunc),
                      "generic function",UnboundMethodErr);
   if (EvaluationData(theEnv)->EvaluationError)
     {
      gfunc->busy--;
      DefgenericData(theEnv)->CurrentGeneric = previousGeneric;
      DefgenericData(theEnv)->CurrentMethod = previousMethod;
      EvaluationData(theEnv)->CurrentEvaluationDepth--;

      GCBlockEndUDF(theEnv,&gcb,returnValue);
      CallPeriodicTasks(theEnv);

      SetExecutingConstruct(theEnv,oldce);
      return;
     }
   if (meth != NULL)
     {
      if (IsMethodApplicable(theEnv,meth))
        {
         meth->busy++;
         DefgenericData(theEnv)->CurrentMethod = meth;
        }
      else
        {
         PrintErrorID(theEnv,"GENRCEXE",4,false);
         SetEvaluationError(theEnv,true);
         DefgenericData(theEnv)->CurrentMethod = NULL;
         WriteString(theEnv,STDERR,"Generic function '");
         WriteString(theEnv,STDERR,DefgenericName(gfunc));
         WriteString(theEnv,STDERR,"' method #");
         PrintUnsignedInteger(theEnv,STDERR,meth->index);
         WriteString(theEnv,STDERR," is not applicable to the given arguments.\n");
        }
     }
   else
     DefgenericData(theEnv)->CurrentMethod = FindApplicableMethod(theEnv,gfunc,prevmeth);
   if (DefgenericData(theEnv)->CurrentMethod != NULL)
     {
#if DEBUGGING_FUNCTIONS
      if (DefgenericData(theEnv)->CurrentGeneric->trace)
        WatchGeneric(theEnv,BEGIN_TRACE);
      if (DefgenericData(theEnv)->CurrentMethod->trace)
        WatchMethod(theEnv,BEGIN_TRACE);
#endif
      if (DefgenericData(theEnv)->CurrentMethod->system)
        {
         Expression fcall;

         fcall.type = FCALL;
         fcall.value = DefgenericData(theEnv)->CurrentMethod->actions->value;
         fcall.nextArg = NULL;
         fcall.argList = GetProcParamExpressions(theEnv);
         EvaluateExpression(theEnv,&fcall,returnValue);
        }
      else
        {
#if PROFILING_FUNCTIONS
         StartProfile(theEnv,&profileFrame,
                      &DefgenericData(theEnv)->CurrentMethod->header.usrData,
                      ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

         EvaluateProcActions(theEnv,DefgenericData(theEnv)->CurrentGeneric->header.whichModule->theModule,
                             DefgenericData(theEnv)->CurrentMethod->actions,DefgenericData(theEnv)->CurrentMethod->localVarCount,
                             returnValue,UnboundMethodErr);

#if PROFILING_FUNCTIONS
         EndProfile(theEnv,&profileFrame);
#endif
        }
      DefgenericData(theEnv)->CurrentMethod->busy--;
#if DEBUGGING_FUNCTIONS
      if (DefgenericData(theEnv)->CurrentMethod->trace)
        WatchMethod(theEnv,END_TRACE);
      if (DefgenericData(theEnv)->CurrentGeneric->trace)
        WatchGeneric(theEnv,END_TRACE);
#endif
     }
   else if (! EvaluationData(theEnv)->EvaluationError)
     {
      PrintErrorID(theEnv,"GENRCEXE",1,false);
      WriteString(theEnv,STDERR,"No applicable methods for '");
      WriteString(theEnv,STDERR,DefgenericName(gfunc));
      WriteString(theEnv,STDERR,"'.\n");
      SetEvaluationError(theEnv,true);
     }
   gfunc->busy--;
   ProcedureFunctionData(theEnv)->ReturnFlag = false;
   PopProcParameters(theEnv);
   DefgenericData(theEnv)->CurrentGeneric = previousGeneric;
   DefgenericData(theEnv)->CurrentMethod = previousMethod;
   EvaluationData(theEnv)->CurrentEvaluationDepth--;

   GCBlockEndUDF(theEnv,&gcb,returnValue);
   CallPeriodicTasks(theEnv);

   SetExecutingConstruct(theEnv,oldce);
  }

/*******************************************************
  NAME         : UnboundMethodErr
  DESCRIPTION  : Print out a synopis of the currently
                   executing method for unbound variable
                   errors
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Error synopsis printed to STDERR
  NOTES        : None
 *******************************************************/
void UnboundMethodErr(
  Environment *theEnv,
  const char *logName)
  {
   WriteString(theEnv,logName,"generic function '");
   WriteString(theEnv,logName,DefgenericName(DefgenericData(theEnv)->CurrentGeneric));
   WriteString(theEnv,logName,"' method #");
   PrintUnsignedInteger(theEnv,logName,DefgenericData(theEnv)->CurrentMethod->index);
   WriteString(theEnv,logName,".\n");
  }

/***********************************************************************
  NAME         : IsMethodApplicable
  DESCRIPTION  : Tests to see if a method satsifies the arguments of a
                   generic function
                 A method is applicable if all its restrictions are
                   satisfied by the corresponding arguments
  INPUTS       : The method address
  RETURNS      : True if method is applicable, false otherwise
  SIDE EFFECTS : Any query functions are evaluated
  NOTES        : Uses globals ProcParamArraySize and ProcParamArray
 ***********************************************************************/
bool IsMethodApplicable(
  Environment *theEnv,
  Defmethod *meth)
  {
   UDFValue temp;
   unsigned int i,j,k;
   RESTRICTION *rp;
#if OBJECT_SYSTEM
   Defclass *type;
#else
   int type;
#endif

   if (((ProceduralPrimitiveData(theEnv)->ProcParamArraySize < meth->minRestrictions) && (meth->minRestrictions != RESTRICTIONS_UNBOUNDED)) ||
       ((ProceduralPrimitiveData(theEnv)->ProcParamArraySize > meth->minRestrictions) && (meth->maxRestrictions != RESTRICTIONS_UNBOUNDED))) // TBD minRestrictions || maxRestrictions
     return false;
   for (i = 0 , k = 0 ; i < ProceduralPrimitiveData(theEnv)->ProcParamArraySize ; i++)
     {
      rp = &meth->restrictions[k];
      if (rp->tcnt != 0)
        {
#if OBJECT_SYSTEM
         type = DetermineRestrictionClass(theEnv,&ProceduralPrimitiveData(theEnv)->ProcParamArray[i]);
         if (type == NULL)
           return false;
         for (j = 0 ; j < rp->tcnt ; j++)
           {
            if (type == rp->types[j])
              break;
            if (HasSuperclass(type,(Defclass *) rp->types[j]))
              break;
            if (rp->types[j] == (void *) DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS_TYPE])
              {
               if (ProceduralPrimitiveData(theEnv)->ProcParamArray[i].header->type == INSTANCE_ADDRESS_TYPE)
                 break;
              }
            else if (rp->types[j] == (void *) DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME_TYPE])
              {
               if (ProceduralPrimitiveData(theEnv)->ProcParamArray[i].header->type == INSTANCE_NAME_TYPE)
                 break;
              }
            else if (rp->types[j] ==
                DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME_TYPE]->directSuperclasses.classArray[0])
              {
               if ((ProceduralPrimitiveData(theEnv)->ProcParamArray[i].header->type == INSTANCE_NAME_TYPE) ||
                   (ProceduralPrimitiveData(theEnv)->ProcParamArray[i].header->type == INSTANCE_ADDRESS_TYPE))
                 break;
              }
           }
#else
         type = ProceduralPrimitiveData(theEnv)->ProcParamArray[i].header->type;
         for (j = 0 ; j < rp->tcnt ; j++)
           {
            if (type == ((CLIPSInteger *) (rp->types[j]))->contents)
              break;
            if (SubsumeType(type,((CLIPSInteger *) (rp->types[j]))->contents))
              break;
           }
#endif
         if (j == rp->tcnt)
           return false;
        }
      if (rp->query != NULL)
        {
         DefgenericData(theEnv)->GenericCurrentArgument = &ProceduralPrimitiveData(theEnv)->ProcParamArray[i];
         EvaluateExpression(theEnv,rp->query,&temp);
         if (temp.value == FalseSymbol(theEnv))
           return false;
        }
      if ((k + 1) != meth->restrictionCount)
        k++;
     }
   return true;
  }

/***************************************************
  NAME         : NextMethodP
  DESCRIPTION  : Determines if a shadowed generic
                   function method is available for
                   execution
  INPUTS       : None
  RETURNS      : True if there is a method available,
                   false otherwise
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (next-methodp)
 ***************************************************/
bool NextMethodP(
  Environment *theEnv)
  {
   Defmethod *meth;

   if (DefgenericData(theEnv)->CurrentMethod == NULL)
     { return false; }

   meth = FindApplicableMethod(theEnv,DefgenericData(theEnv)->CurrentGeneric,DefgenericData(theEnv)->CurrentMethod);
   if (meth != NULL)
     {
      meth->busy--;
      return true;
     }
   else
     { return false; }
  }

void NextMethodPCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   returnValue->lexemeValue = CreateBoolean(theEnv,NextMethodP(theEnv));
  }

/****************************************************
  NAME         : CallNextMethod
  DESCRIPTION  : Executes the next available method
                   in the core for a generic function
  INPUTS       : Caller's buffer for the result
  RETURNS      : Nothing useful
  SIDE EFFECTS : Side effects of execution of shadow
                 EvaluationError set if no method
                   is available to execute.
  NOTES        : H/L Syntax: (call-next-method)
 ****************************************************/
void CallNextMethod(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Defmethod *oldMethod;
#if PROFILING_FUNCTIONS
   struct profileFrameInfo profileFrame;
#endif

   returnValue->lexemeValue = FalseSymbol(theEnv);

   if (EvaluationData(theEnv)->HaltExecution)
     return;
   oldMethod = DefgenericData(theEnv)->CurrentMethod;
   if (DefgenericData(theEnv)->CurrentMethod != NULL)
     DefgenericData(theEnv)->CurrentMethod = FindApplicableMethod(theEnv,DefgenericData(theEnv)->CurrentGeneric,DefgenericData(theEnv)->CurrentMethod);
   if (DefgenericData(theEnv)->CurrentMethod == NULL)
     {
      DefgenericData(theEnv)->CurrentMethod = oldMethod;
      PrintErrorID(theEnv,"GENRCEXE",2,false);
      WriteString(theEnv,STDERR,"Shadowed methods not applicable in current context.\n");
      SetEvaluationError(theEnv,true);
      return;
     }

#if DEBUGGING_FUNCTIONS
   if (DefgenericData(theEnv)->CurrentMethod->trace)
     WatchMethod(theEnv,BEGIN_TRACE);
#endif
   if (DefgenericData(theEnv)->CurrentMethod->system)
     {
      Expression fcall;

      fcall.type = FCALL;
      fcall.value = DefgenericData(theEnv)->CurrentMethod->actions->value;
      fcall.nextArg = NULL;
      fcall.argList = GetProcParamExpressions(theEnv);
      EvaluateExpression(theEnv,&fcall,returnValue);
     }
   else
     {
#if PROFILING_FUNCTIONS
      StartProfile(theEnv,&profileFrame,
                   &DefgenericData(theEnv)->CurrentGeneric->header.usrData,
                   ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

      EvaluateProcActions(theEnv,DefgenericData(theEnv)->CurrentGeneric->header.whichModule->theModule,
                         DefgenericData(theEnv)->CurrentMethod->actions,DefgenericData(theEnv)->CurrentMethod->localVarCount,
                         returnValue,UnboundMethodErr);

#if PROFILING_FUNCTIONS
      EndProfile(theEnv,&profileFrame);
#endif
     }

   DefgenericData(theEnv)->CurrentMethod->busy--;
#if DEBUGGING_FUNCTIONS
   if (DefgenericData(theEnv)->CurrentMethod->trace)
     WatchMethod(theEnv,END_TRACE);
#endif
   DefgenericData(theEnv)->CurrentMethod = oldMethod;
   ProcedureFunctionData(theEnv)->ReturnFlag = false;
  }

/**************************************************************************
  NAME         : CallSpecificMethod
  DESCRIPTION  : Allows a specific method to be called without regards to
                   higher precedence methods which might also be applicable
                   However, shadowed methods can still be called.
  INPUTS       : A data object buffer to hold the method evaluation result
  RETURNS      : Nothing useful
  SIDE EFFECTS : Side-effects of method applicability tests and the
                 evaluation of methods
  NOTES        : H/L Syntax: (call-specific-method
                                <generic-function> <method-index> <args>)
 **************************************************************************/
void CallSpecificMethod(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;
   Defgeneric *gfunc;
   int mi;

   returnValue->lexemeValue = FalseSymbol(theEnv);

   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg)) return;

   gfunc = CheckGenericExists(theEnv,"call-specific-method",theArg.lexemeValue->contents);
   if (gfunc == NULL) return;

   if (! UDFNextArgument(context,INTEGER_BIT,&theArg)) return;

   mi = CheckMethodExists(theEnv,"call-specific-method",gfunc,(unsigned short) theArg.integerValue->contents);
   if (mi == METHOD_NOT_FOUND)
     return;
   gfunc->methods[mi].busy++;
   GenericDispatch(theEnv,gfunc,NULL,&gfunc->methods[mi],
                   GetFirstArgument()->nextArg->nextArg,returnValue);
   gfunc->methods[mi].busy--;
  }

/***********************************************************************
  NAME         : OverrideNextMethod
  DESCRIPTION  : Changes the arguments to shadowed methods, thus the set
                 of applicable methods to this call may change
  INPUTS       : A buffer to hold the result of the call
  RETURNS      : Nothing useful
  SIDE EFFECTS : Any of evaluating method restrictions and bodies
  NOTES        : H/L Syntax: (override-next-method <args>)
 ***********************************************************************/
void OverrideNextMethod(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   returnValue->lexemeValue = FalseSymbol(theEnv);
   if (EvaluationData(theEnv)->HaltExecution)
     return;
   if (DefgenericData(theEnv)->CurrentMethod == NULL)
     {
      PrintErrorID(theEnv,"GENRCEXE",2,false);
      WriteString(theEnv,STDERR,"Shadowed methods not applicable in current context.\n");
      SetEvaluationError(theEnv,true);
      return;
     }
   GenericDispatch(theEnv,DefgenericData(theEnv)->CurrentGeneric,DefgenericData(theEnv)->CurrentMethod,NULL,
                   GetFirstArgument(),returnValue);
  }

/***********************************************************
  NAME         : GetGenericCurrentArgument
  DESCRIPTION  : Returns the value of the generic function
                   argument being tested in the method
                   applicability determination process
  INPUTS       : A data-object buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Data-object set
  NOTES        : Useful for queries in wildcard restrictions
 ***********************************************************/
void GetGenericCurrentArgument(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   returnValue->value = DefgenericData(theEnv)->GenericCurrentArgument->value;
   returnValue->begin = DefgenericData(theEnv)->GenericCurrentArgument->begin;
   returnValue->range = DefgenericData(theEnv)->GenericCurrentArgument->range;
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/************************************************************
  NAME         : FindApplicableMethod
  DESCRIPTION  : Finds the first/next applicable
                   method for a generic function call
  INPUTS       : 1) The generic function pointer
                 2) The address of the current method
                    (NULL to find the first)
  RETURNS      : The address of the first/next
                   applicable method (NULL on errors)
  SIDE EFFECTS : Any from evaluating query restrictions
                 Methoid busy count incremented if applicable
  NOTES        : None
 ************************************************************/
static Defmethod *FindApplicableMethod(
  Environment *theEnv,
  Defgeneric *gfunc,
  Defmethod *meth)
  {
   if (meth != NULL)
     meth++;
   else
     meth = gfunc->methods;
   for ( ; meth < &gfunc->methods[gfunc->mcnt] ; meth++)
     {
      meth->busy++;
      if (IsMethodApplicable(theEnv,meth))
        return(meth);
      meth->busy--;
     }
   return NULL;
  }

#if DEBUGGING_FUNCTIONS

/**********************************************************************
  NAME         : WatchGeneric
  DESCRIPTION  : Prints out a trace of the beginning or end
                   of the execution of a generic function
  INPUTS       : A string to indicate beginning or end of execution
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : Uses the globals CurrentGeneric, ProcParamArraySize and
                   ProcParamArray for other trace info
 **********************************************************************/
static void WatchGeneric(
  Environment *theEnv,
  const char *tstring)
  {
   if (ConstructData(theEnv)->ClearReadyInProgress ||
       ConstructData(theEnv)->ClearInProgress)
     { return; }

   WriteString(theEnv,STDOUT,"GNC ");
   WriteString(theEnv,STDOUT,tstring);
   WriteString(theEnv,STDOUT," ");
   if (DefgenericData(theEnv)->CurrentGeneric->header.whichModule->theModule != GetCurrentModule(theEnv))
     {
      WriteString(theEnv,STDOUT,DefgenericModule(DefgenericData(theEnv)->CurrentGeneric));
      WriteString(theEnv,STDOUT,"::");
     }
   WriteString(theEnv,STDOUT,DefgenericData(theEnv)->CurrentGeneric->header.name->contents);
   WriteString(theEnv,STDOUT," ");
   WriteString(theEnv,STDOUT," ED:");
   WriteInteger(theEnv,STDOUT,EvaluationData(theEnv)->CurrentEvaluationDepth);
   PrintProcParamArray(theEnv,STDOUT);
  }

/**********************************************************************
  NAME         : WatchMethod
  DESCRIPTION  : Prints out a trace of the beginning or end
                   of the execution of a generic function
                   method
  INPUTS       : A string to indicate beginning or end of execution
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : Uses the globals CurrentGeneric, CurrentMethod,
                   ProcParamArraySize and ProcParamArray for
                   other trace info
 **********************************************************************/
static void WatchMethod(
  Environment *theEnv,
  const char *tstring)
  {
   if (ConstructData(theEnv)->ClearReadyInProgress ||
       ConstructData(theEnv)->ClearInProgress)
     { return; }

   WriteString(theEnv,STDOUT,"MTH ");
   WriteString(theEnv,STDOUT,tstring);
   WriteString(theEnv,STDOUT," ");
   if (DefgenericData(theEnv)->CurrentGeneric->header.whichModule->theModule != GetCurrentModule(theEnv))
     {
      WriteString(theEnv,STDOUT,DefgenericModule(DefgenericData(theEnv)->CurrentGeneric));
      WriteString(theEnv,STDOUT,"::");
     }
   WriteString(theEnv,STDOUT,DefgenericData(theEnv)->CurrentGeneric->header.name->contents);
   WriteString(theEnv,STDOUT,":#");
   if (DefgenericData(theEnv)->CurrentMethod->system)
     WriteString(theEnv,STDOUT,"SYS");
   PrintUnsignedInteger(theEnv,STDOUT,DefgenericData(theEnv)->CurrentMethod->index);
   WriteString(theEnv,STDOUT," ");
   WriteString(theEnv,STDOUT," ED:");
   WriteInteger(theEnv,STDOUT,EvaluationData(theEnv)->CurrentEvaluationDepth);
   PrintProcParamArray(theEnv,STDOUT);
  }

#endif

#if OBJECT_SYSTEM

/***************************************************
  NAME         : DetermineRestrictionClass
  DESCRIPTION  : Finds the class of an argument in
                   the ProcParamArray
  INPUTS       : The argument data object
  RETURNS      : The class address, NULL if error
  SIDE EFFECTS : EvaluationError set on errors
  NOTES        : None
 ***************************************************/
static Defclass *DetermineRestrictionClass(
  Environment *theEnv,
  UDFValue *dobj)
  {
   Instance *ins;
   Defclass *cls;

   if (dobj->header->type == INSTANCE_NAME_TYPE)
     {
      ins = FindInstanceBySymbol(theEnv,dobj->lexemeValue);
      cls = (ins != NULL) ? ins->cls : NULL;
     }
   else if (dobj->header->type == INSTANCE_ADDRESS_TYPE)
     {
      ins = dobj->instanceValue;
      cls = (ins->garbage == 0) ? ins->cls : NULL;
     }
   else
     return(DefclassData(theEnv)->PrimitiveClassMap[dobj->header->type]);
   if (cls == NULL)
     {
      SetEvaluationError(theEnv,true);
      PrintErrorID(theEnv,"GENRCEXE",3,false);
      WriteString(theEnv,STDERR,"Unable to determine class of ");
      WriteUDFValue(theEnv,STDERR,dobj);
      WriteString(theEnv,STDERR," in generic function '");
      WriteString(theEnv,STDERR,DefgenericName(DefgenericData(theEnv)->CurrentGeneric));
      WriteString(theEnv,STDERR,"'.\n");
     }
   return(cls);
  }

#endif

#endif

