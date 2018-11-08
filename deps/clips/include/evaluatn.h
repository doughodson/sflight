   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  11/17/17            */
   /*                                                     */
   /*               EVALUATION HEADER FILE                */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines for evaluating expressions.    */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Added EvaluateAndStoreInDataObject function.   */
/*                                                           */
/*      6.30: Added support for passing context information  */
/*            to user defined functions.                     */
/*                                                           */
/*            Added support for external address hash table  */
/*            and subtyping.                                 */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Support for DATA_OBJECT_ARRAY primitive.       */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*      6.40: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*            Added Env prefix to GetHaltExecution and       */
/*            SetHaltExecution functions.                    */
/*                                                           */
/*            Removed LOCALE definition.                     */
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
/*            UDF redesign.                                  */
/*                                                           */
/*            Removed DATA_OBJECT_ARRAY primitive type.      */
/*                                                           */
/*************************************************************/

#ifndef _H_evaluatn

#pragma once

#define _H_evaluatn

#include "constant.h"
#include "entities.h"

typedef struct functionCallBuilder FunctionCallBuilder;

struct functionCallBuilder
  {
   Environment *fcbEnv;
   CLIPSValue *contents;
   size_t bufferReset;
   size_t length;
   size_t bufferMaximum;
  };

typedef enum
  {
   FCBE_NO_ERROR = 0,
   FCBE_NULL_POINTER_ERROR,
   FCBE_FUNCTION_NOT_FOUND_ERROR,
   FCBE_INVALID_FUNCTION_ERROR,
   FCBE_ARGUMENT_COUNT_ERROR,
   FCBE_ARGUMENT_TYPE_ERROR,
   FCBE_PROCESSING_ERROR
  } FunctionCallBuilderError;

#define PARAMETERS_UNBOUNDED USHRT_MAX

#define C_POINTER_EXTERNAL_ADDRESS 0

struct externalAddressType
  {
   const  char *name;
   void (*shortPrintFunction)(Environment *,const char *,void *);
   void (*longPrintFunction)(Environment *,const char *,void *);
   bool (*discardFunction)(Environment *,void *);
   void (*newFunction)(UDFContext *,UDFValue *);
   bool (*callFunction)(UDFContext *,UDFValue *,UDFValue *);
  };

#define CoerceToLongInteger(t,v) ((t == INTEGER_TYPE) ? ValueToLong(v) : (long) ValueToDouble(v))
#define CoerceToInteger(t,v) ((t == INTEGER_TYPE) ? (int) ValueToLong(v) : (int) ValueToDouble(v))
#define CoerceToDouble(t,v) ((t == INTEGER_TYPE) ? (double) ValueToLong(v) : ValueToDouble(v))

#define GetFirstArgument()           (EvaluationData(theEnv)->CurrentExpression->argList)
#define GetNextArgument(ep)          (ep->nextArg)

#define MAXIMUM_PRIMITIVES 150
#define MAXIMUM_EXTERNAL_ADDRESS_TYPES 10

#define BITS_PER_BYTE    8

#define BitwiseTest(n,b)   (((n) & (char) (1 << (b))) ? true : false)
#define BitwiseSet(n,b)    (n |= (char) (1 << (b)))
#define BitwiseClear(n,b)  (n &= (char) ~(1 << (b)))

#define CountToBitMapSize(c) (((c) + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE) 
#define TestBitMap(map,id)  BitwiseTest(map[(id) / BITS_PER_BYTE],(id) % BITS_PER_BYTE)
#define SetBitMap(map,id)   BitwiseSet(map[(id) / BITS_PER_BYTE],(id) % BITS_PER_BYTE)
#define ClearBitMap(map,id) BitwiseClear(map[(id) / BITS_PER_BYTE],(id) % BITS_PER_BYTE)

#define EVALUATION_DATA 44

struct evaluationData
  {
   struct expr *CurrentExpression;
   bool EvaluationError;
   bool HaltExecution;
   int CurrentEvaluationDepth;
   int numberOfAddressTypes;
   struct entityRecord *PrimitivesArray[MAXIMUM_PRIMITIVES];
   struct externalAddressType *ExternalAddressTypes[MAXIMUM_EXTERNAL_ADDRESS_TYPES];
  };

#define EvaluationData(theEnv) ((struct evaluationData *) GetEnvironmentData(theEnv,EVALUATION_DATA))

   void                           InitializeEvaluationData(Environment *);
   bool                           EvaluateExpression(Environment *,struct expr *,UDFValue *);
   void                           SetEvaluationError(Environment *,bool);
   bool                           GetEvaluationError(Environment *);
   void                           SetHaltExecution(Environment *,bool);
   bool                           GetHaltExecution(Environment *);
   void                           ReturnValues(Environment *,UDFValue *,bool);
   void                           WriteUDFValue(Environment *,const char *,UDFValue *);
   void                           WriteCLIPSValue(Environment *,const char *,CLIPSValue *);
   void                           SetMultifieldErrorValue(Environment *,UDFValue *);
   void                           CopyDataObject(Environment *,UDFValue *,UDFValue *,int);
   void                           AtomInstall(Environment *,unsigned short,void *);
   void                           AtomDeinstall(Environment *,unsigned short,void *);
   void                           Retain(Environment *,TypeHeader *);
   void                           Release(Environment *,TypeHeader *);
   void                           RetainCV(Environment *,CLIPSValue *);
   void                           ReleaseCV(Environment *,CLIPSValue *);
   void                           RetainUDFV(Environment *,UDFValue *);
   void                           ReleaseUDFV(Environment *,UDFValue *);
   struct expr                   *ConvertValueToExpression(Environment *,UDFValue *);
   unsigned long                  GetAtomicHashValue(unsigned short,void *,unsigned short);
   void                           InstallPrimitive(Environment *,struct entityRecord *,int);
   int                            InstallExternalAddressType(Environment *,struct externalAddressType *);
   void                           TransferDataObjectValues(UDFValue *,UDFValue *);
   struct expr                   *FunctionReferenceExpression(Environment *,const char *);
   bool                           GetFunctionReference(Environment *,const char *,Expression *);
   bool                           DOsEqual(UDFValue *,UDFValue *);
   bool                           EvaluateAndStoreInDataObject(Environment *,bool,Expression *,UDFValue *,bool);
   void                           ResetErrorFlags(Environment *);
   FunctionCallBuilder           *CreateFunctionCallBuilder(Environment *,size_t);
   void                           FCBAppendUDFValue(FunctionCallBuilder *,UDFValue *);
   void                           FCBAppend(FunctionCallBuilder *,CLIPSValue *);
   void                           FCBAppendCLIPSInteger(FunctionCallBuilder *,CLIPSInteger *);
   void                           FCBAppendInteger(FunctionCallBuilder *,long long);
   void                           FCBAppendCLIPSFloat(FunctionCallBuilder *,CLIPSFloat *);
   void                           FCBAppendFloat(FunctionCallBuilder *,double);
   void                           FCBAppendCLIPSLexeme(FunctionCallBuilder *,CLIPSLexeme *);
   void                           FCBAppendSymbol(FunctionCallBuilder *,const char *);
   void                           FCBAppendString(FunctionCallBuilder *,const char *);
   void                           FCBAppendInstanceName(FunctionCallBuilder *,const char *);
   void                           FCBAppendCLIPSExternalAddress(FunctionCallBuilder *,CLIPSExternalAddress *);
   void                           FCBAppendFact(FunctionCallBuilder *,Fact *);
   void                           FCBAppendInstance(FunctionCallBuilder *,Instance *);
   void                           FCBAppendMultifield(FunctionCallBuilder *,Multifield *);
   void                           FCBDispose(FunctionCallBuilder *);
   void                           FCBReset(FunctionCallBuilder *);
   FunctionCallBuilderError       FCBCall(FunctionCallBuilder *,const char *,CLIPSValue *);
   
#define CVIsType(cv,cvType) (((1 << (((TypeHeader *) (cv)->value)->type)) & (cvType)) ? true : false)

#define ValueIsType(value,vType) ((1 << (((TypeHeader *) value)->type)) & (vType))

#define CVCoerceToFloat(cv) (((cv)->header->type == FLOAT_TYPE) ? \
                             ((cv)->floatValue->contents) : \
                             ((double) (cv)->integerValue->contents))

#define CVCoerceToInteger(cv) (((cv)->header->type == INTEGER_TYPE) ? \
                               ((cv)->integerValue->contents) : \
                               ((long long) (cv)->floatValue->contents))

#endif /* _H_evaluatn */
