   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  10/24/17             */
   /*                                                     */
   /*               EXTERNAL FUNCTION MODULE              */
   /*******************************************************/

/*************************************************************/
/* Purpose: Routines for adding new user or system defined   */
/*   functions.                                              */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Corrected code to remove run-time program      */
/*            compiler warning.                              */
/*                                                           */
/*      6.30: Added support for passing context information  */
/*            to user defined functions.                     */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*      6.40: Changed restrictions from char * to            */
/*            CLIPSLexeme * to support strings               */
/*            originating from sources that are not          */
/*            statically allocated.                          */
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
/*            Callbacks must be environment aware.           */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Eval support for run time and bload only.      */
/*                                                           */
/*************************************************************/

#include "setup.h"

#include <ctype.h>
#include <stdlib.h>

#include "argacces.h"
#include "constant.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "factmngr.h"
#include "memalloc.h"
#include "router.h"

#if OBJECT_SYSTEM
#include "inscom.h"
#endif

#include "extnfunc.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    AddHashFunction(Environment *,struct functionDefinition *);
   static void                    InitializeFunctionHashTable(Environment *);
   static void                    DeallocateExternalFunctionData(Environment *);
#if (! RUN_TIME)
   static bool                    RemoveHashFunction(Environment *,struct functionDefinition *);
   static AddUDFError             DefineFunction(Environment *,const char *,unsigned,void (*)(Environment *,UDFContext *,UDFValue *),
                                                 const char *,unsigned short,unsigned short,const char *,void *);
#endif
   static void                    PrintType(Environment *,const char *,int,int *,const char *);
   static void                    AssignErrorValue(UDFContext *);

/*********************************************************/
/* InitializeExternalFunctionData: Allocates environment */
/*    data for external functions.                       */
/*********************************************************/
void InitializeExternalFunctionData(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,EXTERNAL_FUNCTION_DATA,sizeof(struct externalFunctionData),DeallocateExternalFunctionData);
  }

/***********************************************************/
/* DeallocateExternalFunctionData: Deallocates environment */
/*    data for external functions.                         */
/***********************************************************/
static void DeallocateExternalFunctionData(
  Environment *theEnv)
  {
   struct FunctionHash *fhPtr, *nextFHPtr;
   int i;

#if ! RUN_TIME
   struct functionDefinition *tmpPtr, *nextPtr;

   tmpPtr = ExternalFunctionData(theEnv)->ListOfFunctions;
   while (tmpPtr != NULL)
     {
      nextPtr = tmpPtr->next;
      rtn_struct(theEnv,functionDefinition,tmpPtr);
      tmpPtr = nextPtr;
     }
#endif

   if (ExternalFunctionData(theEnv)->FunctionHashtable == NULL)
     { return; }

   for (i = 0; i < SIZE_FUNCTION_HASH; i++)
     {
      fhPtr = ExternalFunctionData(theEnv)->FunctionHashtable[i];
      while (fhPtr != NULL)
        {
         nextFHPtr = fhPtr->next;
         rtn_struct(theEnv,FunctionHash,fhPtr);
         fhPtr = nextFHPtr;
        }
     }

   genfree(theEnv,ExternalFunctionData(theEnv)->FunctionHashtable,
           sizeof (struct FunctionHash *) * SIZE_FUNCTION_HASH);
  }

#if (! RUN_TIME)

/****************************************************/
/* AddUDF: Used to define a system or user external */
/*   function so that the KB can access it.         */
/****************************************************/
AddUDFError AddUDF(
  Environment *theEnv,
  const char *clipsFunctionName,
  const char *returnTypes,
  unsigned short minArgs,
  unsigned short maxArgs,
  const char *argumentTypes,
  UserDefinedFunction *cFunctionPointer,
  const char *cFunctionName,
  void *context)
  {
   unsigned returnTypeBits;
   size_t i;

   if ((minArgs != UNBOUNDED) && (minArgs > maxArgs))
     { return AUE_MIN_EXCEEDS_MAX_ERROR; }

   if (argumentTypes != NULL)
     {
      for (i = 0; argumentTypes[i] != EOS; i++)
        {
         if (strchr("bdefilmnsyv*;",argumentTypes[i]) == NULL)
           { return AUE_INVALID_ARGUMENT_TYPE_ERROR; }
        }
     }
     
   if (returnTypes != NULL)
     {
      for (i = 0; returnTypes[i] != EOS; i++)
        {
         if (strchr("bdefilmnsyv*;",returnTypes[i]) == NULL)
           { return AUE_INVALID_RETURN_TYPE_ERROR; }
        }

      PopulateRestriction(theEnv,&returnTypeBits,ANY_TYPE_BITS,returnTypes,0);
     }
   else
     { returnTypeBits = ANY_TYPE_BITS; }

   return DefineFunction(theEnv,clipsFunctionName,returnTypeBits,cFunctionPointer,
                         cFunctionName,minArgs,maxArgs,argumentTypes,context);
  }

/*************************************************************/
/* DefineFunction: Used to define a system or user external  */
/*   function so that the KB can access it. Allows argument  */
/*   restrictions to be attached to the function.            */
/*************************************************************/
static AddUDFError DefineFunction(
  Environment *theEnv,
  const char *name,
  unsigned returnTypeBits,
  void (*pointer)(Environment *,UDFContext *,UDFValue *),
  const char *actualName,
  unsigned short minArgs,
  unsigned short maxArgs,
  const char *restrictions,
  void *context)
  {
   struct functionDefinition *newFunction;

   newFunction = FindFunction(theEnv,name);
   if (newFunction != NULL)
     { return AUE_FUNCTION_NAME_IN_USE_ERROR; }

   newFunction = get_struct(theEnv,functionDefinition);
   newFunction->callFunctionName = CreateSymbol(theEnv,name);
   IncrementLexemeCount(newFunction->callFunctionName);
   newFunction->next = GetFunctionList(theEnv);
   ExternalFunctionData(theEnv)->ListOfFunctions = newFunction;
   AddHashFunction(theEnv,newFunction);

   newFunction->unknownReturnValueType = returnTypeBits;
   newFunction->functionPointer = pointer;
   newFunction->actualFunctionName = actualName;

   newFunction->minArgs = minArgs;
   newFunction->maxArgs = maxArgs;

   if (restrictions == NULL)
     { newFunction->restrictions = NULL; }
   else
     {
      newFunction->restrictions = CreateString(theEnv,restrictions);
      IncrementLexemeCount(newFunction->restrictions);
     }

   newFunction->parser = NULL;
   newFunction->overloadable = true;
   newFunction->sequenceuseok = true;
   newFunction->usrData = NULL;
   newFunction->context = context;

   return AUE_NO_ERROR;
  }

/********************************************/
/* RemoveUDF: Used to remove a function     */
/*   definition from the list of functions. */
/********************************************/
bool RemoveUDF(
  Environment *theEnv,
  const char *functionName)
  {
   CLIPSLexeme *findValue;
   struct functionDefinition *fPtr, *lastPtr = NULL;

   findValue = FindSymbolHN(theEnv,functionName,SYMBOL_BIT);

   for (fPtr = ExternalFunctionData(theEnv)->ListOfFunctions;
        fPtr != NULL;
        fPtr = fPtr->next)
     {
      if (fPtr->callFunctionName == findValue)
        {
         ReleaseLexeme(theEnv,fPtr->callFunctionName);
         RemoveHashFunction(theEnv,fPtr);

         if (lastPtr == NULL)
           { ExternalFunctionData(theEnv)->ListOfFunctions = fPtr->next; }
         else
           { lastPtr->next = fPtr->next; }

         if (fPtr->restrictions != NULL)
           { ReleaseLexeme(theEnv,fPtr->restrictions); }
         ClearUserDataList(theEnv,fPtr->usrData);
         rtn_struct(theEnv,functionDefinition,fPtr);
         return true;
        }

      lastPtr = fPtr;
     }

   return false;
  }

/******************************************/
/* RemoveHashFunction: Removes a function */
/*   from the function hash table.        */
/******************************************/
static bool RemoveHashFunction(
  Environment *theEnv,
  struct functionDefinition *fdPtr)
  {
   struct FunctionHash *fhPtr, *lastPtr = NULL;
   size_t hashValue;

   hashValue = HashSymbol(fdPtr->callFunctionName->contents,SIZE_FUNCTION_HASH);

   for (fhPtr = ExternalFunctionData(theEnv)->FunctionHashtable[hashValue];
        fhPtr != NULL;
        fhPtr = fhPtr->next)
     {
      if (fhPtr->fdPtr == fdPtr)
        {
         if (lastPtr == NULL)
           { ExternalFunctionData(theEnv)->FunctionHashtable[hashValue] = fhPtr->next; }
         else
           { lastPtr->next = fhPtr->next; }

         rtn_struct(theEnv,FunctionHash,fhPtr);
         return true;
        }

      lastPtr = fhPtr;
     }

   return false;
  }

#endif

/***************************************************************************/
/* AddFunctionParser: Associates a specialized expression parsing function */
/*   with the function entry for a function which was defined using        */
/*   DefineFunction. When this function is parsed, the specialized parsing */
/*   function will be called to parse the arguments of the function. Only  */
/*   user and system defined functions can have specialized parsing        */
/*   routines. Generic functions and deffunctions can not have specialized */
/*   parsing routines.                                                     */
/***************************************************************************/
bool AddFunctionParser(
  Environment *theEnv,
  const char *functionName,
  struct expr *(*fpPtr)(Environment *,struct expr *,const char *))
  {
   struct functionDefinition *fdPtr;

   fdPtr = FindFunction(theEnv,functionName);
   if (fdPtr == NULL)
     {
      WriteString(theEnv,STDERR,"Function parsers can only be added for existing functions.\n");
      return false;
     }

   fdPtr->parser = fpPtr;
   fdPtr->overloadable = false;

   return true;
  }

#if (! RUN_TIME)

/*********************************************************************/
/* RemoveFunctionParser: Removes a specialized expression parsing    */
/*   function (if it exists) from the function entry for a function. */
/*********************************************************************/
bool RemoveFunctionParser(
  Environment *theEnv,
  const char *functionName)
  {
   struct functionDefinition *fdPtr;

   fdPtr = FindFunction(theEnv,functionName);
   if (fdPtr == NULL)
     {
      WriteString(theEnv,STDERR,"Function parsers can only be removed from existing functions.\n");
      return false;
     }

   fdPtr->parser = NULL;

   return true;
  }

/*****************************************************************/
/* FuncSeqOvlFlags: Makes a system function overloadable or not, */
/* i.e. can the function be a method for a generic function.     */
/*****************************************************************/
bool FuncSeqOvlFlags(
  Environment *theEnv,
  const char *functionName,
  bool seqp,
  bool ovlp)
  {
   struct functionDefinition *fdPtr;

   fdPtr = FindFunction(theEnv,functionName);
   if (fdPtr == NULL)
     {
      WriteString(theEnv,STDERR,"Only existing functions can be marked as using sequence expansion arguments/overloadable or not.\n");
      return false;
     }
     
   fdPtr->sequenceuseok = (seqp ? true : false);
   fdPtr->overloadable = (ovlp ? true : false);
   
   return true;
  }

#endif

/***********************************************/
/* GetNthRestriction: Returns the restriction  */
/*   type for the nth parameter of a function. */
/***********************************************/
unsigned GetNthRestriction(
  Environment *theEnv,
  struct functionDefinition *theFunction,
  unsigned int position)
  {
   unsigned rv, df;
   const char *restrictions;

   if (theFunction == NULL) return(ANY_TYPE_BITS);

   if (theFunction->restrictions == NULL) return(ANY_TYPE_BITS);
   restrictions = theFunction->restrictions->contents;

   PopulateRestriction(theEnv,&df,ANY_TYPE_BITS,restrictions,0);
   PopulateRestriction(theEnv,&rv,df,restrictions,position);

   return rv;
  }

/*************************************************/
/* GetFunctionList: Returns the ListOfFunctions. */
/*************************************************/
struct functionDefinition *GetFunctionList(
  Environment *theEnv)
  {
   return(ExternalFunctionData(theEnv)->ListOfFunctions);
  }

/**************************************************************/
/* InstallFunctionList: Sets the ListOfFunctions and adds all */
/*   the function entries to the FunctionHashTable.           */
/**************************************************************/
void InstallFunctionList(
  Environment *theEnv,
  struct functionDefinition *value)
  {
   int i;
   struct FunctionHash *fhPtr, *nextPtr;

   if (ExternalFunctionData(theEnv)->FunctionHashtable != NULL)
     {
      for (i = 0; i < SIZE_FUNCTION_HASH; i++)
        {
         fhPtr = ExternalFunctionData(theEnv)->FunctionHashtable[i];
         while (fhPtr != NULL)
           {
            nextPtr = fhPtr->next;
            rtn_struct(theEnv,FunctionHash,fhPtr);
            fhPtr = nextPtr;
           }
         ExternalFunctionData(theEnv)->FunctionHashtable[i] = NULL;
        }
     }

   ExternalFunctionData(theEnv)->ListOfFunctions = value;

   while (value != NULL)
     {
      AddHashFunction(theEnv,value);
      value = value->next;
     }
  }

/********************************************************/
/* FindFunction: Returns a pointer to the corresponding */
/*   functionDefinition structure if a function name is */
/*   in the function list, otherwise returns NULL.      */
/********************************************************/
struct functionDefinition *FindFunction(
  Environment *theEnv,
  const char *functionName)
  {
   struct FunctionHash *fhPtr;
   size_t hashValue;
   CLIPSLexeme *findValue;

   if (ExternalFunctionData(theEnv)->FunctionHashtable == NULL) return NULL;

   hashValue = HashSymbol(functionName,SIZE_FUNCTION_HASH);

   findValue = FindSymbolHN(theEnv,functionName,SYMBOL_BIT);

   for (fhPtr = ExternalFunctionData(theEnv)->FunctionHashtable[hashValue];
        fhPtr != NULL;
        fhPtr = fhPtr->next)
     {
      if (fhPtr->fdPtr->callFunctionName == findValue)
        { return(fhPtr->fdPtr); }
     }

   return NULL;
  }

/********************************************************/
/* GetUDFContext: Returns the context associated a UDF. */
/********************************************************/
void *GetUDFContext(
  Environment *theEnv,
  const char *functionName)
  {
   struct FunctionHash *fhPtr;
   size_t hashValue;
   CLIPSLexeme *findValue;

   if (ExternalFunctionData(theEnv)->FunctionHashtable == NULL) return NULL;

   hashValue = HashSymbol(functionName,SIZE_FUNCTION_HASH);

   findValue = FindSymbolHN(theEnv,functionName,SYMBOL_BIT);

   for (fhPtr = ExternalFunctionData(theEnv)->FunctionHashtable[hashValue];
        fhPtr != NULL;
        fhPtr = fhPtr->next)
     {
      if (fhPtr->fdPtr->callFunctionName == findValue)
        { return fhPtr->fdPtr->context; }
     }

   return NULL;
  }

/*********************************************************/
/* InitializeFunctionHashTable: Purpose is to initialize */
/*   the function hash table to NULL.                    */
/*********************************************************/
static void InitializeFunctionHashTable(
  Environment *theEnv)
  {
   int i;

   ExternalFunctionData(theEnv)->FunctionHashtable = (struct FunctionHash **)
                       gm2(theEnv,sizeof (struct FunctionHash *) *
                           SIZE_FUNCTION_HASH);

   for (i = 0; i < SIZE_FUNCTION_HASH; i++) ExternalFunctionData(theEnv)->FunctionHashtable[i] = NULL;
  }

/****************************************************************/
/* AddHashFunction: Adds a function to the function hash table. */
/****************************************************************/
static void AddHashFunction(
  Environment *theEnv,
  struct functionDefinition *fdPtr)
  {
   struct FunctionHash *newhash, *temp;
   size_t hashValue;

   if (ExternalFunctionData(theEnv)->FunctionHashtable == NULL) InitializeFunctionHashTable(theEnv);

   newhash = get_struct(theEnv,FunctionHash);
   newhash->fdPtr = fdPtr;

   hashValue = HashSymbol(fdPtr->callFunctionName->contents,SIZE_FUNCTION_HASH);

   temp = ExternalFunctionData(theEnv)->FunctionHashtable[hashValue];
   ExternalFunctionData(theEnv)->FunctionHashtable[hashValue] = newhash;
   newhash->next = temp;
  }

/*************************************************/
/* GetMinimumArgs: Returns the minimum number of */
/*   arguments expected by an external function. */
/*************************************************/
int GetMinimumArgs(
  struct functionDefinition *theFunction)
  {
   return theFunction->minArgs;
  }

/*************************************************/
/* GetMaximumArgs: Returns the maximum number of */
/*   arguments expected by an external function. */
/*************************************************/
int GetMaximumArgs(
  struct functionDefinition *theFunction)
  {
   return theFunction->maxArgs;
  }

/********************/
/* AssignErrorValue */
/********************/
void AssignErrorValue(
  UDFContext *context)
  {
   if (context->theFunction->unknownReturnValueType & BOOLEAN_BIT)
     { context->returnValue->lexemeValue = context->environment->FalseSymbol; }
   else if (context->theFunction->unknownReturnValueType & STRING_BIT)
     { context->returnValue->lexemeValue = CreateString(context->environment,""); }
   else if (context->theFunction->unknownReturnValueType & SYMBOL_BIT)
     { context->returnValue->lexemeValue = CreateSymbol(context->environment,"nil"); }
   else if (context->theFunction->unknownReturnValueType & INTEGER_BIT)
     { context->returnValue->integerValue = CreateInteger(context->environment,0); }
   else if (context->theFunction->unknownReturnValueType & FLOAT_BIT)
     { context->returnValue->floatValue = CreateFloat(context->environment,0.0); }
   else if (context->theFunction->unknownReturnValueType & MULTIFIELD_BIT)
     { SetMultifieldErrorValue(context->environment,context->returnValue); }
   else if (context->theFunction->unknownReturnValueType & INSTANCE_NAME_BIT)
     { context->returnValue->lexemeValue = CreateInstanceName(context->environment,"nil"); }
   else if (context->theFunction->unknownReturnValueType & FACT_ADDRESS_BIT)
     { context->returnValue->factValue = &FactData(context->environment)->DummyFact; }
#if OBJECT_SYSTEM
   else if (context->theFunction->unknownReturnValueType & INSTANCE_ADDRESS_BIT)
     { context->returnValue->value = &InstanceData(context->environment)->DummyInstance; }
#endif
   else if (context->theFunction->unknownReturnValueType & EXTERNAL_ADDRESS_BIT)
     { context->returnValue->value = CreateExternalAddress(context->environment,NULL,0); }
   else
     { context->returnValue->value = context->environment->VoidConstant; }
  }

/*********************/
/* UDFArgumentCount: */
/*********************/
unsigned int UDFArgumentCount(
  UDFContext *context)
  {
   unsigned int count = 0;
   struct expr *argPtr;

   for (argPtr = EvaluationData(context->environment)->CurrentExpression->argList;
        argPtr != NULL;
        argPtr = argPtr->nextArg)
     { count++; }

   return count;
  }

/*********************/
/* UDFFirstArgument: */
/*********************/
bool UDFFirstArgument(
  UDFContext *context,
  unsigned expectedType,
  UDFValue *returnValue)
  {
   context->lastArg = EvaluationData(context->environment)->CurrentExpression->argList;
   context->lastPosition = 1;
   return UDFNextArgument(context,expectedType,returnValue);
  }

/********************/
/* UDFNextArgument: */
/********************/
bool UDFNextArgument(
  UDFContext *context,
  unsigned expectedType,
  UDFValue *returnValue)
  {
   struct expr *argPtr = context->lastArg;
   unsigned int argumentPosition = context->lastPosition;
   Environment *theEnv = context->environment;

   if (argPtr == NULL)
     {
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      return false;
     }

   context->lastPosition++;
   context->lastArg = context->lastArg->nextArg;

   switch (argPtr->type)
     {
      case INTEGER_TYPE:
        returnValue->value = argPtr->value;
        if (expectedType & INTEGER_BIT) return true;
        ExpectedTypeError0(theEnv,UDFContextFunctionName(context),argumentPosition);
        PrintTypesString(theEnv,STDERR,expectedType,true);
        SetHaltExecution(theEnv,true);
        SetEvaluationError(theEnv,true);
        AssignErrorValue(context);
        return false;
        break;

      case FLOAT_TYPE:
        returnValue->value = argPtr->value;
        if (expectedType & FLOAT_BIT) return true;
        ExpectedTypeError0(theEnv,UDFContextFunctionName(context),argumentPosition);
        PrintTypesString(theEnv,STDERR,expectedType,true);
        SetHaltExecution(theEnv,true);
        SetEvaluationError(theEnv,true);
        AssignErrorValue(context);
        return false;
        break;

      case SYMBOL_TYPE:
        returnValue->value = argPtr->value;
        if (expectedType & SYMBOL_BIT) return true;
        if (expectedType & BOOLEAN_BIT)
          {
           if ((returnValue->lexemeValue == FalseSymbol(theEnv)) ||
               (returnValue->lexemeValue == TrueSymbol(theEnv)))
             { return true; }
          }
        ExpectedTypeError0(theEnv,UDFContextFunctionName(context),argumentPosition);
        PrintTypesString(theEnv,STDERR,expectedType,true);
        SetHaltExecution(theEnv,true);
        SetEvaluationError(theEnv,true);
        AssignErrorValue(context);
        return false;
        break;

      case STRING_TYPE:
        returnValue->value = argPtr->value;
        if (expectedType & STRING_BIT) return true;
        ExpectedTypeError0(theEnv,UDFContextFunctionName(context),argumentPosition);
        PrintTypesString(theEnv,STDERR,expectedType,true);
        SetHaltExecution(theEnv,true);
        SetEvaluationError(theEnv,true);
        AssignErrorValue(context);
        return false;
        break;

      case INSTANCE_NAME_TYPE:
        returnValue->value = argPtr->value;
        if (expectedType & INSTANCE_NAME_BIT) return true;
        ExpectedTypeError0(theEnv,UDFContextFunctionName(context),argumentPosition);
        PrintTypesString(theEnv,STDERR,expectedType,true);
        SetHaltExecution(theEnv,true);
        SetEvaluationError(theEnv,true);
        AssignErrorValue(context);
        return false;
        break;
     }

   EvaluateExpression(theEnv,argPtr,returnValue);

   switch (returnValue->header->type)
     {
      case VOID_TYPE:
        if (expectedType & VOID_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case INTEGER_TYPE:
        if (expectedType & INTEGER_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case FLOAT_TYPE:
        if (expectedType & FLOAT_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case SYMBOL_TYPE:
        if (expectedType & SYMBOL_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
          
        if (expectedType & BOOLEAN_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else if ((returnValue->lexemeValue == FalseSymbol(theEnv)) ||
                    (returnValue->lexemeValue == TrueSymbol(theEnv)))
             { return true; }
          }

        break;

      case STRING_TYPE:
        if (expectedType & STRING_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case INSTANCE_NAME_TYPE:
        if (expectedType & INSTANCE_NAME_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case EXTERNAL_ADDRESS_TYPE:
        if (expectedType & EXTERNAL_ADDRESS_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case FACT_ADDRESS_TYPE:
        if (expectedType & FACT_ADDRESS_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case INSTANCE_ADDRESS_TYPE:
        if (expectedType & INSTANCE_ADDRESS_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;

      case MULTIFIELD_TYPE:
        if (expectedType & MULTIFIELD_BIT)
          {
           if (EvaluationData(theEnv)->EvaluationError)
             {
              AssignErrorValue(context);
              return false;
             }
           else return true;
          }
        break;
     }

   ExpectedTypeError0(theEnv,UDFContextFunctionName(context),argumentPosition);
   PrintTypesString(theEnv,STDERR,expectedType,true);

   SetHaltExecution(theEnv,true);
   SetEvaluationError(theEnv,true);
   AssignErrorValue(context);

   return false;
  }

/*******************/
/* UDFNthArgument: */
/*******************/
bool UDFNthArgument(
  UDFContext *context,
  unsigned int argumentPosition,
  unsigned expectedType,
  UDFValue *returnValue)
  {
   if (argumentPosition < context->lastPosition)
     {
      context->lastArg = EvaluationData(context->environment)->CurrentExpression->argList;
      context->lastPosition = 1;
     }

   for ( ; (context->lastArg != NULL) && (context->lastPosition < argumentPosition) ;
           context->lastArg = context->lastArg->nextArg)
     { context->lastPosition++; }

   return UDFNextArgument(context,expectedType,returnValue);
  }

/******************************/
/* UDFInvalidArgumentMessage: */
/******************************/
void UDFInvalidArgumentMessage(
  UDFContext *context,
  const char *typeString)
  {
   ExpectedTypeError1(context->environment,
                      UDFContextFunctionName(context),
                      context->lastPosition-1,typeString);
  }

/******************/
/* UDFThrowError: */
/******************/
void UDFThrowError(
  UDFContext *context)
  {
   Environment *theEnv = context->environment;

   SetHaltExecution(theEnv,true);
   SetEvaluationError(theEnv,true);
  }

/***************************/
/* UDFContextFunctionName: */
/***************************/
const char *UDFContextFunctionName(
  UDFContext *context)
  {
   return context->theFunction->callFunctionName->contents;
  }

/**************/
/* PrintType: */
/**************/
static void PrintType(
  Environment *theEnv,
  const char *logicalName,
  int typeCount,
  int *typesPrinted,
  const char *typeName)
  {
   if (*typesPrinted == 0)
     {
      WriteString(theEnv,logicalName,typeName);
      (*typesPrinted)++;
      return;
     }

   if (typeCount == 2)
     { WriteString(theEnv,logicalName," or "); }
   else if (((*typesPrinted) + 1) == typeCount)
     { WriteString(theEnv,logicalName,", or "); }
   else
     { WriteString(theEnv,logicalName,", "); }

   WriteString(theEnv,logicalName,typeName);
   (*typesPrinted)++;
  }

/********************/
/* PrintTypesString */
/********************/
void PrintTypesString(
  Environment *theEnv,
  const char *logicalName,
  unsigned expectedType,
  bool printCRLF)
  {
   int typeCount, typesPrinted;

   typeCount = 0;
   if (expectedType & INTEGER_BIT) typeCount++;
   if (expectedType & FLOAT_BIT) typeCount++;
   if (expectedType & BOOLEAN_BIT) typeCount++;
   if (expectedType & SYMBOL_BIT) typeCount++;
   if (expectedType & STRING_BIT) typeCount++;
   if (expectedType & INSTANCE_NAME_BIT) typeCount++;
   if (expectedType & INSTANCE_ADDRESS_BIT) typeCount++;
   if (expectedType & FACT_ADDRESS_BIT) typeCount++;
   if (expectedType & EXTERNAL_ADDRESS_BIT) typeCount++;
   if (expectedType & MULTIFIELD_BIT) typeCount++;

   typesPrinted = 0;
   if (expectedType & INTEGER_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"integer"); }

    if (expectedType & FLOAT_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"float"); }

   if (expectedType & BOOLEAN_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"boolean"); }

   if (expectedType & SYMBOL_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"symbol"); }

   if (expectedType & STRING_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"string"); }

   if (expectedType & INSTANCE_NAME_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"instance name"); }

   if (expectedType & INSTANCE_ADDRESS_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"instance address"); }

   if (expectedType & FACT_ADDRESS_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"fact address"); }

   if (expectedType & EXTERNAL_ADDRESS_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"external address"); }

   if (expectedType & MULTIFIELD_BIT)
     { PrintType(theEnv,logicalName,typeCount,&typesPrinted,"multifield"); }

   if (printCRLF)
     { WriteString(theEnv,logicalName,".\n"); }
  }
