   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/09/18             */
   /*                                                     */
   /*             MULTIFIELD FUNCTIONS MODULE             */
   /*******************************************************/

/*************************************************************/
/* Purpose: Contains the code for several multifield         */
/*   functions including first$, rest$, subseq$, delete$,    */
/*   delete-member$, replace-member$, replace$, insert$,     */
/*   explode$, implode$, nth$, member$, subsetp and progn$.  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*      Brian Dantes                                         */
/*      Barry Cameron                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*            Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Moved ImplodeMultifield to multifld.c.         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Fixed memory leaks when error occurred.        */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Fixed linkage issue when DEFMODULE_CONSTRUCT   */
/*            compiler flag is set to 0.                     */
/*                                                           */
/*      6.40: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*            Added Env prefix to GetHaltExecution and       */
/*            SetHaltExecution functions.                    */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            Removed member, mv-replace, mv-subseq,         */
/*            mv-delete, str-implode, str-explode, subset,   */
/*            and nth functions.                             */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Added GCBlockStart and GCBlockEnd functions    */
/*            for garbage collection blocks.                 */
/*                                                           */
/*            Eval support for run time and bload only.      */
/*                                                           */
/*            The explode$ function via StringToMultifield   */
/*            now converts non-primitive value tokens        */
/*            (such as parentheses) to symbols rather than   */
/*            strings.                                       */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if MULTIFIELD_FUNCTIONS || OBJECT_SYSTEM

#include <stdio.h>
#include <string.h>

#include "argacces.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "memalloc.h"
#include "multifld.h"
#include "multifun.h"
#if OBJECT_SYSTEM
#include "object.h"
#endif
#include "pprint.h"
#include "prcdrpsr.h"
#include "prcdrfun.h"
#include "prntutil.h"
#include "router.h"
#if (! BLOAD_ONLY) && (! RUN_TIME)
#include "scanner.h"
#endif
#include "utility.h"

/**************/
/* STRUCTURES */
/**************/

typedef struct fieldVarStack
  {
   unsigned short type;
   void *value;
   size_t index;
   struct fieldVarStack *nxt;
  } FIELD_VAR_STACK;

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if MULTIFIELD_FUNCTIONS
   static bool                    MVRangeCheck(size_t,size_t,size_t *,unsigned int);
   static void                    MultifieldPrognDriver(UDFContext *,UDFValue *,const char *);
#if (! BLOAD_ONLY)
   static struct expr            *MultifieldPrognParser(Environment *,struct expr *,const char *);
   static struct expr            *ForeachParser(Environment *,struct expr *,const char *);
   static void                    ReplaceMvPrognFieldVars(Environment *,CLIPSLexeme *,struct expr *,int);
#endif /* (! BLOAD_ONLY) && (! RUN_TIME) */
#endif /* MULTIFIELD_FUNCTIONS */
   static void                    MVRangeErrorSizet(Environment *,size_t,size_t,size_t,const char *);
#endif /* MULTIFIELD_FUNCTIONS || OBJECT_SYSTEM */

/***************************************/
/* LOCAL INTERNAL VARIABLE DEFINITIONS */
/***************************************/

#if MULTIFIELD_FUNCTIONS

#define MULTIFUN_DATA 10

struct multiFunctionData
  {
   FIELD_VAR_STACK *FieldVarStack;
  };

#define MultiFunctionData(theEnv) ((struct multiFunctionData *) GetEnvironmentData(theEnv,MULTIFUN_DATA))

/**********************************************/
/* MultifieldFunctionDefinitions: Initializes */
/*   the multifield functions.                */
/**********************************************/
void MultifieldFunctionDefinitions(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,MULTIFUN_DATA,sizeof(struct multiFunctionData),NULL);

#if ! RUN_TIME
   AddUDF(theEnv,"first$","m",1,1,"m",FirstFunction,"FirstFunction",NULL);
   AddUDF(theEnv,"rest$","m",1,1,"m",RestFunction,"RestFunction",NULL);
   AddUDF(theEnv,"subseq$","m",3,3,"l;m",SubseqFunction,"SubseqFunction",NULL);
   AddUDF(theEnv,"delete-member$","m",2,UNBOUNDED,"*;m",DeleteMemberFunction,"DeleteMemberFunction",NULL);
   AddUDF(theEnv,"replace-member$","m",3,UNBOUNDED,"*;m",ReplaceMemberFunction,"ReplaceMemberFunction",NULL);
   AddUDF(theEnv,"delete$","m",3,3,"l;m",DeleteFunction,"DeleteFunction",NULL);
   AddUDF(theEnv,"replace$","m",4,UNBOUNDED,"*;m;l;l",ReplaceFunction,"ReplaceFunction",NULL);
   AddUDF(theEnv,"insert$","m",3,UNBOUNDED,"*;m;l",InsertFunction,"InsertFunction",NULL);
   AddUDF(theEnv,"explode$","m",1,1,"s",ExplodeFunction,"ExplodeFunction",NULL);
   AddUDF(theEnv,"implode$","s",1,1,"m",ImplodeFunction,"ImplodeFunction",NULL);
   AddUDF(theEnv,"nth$","synldife",2,2,";l;m",NthFunction,"NthFunction",NULL);
   AddUDF(theEnv,"member$","blm",2,2,";*;m",MemberFunction,"MemberFunction",NULL);
   AddUDF(theEnv,"subsetp","b",2,2,";m;m",SubsetpFunction,"SubsetpFunction",NULL);
   AddUDF(theEnv,"progn$","*",0,UNBOUNDED,NULL,MultifieldPrognFunction,"MultifieldPrognFunction",NULL);
   AddUDF(theEnv,"foreach","*",0,UNBOUNDED,NULL,ForeachFunction,"ForeachFunction",NULL);
   FuncSeqOvlFlags(theEnv,"progn$",false,false);
   FuncSeqOvlFlags(theEnv,"foreach",false,false);
   AddUDF(theEnv,"(get-progn$-field)","*",0,0,NULL,GetMvPrognField,"GetMvPrognField",NULL);
   AddUDF(theEnv,"(get-progn$-index)","l",0,0,NULL,GetMvPrognIndex,"GetMvPrognIndex",NULL);
#endif

#if ! BLOAD_ONLY
   AddFunctionParser(theEnv,"progn$",MultifieldPrognParser);
   AddFunctionParser(theEnv,"foreach",ForeachParser);
#endif
  }

/****************************************/
/* DeleteFunction: H/L access routine   */
/*   for the delete$ function.          */
/****************************************/
void DeleteFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue value1, value2, value3;
   long long start, end;
   size_t rs, re, srcLen, dstLen, i, j;

   /*=======================================*/
   /* Check for the correct argument types. */
   /*=======================================*/

   if ((! UDFFirstArgument(context,MULTIFIELD_BIT,&value1)) ||
       (! UDFNextArgument(context,INTEGER_BIT,&value2)) ||
       (! UDFNextArgument(context,INTEGER_BIT,&value3)))
     { return; }

   /*===========================================*/
   /* Verify the start and end index arguments. */
   /*===========================================*/

   start = value2.integerValue->contents;
   end = value3.integerValue->contents;

   if ((end < start) || (start < 1) || (end < 1) ||
       (((long long) ((size_t) start)) != start) ||
       (((long long) ((size_t) end)) != end))
     {
      MVRangeError(theEnv,start,end,value1.range,"delete$");
      SetEvaluationError(theEnv,true);
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }
      
   /*============================================*/
   /* Convert the indices to unsigned zero-based */
   /* values including the begin value.          */
   /*============================================*/
   
   rs = (size_t) start;
   re = (size_t) end;
   srcLen = value1.range;
   
   if ((rs > srcLen) || (re > srcLen))
     {
      MVRangeError(theEnv,start,end,value1.range,"delete$");
      SetEvaluationError(theEnv,true);
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }
     
   rs--;
   re--;
   rs += value1.begin;
   re += value1.begin;
   
   /*=================================================*/
   /* Delete the section out of the multifield value. */
   /*=================================================*/

   dstLen = srcLen - (re - rs + 1);
   returnValue->begin = 0;
   returnValue->range = dstLen;
   returnValue->multifieldValue = CreateMultifield(theEnv,dstLen);

   for (i = value1.begin, j = 0; i < (value1.begin + value1.range); i++)
     {
      if ((i >= rs) && (i <= re)) continue;
      
      returnValue->multifieldValue->contents[j++].value = value1.multifieldValue->contents[i].value;
     }
  }

/*****************************************/
/* ReplaceFunction: H/L access routine   */
/*   for the replace$ function.          */
/*****************************************/
void ReplaceFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue value1, value2, value3, value4;
   Expression *fieldarg;
   long long start, end;
   size_t rs, re, srcLen, dstLen;
   size_t i, j, k;

   /*=======================================*/
   /* Check for the correct argument types. */
   /*=======================================*/

   if ((! UDFFirstArgument(context,MULTIFIELD_BIT,&value1)) ||
       (! UDFNextArgument(context,INTEGER_BIT,&value2)) ||
       (! UDFNextArgument(context,INTEGER_BIT,&value3)))
     { return; }

   /*===============================*/
   /* Create the replacement value. */
   /*===============================*/

   fieldarg = GetFirstArgument()->nextArg->nextArg->nextArg;
   if (fieldarg->nextArg != NULL)
     { StoreInMultifield(theEnv,&value4,fieldarg,true); }
   else
     { EvaluateExpression(theEnv,fieldarg,&value4); }
     
   /*===========================================*/
   /* Verify the start and end index arguments. */
   /*===========================================*/

   start = value2.integerValue->contents; // TBD Refactor
   end = value3.integerValue->contents;

   if ((end < start) || (start < 1) || (end < 1) ||
       (((long long) ((size_t) start)) != start) ||
       (((long long) ((size_t) end)) != end))
     {
      MVRangeError(theEnv,start,end,value1.range,"replace$");
      SetEvaluationError(theEnv,true);
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }
  
   /*============================================*/
   /* Convert the indices to unsigned zero-based */
   /* values including the begin value.          */
   /*============================================*/
   
   rs = (size_t) start;
   re = (size_t) end;
   srcLen = value1.range;
   
   if ((rs > srcLen) || (re > srcLen))
     {
      MVRangeError(theEnv,start,end,value1.range,"replace$");
      SetEvaluationError(theEnv,true);
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }
     
   rs--;
   re--;
   rs += value1.begin;
   re += value1.begin;
  
   /*=================================================*/
   /* Delete the section out of the multifield value. */
   /*=================================================*/

   if (value4.header->type == MULTIFIELD_TYPE) // TBD Refactor
     { dstLen = srcLen - (re - rs + 1) + value4.range; }
   else
     { dstLen = srcLen - (re - rs); }
     
   returnValue->begin = 0;
   returnValue->range = dstLen;
   returnValue->multifieldValue = CreateMultifield(theEnv,dstLen);

   for (i = value1.begin, j = 0; i < (value1.begin + value1.range); i++)
     {
      if (i == rs)
        {
         if (value4.header->type == MULTIFIELD_TYPE)
           {
            for (k = value4.begin; k < (value4.begin + value4.range); k++)
              { returnValue->multifieldValue->contents[j++].value = value4.multifieldValue->contents[k].value; }
           }
         else
           { returnValue->multifieldValue->contents[j++].value = value4.value; }
           
         continue;
        }
      else if ((i > rs) && (i <= re))
        { continue; }
      
      returnValue->multifieldValue->contents[j++].value = value1.multifieldValue->contents[i].value;
     }
  }

/**********************************************/
/* DeleteMemberFunction: H/L access routine   */
/*   for the delete-member$ function.         */
/**********************************************/
void DeleteMemberFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue resultValue, valueSought;
   unsigned int argCnt;
   size_t i, j, k, valueSoughtLength;
   size_t rs, re;
   Multifield *update;

   /*============================================*/
   /* Check for the correct number of arguments. */
   /*============================================*/

   argCnt = UDFArgumentCount(context);

   /*=======================================*/
   /* Check for the correct argument types. */
   /*=======================================*/

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&resultValue))
     { return; }

   /*===================================================*/
   /* For every value specified, delete all occurrences */
   /* of those values from the multifield.              */
   /*===================================================*/

   for (i = 2 ; i <= argCnt ; i++)
     {
      if (! UDFNextArgument(context,ANY_TYPE_BITS,&valueSought))
        {
         SetEvaluationError(theEnv,true);
         SetMultifieldErrorValue(theEnv,returnValue);
         return;
        }
        
      if (valueSought.header->type == MULTIFIELD_TYPE)
        {
         valueSoughtLength = valueSought.range;
         if (valueSoughtLength == 0) continue;
        }
      else
        { valueSoughtLength = 1; }
        
      while ((rs = FindValueInMultifield(&valueSought,&resultValue)) != VALUE_NOT_FOUND)  // TBD Refactor
        {
         update = CreateMultifield(theEnv,resultValue.range - valueSoughtLength);
         re = rs + valueSoughtLength - 1;
         
         for (j = resultValue.begin, k = 0; j < (resultValue.begin + resultValue.range); j++)
            {
             if ((j >= rs) && (j <= re)) continue;
      
             update->contents[k++].value = resultValue.multifieldValue->contents[j].value;
            }
            
         resultValue.multifieldValue = update;
         resultValue.begin = 0;
         resultValue.range = resultValue.range - valueSoughtLength;
        }
     }

   returnValue->multifieldValue = resultValue.multifieldValue;
   returnValue->begin = resultValue.begin;
   returnValue->range = resultValue.range;
  }

/***********************************************/
/* ReplaceMemberFunction: H/L access routine   */
/*   for the replace-member$ function.         */
/***********************************************/
void ReplaceMemberFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue resultValue,replVal,*delVals,tmpVal;
   unsigned int i, argCnt;
   unsigned delSize;
   size_t j, k;
   size_t mink[2], *minkp;
   size_t replLen = 1;

   /*============================================*/
   /* Check for the correct number of arguments. */
   /*============================================*/

   argCnt = UDFArgumentCount(context);

   /*=======================================*/
   /* Check for the correct argument types. */
   /*=======================================*/

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&resultValue))
     { return; }

   if (! UDFNextArgument(context,ANY_TYPE_BITS,&replVal))
     { return; }
     
   if (replVal.header->type == MULTIFIELD_TYPE)
     replLen = replVal.range;

   /*======================================================*/
   /* For the value (or values from multifield) specified, */
   /* replace all occurrences of those values with all     */
   /* values specified.                                    */
   /*======================================================*/

   delSize = (sizeof(UDFValue) * (argCnt-2));
   delVals = (UDFValue *) gm2(theEnv,delSize);

   for (i = 3 ; i <= argCnt ; i++)
     {
      if (! UDFNthArgument(context,i,ANY_TYPE_BITS,&delVals[i-3]))
        {
         rm(theEnv,delVals,delSize);
         return;
        }
     }
   minkp = NULL;
   while (FindDOsInSegment(delVals,argCnt-2,&resultValue,&j,&k,minkp,minkp ? 1 : 0))
     {
      if (ReplaceMultiValueFieldSizet(theEnv,&tmpVal,&resultValue,j,k,
                                 &replVal,"replace-member$") == false)
        {
         rm(theEnv,delVals,delSize);
         SetEvaluationError(theEnv,true);
         SetMultifieldErrorValue(theEnv,returnValue);
         return;
        }
      GenCopyMemory(UDFValue,1,&resultValue,&tmpVal);
      mink[0] = 1;
      mink[1] = j + replLen - 1;
      minkp = mink;
     }
   rm(theEnv,delVals,delSize);
   GenCopyMemory(UDFValue,1,returnValue,&resultValue);
  }

/****************************************/
/* InsertFunction: H/L access routine   */
/*   for the insert$ function.          */
/****************************************/
void InsertFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue value1, value2, value3;
   Expression *fieldarg;
   long long theIndex;
   size_t uindex;

   /*=======================================*/
   /* Check for the correct argument types. */
   /*=======================================*/

   if ((! UDFFirstArgument(context,MULTIFIELD_BIT,&value1)) ||
       (! UDFNextArgument(context,INTEGER_BIT,&value2)))
     { return; }

   /*=============================*/
   /* Create the insertion value. */
   /*=============================*/

   fieldarg = GetFirstArgument()->nextArg->nextArg;
   if (fieldarg->nextArg != NULL)
     StoreInMultifield(theEnv,&value3,fieldarg,true);
   else
     EvaluateExpression(theEnv,fieldarg,&value3);

   /*============================*/
   /* Coerce the index argument. */
   /*============================*/
   
   theIndex = value2.integerValue->contents;
   
   if ((((long long) ((size_t) theIndex)) != theIndex) ||
       (theIndex < 1))
 	 {
      MVRangeError(theEnv,theIndex,theIndex,value1.range,"insert$");
	  return;
	 }
     
   uindex = (size_t) theIndex;

   /*===========================================*/
   /* Insert the value in the multifield value. */
   /*===========================================*/

   if (InsertMultiValueField(theEnv,returnValue,&value1,uindex,
                             &value3,"insert$") == false)
     {
      SetEvaluationError(theEnv,true);
      SetMultifieldErrorValue(theEnv,returnValue);
     }
  }

/*****************************************/
/* ExplodeFunction: H/L access routine   */
/*   for the explode$ function.          */
/*****************************************/
void ExplodeFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue value;
   Multifield *theMultifield;
   size_t end;

   /*==================================*/
   /* The argument should be a string. */
   /*==================================*/

   if (! UDFFirstArgument(context,STRING_BIT,&value))
     { return; }

   /*=====================================*/
   /* Convert the string to a multifield. */
   /*=====================================*/

   theMultifield = StringToMultifield(theEnv,value.lexemeValue->contents);
   if (theMultifield == NULL)
     {
      theMultifield = CreateMultifield(theEnv,0L);
      end = 0;
     }
   else
     { end = theMultifield->length; }

   /*========================*/
   /* Return the multifield. */
   /*========================*/

   returnValue->begin = 0;
   returnValue->range = end;
   returnValue->value = theMultifield;
  }

/*****************************************/
/* ImplodeFunction: H/L access routine   */
/*   for the implode$ function.          */
/*****************************************/
void ImplodeFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;

   /*======================================*/
   /* The argument should be a multifield. */
   /*======================================*/

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&theArg))
     { return; }

   /*====================*/
   /* Return the string. */
   /*====================*/

   returnValue->value = ImplodeMultifield(theEnv,&theArg);
  }

/****************************************/
/* SubseqFunction: H/L access routine   */
/*   for the subseq$ function.          */
/****************************************/
void SubseqFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;
   Multifield *theList;
   long long start, end; /* 6.04 Bug Fix */
   size_t offset, length;
   size_t ustart, uend;

   /*===================================*/
   /* Get the segment to be subdivided. */
   /*===================================*/

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&theArg))
     { return; }

   theList = theArg.multifieldValue;
   offset = theArg.begin;
   length = theArg.range;

   /*=============================================*/
   /* Get range arguments. If they are not within */
   /* appropriate ranges, return a null segment.  */
   /*=============================================*/

   if (! UDFNextArgument(context,INTEGER_BIT,&theArg))
     { return; }

   start = theArg.integerValue->contents;

   if (! UDFNextArgument(context,INTEGER_BIT,&theArg))
     { return; }
     
   end = theArg.integerValue->contents;

   if ((end < 1) || (end < start))
     {
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }

   /*==================================================*/
   /* Adjust lengths to conform to segment boundaries. */
   /*==================================================*/
     
   if (start < 1) start = 1;
   
   if (((long long) ((size_t) start)) == start)
     { ustart = (size_t) start; }
   else
     { ustart = SIZE_MAX; }
   
   if (ustart > length)
     {
      SetMultifieldErrorValue(theEnv,returnValue);
      return;
     }

   if (((long long) ((size_t) end)) == end)
     { uend = (size_t) end; }
   else
     { uend = SIZE_MAX; }
     
   if (uend > length) uend = length;

   /*=========================*/
   /* Return the new segment. */
   /*=========================*/

   returnValue->value = theList;
   returnValue->range = ((uend - ustart) + 1);
   returnValue->begin = (offset + ustart - 1);
  }

/***************************************/
/* FirstFunction: H/L access routine   */
/*   for the first$ function.          */
/***************************************/
void FirstFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;
   Multifield *theList;

   /*===================================*/
   /* Get the segment to be subdivided. */
   /*===================================*/

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&theArg)) return;

   theList = theArg.multifieldValue;

   /*=========================*/
   /* Return the new segment. */
   /*=========================*/

   returnValue->value = theList;
   if (theArg.range >= 1)
     { returnValue->range = 1; }
   else
     { returnValue->range = 0; }
   returnValue->begin = theArg.begin;
  }

/**************************************/
/* RestFunction: H/L access routine   */
/*   for the rest$ function.          */
/**************************************/
void RestFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;
   Multifield *theList;

   /*===================================*/
   /* Get the segment to be subdivided. */
   /*===================================*/

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&theArg)) return;

   theList = theArg.multifieldValue;

   /*=========================*/
   /* Return the new segment. */
   /*=========================*/

   returnValue->value = theList;
   
   if (theArg.range > 0)
     {
      returnValue->begin = theArg.begin + 1;
      returnValue->range = theArg.range - 1;
     }
   else
     {
      returnValue->begin = theArg.begin;
      returnValue->range = theArg.range;
     }
  }

/*************************************/
/* NthFunction: H/L access routine   */
/*   for the nth$ function.          */
/*************************************/
void NthFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue value1, value2;
   Multifield *elm_ptr;
   long long n; /* 6.04 Bug Fix */
   size_t un;

   if ((! UDFFirstArgument(context,INTEGER_BIT,&value1)) ||
	   (! UDFNextArgument(context,MULTIFIELD_BIT,&value2)))
     { return; }

   n = value1.integerValue->contents; /* 6.04 Bug Fix */
   
   if (((long long) ((size_t) n)) != n)
 	 {
      returnValue->lexemeValue = CreateSymbol(theEnv,"nil");
	  return;
	 }
   un = (size_t) n;
   
   if ((un > value2.range) || (un < 1))
	 {
      returnValue->lexemeValue = CreateSymbol(theEnv,"nil");
	  return;
	 }

   elm_ptr = value2.multifieldValue;
   returnValue->value = elm_ptr->contents[value2.begin + un - 1].value;
  }

/* ------------------------------------------------------------------
 *    SubsetFunction:
 *               This function compares two multi-field variables
 *               to see if the first is a subset of the second. It
 *               does not consider order.
 *
 *    INPUTS:    Two arguments via argument stack. First is the sublist
 *               multi-field variable, the second is the list to be
 *               compared to. Both should be of type MULTIFIELD_TYPE.
 *
 *    OUTPUTS:   True if the first list is a subset of the
 *               second, else false
 *
 *    NOTES:     This function is called from H/L with the subset
 *               command. Repeated values in the sublist must also
 *               be repeated in the main list.
 * ------------------------------------------------------------------
 */

void SubsetpFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue item1, item2;
   size_t i, j;
   bool found;

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&item1))
     { return; }

   if (! UDFNextArgument(context,MULTIFIELD_BIT,&item2))
     { return; }

   if (item1.range == 0)
     {
      returnValue->lexemeValue = TrueSymbol(theEnv);
      return;
     }

   if (item2.range == 0)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   for (i = item1.begin; i < (item1.begin + item1.range); i++)
     {
      found = false;
      
      for (j = item2.begin; j < (item2.begin + item2.range); j++)
        {
         if (item1.multifieldValue->contents[i].value == item2.multifieldValue->contents[j].value)
           {
            found = true;
            break;
           }
        }
        
      if (! found)
        {
         returnValue->lexemeValue = FalseSymbol(theEnv);
         return;
        }
     }

   returnValue->lexemeValue = TrueSymbol(theEnv);
  }

/**************************************/
/* MemberFunction: H/L access routine */
/*   for the member$ function.        */
/**************************************/
void MemberFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue item1, item2;
   size_t i, pos;

   returnValue->lexemeValue = FalseSymbol(theEnv);

   if (! UDFFirstArgument(context,ANY_TYPE_BITS,&item1)) return;

   if (! UDFNextArgument(context,MULTIFIELD_BIT,&item2)) return;

   /*==========================================*/
   /* Member sought is not a multifield value. */
   /*==========================================*/
   
   if (item1.header->type != MULTIFIELD_TYPE)
     {
      for (i = item2.begin; i < (item2.begin + item2.range); i++)
        {
         if (item1.value == item2.multifieldValue->contents[i].value)
           {
            returnValue->integerValue = CreateInteger(theEnv,(long long) (i + 1));
            return;
           }
        }
      return;
     }
   
   /*=====================================*/
   /* Multifield sought can not be larger */
   /* than the multifield being searched. */
   /*=====================================*/
   
   if (item1.range > item2.range) return;
   
   /*================================================*/
   /* Search for the first multifield in the second. */
   /*================================================*/
   
   pos = FindValueInMultifield(&item1,&item2);
   
   if (pos == VALUE_NOT_FOUND) return;
      
   if (item1.range == 1)
     { returnValue->integerValue = CreateInteger(theEnv,(long long) (pos + 1)); }
   else
     {
      returnValue->value = CreateMultifield(theEnv,2);
      returnValue->multifieldValue->contents[0].integerValue = CreateInteger(theEnv,(long long) (pos + 1));
      returnValue->multifieldValue->contents[1].integerValue = CreateInteger(theEnv,(long long) (pos + item1.range));
      returnValue->begin = 0;
      returnValue->range = 2;
     }
  }

/**************************/
/* FindValueInMultifield: */
/**************************/
size_t FindValueInMultifield(
  UDFValue *valueSought,
  UDFValue *multifieldToSearch)
  {
   size_t i, j;
   bool found;

   /*==========================================*/
   /* Member sought is not a multifield value. */
   /*==========================================*/
   
   if (valueSought->header->type != MULTIFIELD_TYPE)
     {
      for (i = multifieldToSearch->begin; i < (multifieldToSearch->begin + multifieldToSearch->range); i++)
        {
         if (valueSought->value == multifieldToSearch->multifieldValue->contents[i].value)
           { return i; }
        }
        
      return VALUE_NOT_FOUND;
     }
   
   /*=====================================*/
   /* Multifield sought can not be larger */
   /* than the multifield being searched. */
   /*=====================================*/
   
   if (valueSought->range > multifieldToSearch->range) return VALUE_NOT_FOUND;
   
   /*================================================*/
   /* Search for the first multifield in the second. */
   /*================================================*/
   
   for (i = 0; i <= (multifieldToSearch->range - valueSought->range); i++)
     {
      found = true;
      for (j = 0; j < valueSought->range; j++)
        {
         if (valueSought->multifieldValue->contents[valueSought->begin+j].value !=
             multifieldToSearch->multifieldValue->contents[multifieldToSearch->begin+i+j].value)
           {
            found = false;
            break;
           }
        }
        
      if (found)
        { return i; }
     }

   return VALUE_NOT_FOUND;
  }

/*********************/
/* FindDOsInSegment: */
/*********************/
/* 6.05 Bug Fix */
bool FindDOsInSegment(
  UDFValue *searchDOs,
  unsigned int scnt,
  UDFValue *value,
  size_t *si,
  size_t *ei,
  size_t *excludes,
  unsigned int epaircnt)
  {
   size_t slen;
   unsigned int j;
   size_t i, k, mul_length;

   mul_length = value->range;
   for (i = 0 ; i < mul_length ; i++)
     {
      for (j = 0 ; j < scnt ; j++)
        {
         if (searchDOs[j].header->type == MULTIFIELD_TYPE)
           {
            slen = searchDOs[j].range;
            
            if (MVRangeCheck(i+1,i+slen,excludes,epaircnt))
              {
               for (k = 0L ; (k < slen) && ((k + i) < mul_length) ; k++)
                 if (searchDOs[j].multifieldValue->contents[k+searchDOs[j].begin].value !=
                     value->multifieldValue->contents[k+i+value->begin].value)
                   break;
               if (k >= slen)
                 {
                  *si = i + 1;
                  *ei = i + slen;
                  return true;
                 }
              }
           }
         else if ((searchDOs[j].value == value->multifieldValue->contents[i + value->begin].value) &&
                  MVRangeCheck(i+1L,i+1L,excludes,epaircnt))
           {
            *si = *ei = i + 1;
            return true;
           }
        }
     }

   return false;
  }

/*****************/
/* MVRangeCheck: */
/*****************/
static bool MVRangeCheck(
  size_t si,
  size_t ei,
  size_t *elist,
  unsigned int epaircnt)
{
  unsigned int i;

  if (!elist || !epaircnt)
    return true;
  for (i = 0 ; i < epaircnt ; i++)
    if (((si >= elist[i*2]) && (si <= elist[i*2+1])) ||
        ((ei >= elist[i*2]) && (ei <= elist[i*2+1])))
    return false;

  return true;
}

#if (! BLOAD_ONLY)

/******************************************************/
/* MultifieldPrognParser: Parses the progn$ function. */
/******************************************************/
static struct expr *MultifieldPrognParser(
  Environment *theEnv,
  struct expr *top,
  const char *infile)
  {
   struct BindInfo *oldBindList, *newBindList, *prev;
   struct token tkn;
   struct expr *tmp;
   CLIPSLexeme *fieldVar = NULL;

   SavePPBuffer(theEnv," ");
   GetToken(theEnv,infile,&tkn);

   /* ================================
      Simple form: progn$ <mf-exp> ...
      ================================ */
   if (tkn.tknType != LEFT_PARENTHESIS_TOKEN)
     {
      top->argList = ParseAtomOrExpression(theEnv,infile,&tkn);
      if (top->argList == NULL)
        {
         ReturnExpression(theEnv,top);
         return NULL;
        }
     }
   else
     {
      GetToken(theEnv,infile,&tkn);
      if (tkn.tknType != SF_VARIABLE_TOKEN)
        {
         if (tkn.tknType != SYMBOL_TOKEN)
           goto MvPrognParseError;
         top->argList = Function2Parse(theEnv,infile,tkn.lexemeValue->contents);
         if (top->argList == NULL)
           {
            ReturnExpression(theEnv,top);
            return NULL;
           }
        }

      /* =========================================
         Complex form: progn$ (<var> <mf-exp>) ...
         ========================================= */
      else
        {
         fieldVar = tkn.lexemeValue;
         SavePPBuffer(theEnv," ");
         top->argList = ParseAtomOrExpression(theEnv,infile,NULL);
         if (top->argList == NULL)
           {
            ReturnExpression(theEnv,top);
            return NULL;
           }
         GetToken(theEnv,infile,&tkn);
         if (tkn.tknType != RIGHT_PARENTHESIS_TOKEN)
           goto MvPrognParseError;
         PPBackup(theEnv);
         /* PPBackup(theEnv); */
         SavePPBuffer(theEnv,tkn.printForm);
         SavePPBuffer(theEnv," ");
        }
     }

   if (CheckArgumentAgainstRestriction(theEnv,top->argList,MULTIFIELD_BIT))
     goto MvPrognParseError;

   oldBindList = GetParsedBindNames(theEnv);
   SetParsedBindNames(theEnv,NULL);
   IncrementIndentDepth(theEnv,3);
   ExpressionData(theEnv)->BreakContext = true;
   ExpressionData(theEnv)->ReturnContext = ExpressionData(theEnv)->svContexts->rtn;
   PPCRAndIndent(theEnv);
   top->argList->nextArg = GroupActions(theEnv,infile,&tkn,true,NULL,false);
   DecrementIndentDepth(theEnv,3);
   PPBackup(theEnv);
   PPBackup(theEnv);
   SavePPBuffer(theEnv,tkn.printForm);
   if (top->argList->nextArg == NULL)
     {
      ClearParsedBindNames(theEnv);
      SetParsedBindNames(theEnv,oldBindList);
      ReturnExpression(theEnv,top);
      return NULL;
     }
   tmp = top->argList->nextArg;
   top->argList->nextArg = tmp->argList;
   tmp->argList = NULL;
   ReturnExpression(theEnv,tmp);
   newBindList = GetParsedBindNames(theEnv);
   prev = NULL;
   while (newBindList != NULL)
     {
      if ((fieldVar == NULL) ? false :
          (strcmp(newBindList->name->contents,fieldVar->contents) == 0))
        {
         ClearParsedBindNames(theEnv);
         SetParsedBindNames(theEnv,oldBindList);
         PrintErrorID(theEnv,"MULTIFUN",2,false);
         WriteString(theEnv,STDERR,"Cannot rebind field variable in function 'progn$'.\n");
         ReturnExpression(theEnv,top);
         return NULL;
        }
      prev = newBindList;
      newBindList = newBindList->next;
     }
   if (prev == NULL)
     SetParsedBindNames(theEnv,oldBindList);
   else
     prev->next = oldBindList;
   if (fieldVar != NULL)
     ReplaceMvPrognFieldVars(theEnv,fieldVar,top->argList->nextArg,0);
   return(top);

MvPrognParseError:
   SyntaxErrorMessage(theEnv,"progn$");
   ReturnExpression(theEnv,top);
   return NULL;
  }

/***********************************************/
/* ForeachParser: Parses the foreach function. */
/***********************************************/
static struct expr *ForeachParser(
  Environment *theEnv,
  struct expr *top,
  const char *infile)
  {
   struct BindInfo *oldBindList,*newBindList,*prev;
   struct token tkn;
   struct expr *tmp;
   CLIPSLexeme *fieldVar;

   SavePPBuffer(theEnv," ");
   GetToken(theEnv,infile,&tkn);

   if (tkn.tknType != SF_VARIABLE_TOKEN)
     { goto ForeachParseError; }

   fieldVar = tkn.lexemeValue;
   SavePPBuffer(theEnv," ");
   top->argList = ParseAtomOrExpression(theEnv,infile,NULL);
   if (top->argList == NULL)
     {
      ReturnExpression(theEnv,top);
      return NULL;
     }

   if (CheckArgumentAgainstRestriction(theEnv,top->argList,MULTIFIELD_BIT))
     goto ForeachParseError;

   oldBindList = GetParsedBindNames(theEnv);
   SetParsedBindNames(theEnv,NULL);
   IncrementIndentDepth(theEnv,3);
   ExpressionData(theEnv)->BreakContext = true;
   ExpressionData(theEnv)->ReturnContext = ExpressionData(theEnv)->svContexts->rtn;
   PPCRAndIndent(theEnv);
   top->argList->nextArg = GroupActions(theEnv,infile,&tkn,true,NULL,false);
   DecrementIndentDepth(theEnv,3);
   PPBackup(theEnv);
   PPBackup(theEnv);
   SavePPBuffer(theEnv,tkn.printForm);
   if (top->argList->nextArg == NULL)
     {
      ClearParsedBindNames(theEnv);
      SetParsedBindNames(theEnv,oldBindList);
      ReturnExpression(theEnv,top);
      return NULL;
     }
   tmp = top->argList->nextArg;
   top->argList->nextArg = tmp->argList;
   tmp->argList = NULL;
   ReturnExpression(theEnv,tmp);
   newBindList = GetParsedBindNames(theEnv);
   prev = NULL;
   while (newBindList != NULL)
     {
      if ((fieldVar == NULL) ? false :
          (strcmp(newBindList->name->contents,fieldVar->contents) == 0))
        {
         ClearParsedBindNames(theEnv);
         SetParsedBindNames(theEnv,oldBindList);
         PrintErrorID(theEnv,"MULTIFUN",2,false);
         WriteString(theEnv,STDERR,"Cannot rebind field variable in function 'foreach'.\n");
         ReturnExpression(theEnv,top);
         return NULL;
        }
      prev = newBindList;
      newBindList = newBindList->next;
     }
   if (prev == NULL)
     SetParsedBindNames(theEnv,oldBindList);
   else
     prev->next = oldBindList;
   if (fieldVar != NULL)
     ReplaceMvPrognFieldVars(theEnv,fieldVar,top->argList->nextArg,0);
   return(top);

ForeachParseError:
   SyntaxErrorMessage(theEnv,"foreach");
   ReturnExpression(theEnv,top);
   return NULL;
  }

/**********************************************/
/* ReplaceMvPrognFieldVars: Replaces variable */
/*   references found in the progn$ function. */
/**********************************************/
static void ReplaceMvPrognFieldVars(
  Environment *theEnv,
  CLIPSLexeme *fieldVar,
  struct expr *theExp,
  int depth)
  {
   size_t flen;

   flen = strlen(fieldVar->contents);
   while (theExp != NULL)
     {
      if ((theExp->type != SF_VARIABLE) ? false :
          (strncmp(theExp->lexemeValue->contents,fieldVar->contents,
                   (STD_SIZE) flen) == 0))
        {
         if (theExp->lexemeValue->contents[flen] == '\0')
           {
            theExp->type = FCALL;
            theExp->value = FindFunction(theEnv,"(get-progn$-field)");
            theExp->argList = GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,depth));
           }
         else if (strcmp(theExp->lexemeValue->contents + flen,"-index") == 0)
           {
            theExp->type = FCALL;
            theExp->value = FindFunction(theEnv,"(get-progn$-index)");
            theExp->argList = GenConstant(theEnv,INTEGER_TYPE,CreateInteger(theEnv,depth));
           }
        }
      else if (theExp->argList != NULL)
        {
         if ((theExp->type == FCALL) && ((theExp->value == (void *) FindFunction(theEnv,"progn$")) ||
                                        (theExp->value == (void *) FindFunction(theEnv,"foreach")) ))
           ReplaceMvPrognFieldVars(theEnv,fieldVar,theExp->argList,depth+1);
         else
           ReplaceMvPrognFieldVars(theEnv,fieldVar,theExp->argList,depth);
        }
      theExp = theExp->nextArg;
     }
  }

#endif /* (! BLOAD_ONLY) */

/*****************************************/
/* MultifieldPrognFunction: H/L access   */
/*   routine for the progn$ function.    */
/*****************************************/
void MultifieldPrognFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   MultifieldPrognDriver(context,returnValue,"progn$");
  }

/***************************************/
/* ForeachFunction: H/L access routine */
/*   for the foreach function.         */
/***************************************/
void ForeachFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   MultifieldPrognDriver(context,returnValue,"foreach");
  }

/*******************************************/
/* MultifieldPrognDriver: Driver routine   */
/*   for the progn$ and foreach functions. */
/******************************************/
static void MultifieldPrognDriver(
  UDFContext *context,
  UDFValue *returnValue,
  const char *functionName)
  {
   Expression *theExp;
   UDFValue argval;
   size_t i, end;
   FIELD_VAR_STACK *tmpField;
   GCBlock gcb;
   Environment *theEnv = context->environment;

   tmpField = get_struct(theEnv,fieldVarStack);
   tmpField->type = SYMBOL_TYPE;
   tmpField->value = FalseSymbol(theEnv);
   tmpField->nxt = MultiFunctionData(theEnv)->FieldVarStack;
   MultiFunctionData(theEnv)->FieldVarStack = tmpField;
   returnValue->value = FalseSymbol(theEnv);

   if (! UDFFirstArgument(context,MULTIFIELD_BIT,&argval))
     {
      MultiFunctionData(theEnv)->FieldVarStack = tmpField->nxt;
      rtn_struct(theEnv,fieldVarStack,tmpField);
      returnValue->value = FalseSymbol(theEnv);
      return;
     }

   GCBlockStart(theEnv,&gcb);

   end = argval.begin + argval.range;
   for (i = argval.begin; i < end; i++)
     {
      tmpField->type = argval.multifieldValue->contents[i].header->type;
      tmpField->value = argval.multifieldValue->contents[i].value;
      tmpField->index = 1 + i - argval.begin;
      for (theExp = GetFirstArgument()->nextArg ; theExp != NULL ; theExp = theExp->nextArg)
        {
         EvaluateExpression(theEnv,theExp,returnValue);

         if (EvaluationData(theEnv)->HaltExecution || ProcedureFunctionData(theEnv)->BreakFlag || ProcedureFunctionData(theEnv)->ReturnFlag)
           {
            ProcedureFunctionData(theEnv)->BreakFlag = false;
            if (EvaluationData(theEnv)->HaltExecution)
              {
               returnValue->value = FalseSymbol(theEnv);
              }
            MultiFunctionData(theEnv)->FieldVarStack = tmpField->nxt;
            rtn_struct(theEnv,fieldVarStack,tmpField);
            GCBlockEndUDF(theEnv,&gcb,returnValue);
            return;
           }

         /*===================================*/
         /* Garbage collect if this isn't the */
         /* last evaluation of the progn$.    */
         /*===================================*/

         if (((i + 1) < end) || (theExp->nextArg != NULL))
           {
            CleanCurrentGarbageFrame(theEnv,NULL);
            CallPeriodicTasks(theEnv);
           }
        }
     }

   ProcedureFunctionData(theEnv)->BreakFlag = false;
   MultiFunctionData(theEnv)->FieldVarStack = tmpField->nxt;
   rtn_struct(theEnv,fieldVarStack,tmpField);

   GCBlockEndUDF(theEnv,&gcb,returnValue);
   CallPeriodicTasks(theEnv);
  }

/*******************/
/* GetMvPrognField */
/*******************/
void GetMvPrognField(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   long long depth;
   FIELD_VAR_STACK *tmpField;

   depth = GetFirstArgument()->integerValue->contents;
   tmpField = MultiFunctionData(theEnv)->FieldVarStack;
   while (depth > 0)
     {
      tmpField = tmpField->nxt;
      depth--;
     }
   returnValue->value = tmpField->value;
  }

/*******************/
/* GetMvPrognIndex */
/*******************/
void GetMvPrognIndex(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   long long depth;
   FIELD_VAR_STACK *tmpField;

   depth = GetFirstArgument()->integerValue->contents;
   tmpField = MultiFunctionData(theEnv)->FieldVarStack;
   while (depth > 0)
     {
      tmpField = tmpField->nxt;
      depth--;
     }
     
   returnValue->integerValue = CreateInteger(theEnv,(long long) tmpField->index);
  }

#endif /* MULTIFIELD_FUNCTIONS */

#if OBJECT_SYSTEM || MULTIFIELD_FUNCTIONS

bool ReplaceMultiValueFieldSizet(
  Environment *theEnv,
  UDFValue *dst,
  UDFValue *src,
  size_t rb,
  size_t re,
  UDFValue *field,
  const char *funcName)
  {
   size_t i, j, k;
   CLIPSValue *deptr;
   CLIPSValue *septr;
   size_t srclen, dstlen;
   size_t urb, ure;

   srclen = ((src != NULL) ? src->range : 0);
   if ((re < rb) ||
	   (rb < 1) || (re < 1) ||
	   (rb > srclen) || (re > srclen))
	 {
	  MVRangeErrorSizet(theEnv,rb,re,srclen,funcName);
	  return false;
	 }
     
   rb = src->begin + rb - 1;
   re = src->begin + re - 1;
   
   if (field->header->type == MULTIFIELD_TYPE)
	 dstlen = (size_t) (srclen + field->range - (re-rb+1));
   else
	 dstlen = (size_t) (srclen + 1 - (re-rb+1));
   dst->begin = 0;
   dst->value = CreateMultifield(theEnv,dstlen);
   dst->range = dstlen;
   
   urb = (size_t) rb;
   ure = (size_t) re;
   
   for (i = 0 , j = src->begin ; j < urb ; i++ , j++)
	 {
	  deptr = &dst->multifieldValue->contents[i];
	  septr = &src->multifieldValue->contents[j];
	  deptr->value = septr->value;
	 }
   if (field->header->type != MULTIFIELD_TYPE)
	 {
	  deptr = &dst->multifieldValue->contents[i++];
	  deptr->value = field->value;
	 }
   else
	 {
	  for (k = field->begin ; k < (field->begin + field->range) ; k++ , i++)
		{
		 deptr = &dst->multifieldValue->contents[i];
		 septr = &field->multifieldValue->contents[k];
		 deptr->value = septr->value;
		}
	 }
   while (j < ure)
	 j++;
   for (j++ ; i < dstlen ; i++ , j++)
	 {
	  deptr = &dst->multifieldValue->contents[i];
	  septr = &src->multifieldValue->contents[j];
	  deptr->value = septr->value;
	 }
   return true;
  }

/**************************************************************************
  NAME         : InsertMultiValueField
  DESCRIPTION  : Performs an insert on the src multi-field value
                   storing the results in the dst multi-field value
  INPUTS       : 1) The destination value buffer
                 2) The source value (can be NULL)
                 3) The index for the change
                 4) The new field value
  RETURNS      : True if successful, false otherwise
  SIDE EFFECTS : Allocates and sets a ephemeral segment (even if new
                   number of fields is 0)
                 Src value segment is not changed
  NOTES        : index is NOT guaranteed to be valid
                 src is guaranteed to be a multi-field variable or NULL
 **************************************************************************/
bool InsertMultiValueField(
  Environment *theEnv,
  UDFValue *dst,
  UDFValue *src,
  size_t theIndex,
  UDFValue *field,
  const char *funcName)
  {
   size_t i, j, k;
   CLIPSValue *deptr, *septr;
   size_t srclen, dstlen;

   srclen = ((src != NULL) ? src->range : 0);

   if (theIndex > (srclen + 1))
     { theIndex = (srclen + 1); }
     
   dst->begin = 0;
   if (src == NULL)
     {
      if (field->header->type == MULTIFIELD_TYPE)
        {
         DuplicateMultifield(theEnv,dst,field);
         AddToMultifieldList(theEnv,dst->multifieldValue);
        }
      else
        {
         dst->value = CreateMultifield(theEnv,0L);
         dst->range = 1;
         deptr = &dst->multifieldValue->contents[0];
         deptr->value = field->value;
        }
      return true;
     }
   dstlen = (field->header->type == MULTIFIELD_TYPE) ? field->range + srclen : srclen + 1;
   dst->value = CreateMultifield(theEnv,dstlen);
   dst->range = dstlen;
   theIndex--;
   for (i = 0 , j = src->begin ; i < (size_t) theIndex ; i++ , j++)
     {
      deptr = &dst->multifieldValue->contents[i];
      septr = &src->multifieldValue->contents[j];
      deptr->value = septr->value;
     }
   if (field->header->type != MULTIFIELD_TYPE)
     {
      deptr = &dst->multifieldValue->contents[theIndex];
      deptr->value = field->value;
      i++;
     }
   else
     {
      for (k = field->begin ; k < (field->begin + field->range) ; k++ , i++)
        {
         deptr = &dst->multifieldValue->contents[i];
         septr = &field->multifieldValue->contents[k];
         deptr->value = septr->value;
        }
     }
   for ( ; j < (src->begin + src->range) ; i++ , j++)
     {
      deptr = &dst->multifieldValue->contents[i];
      septr = &src->multifieldValue->contents[j];
      deptr->value = septr->value;
     }
   return true;
  }

/*******************************************************
  NAME         : MVRangeError
  DESCRIPTION  : Prints out an error messages for index
                   out-of-range errors in multi-field
                   access functions
  INPUTS       : 1) The bad range start
                 2) The bad range end
                 3) The max end of the range (min is
                     assumed to be 1)
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 ******************************************************/
void MVRangeError(
  Environment *theEnv,
  long long brb,
  long long bre,
  size_t max,
  const char *funcName)
  {
   PrintErrorID(theEnv,"MULTIFUN",1,false);
   WriteString(theEnv,STDERR,"Multifield index ");
   if (brb == bre)
     WriteInteger(theEnv,STDERR,brb);
   else
     {
      WriteString(theEnv,STDERR,"range ");
      WriteInteger(theEnv,STDERR,brb);
      WriteString(theEnv,STDERR,"..");
      WriteInteger(theEnv,STDERR,bre);
     }
   WriteString(theEnv,STDERR," out of range 1..");
   PrintUnsignedInteger(theEnv,STDERR,max);
   if (funcName != NULL)
     {
      WriteString(theEnv,STDERR," in function '");
      WriteString(theEnv,STDERR,funcName);
      WriteString(theEnv,STDERR,"'");
     }
   WriteString(theEnv,STDERR,".\n");
  }

static void MVRangeErrorSizet(
  Environment *theEnv,
  size_t brb,
  size_t bre,
  size_t max,
  const char *funcName)
  {
   PrintErrorID(theEnv,"MULTIFUN",1,false);
   WriteString(theEnv,STDERR,"Multifield index ");
   if (brb == bre)
     PrintUnsignedInteger(theEnv,STDERR,brb);
   else
     {
      WriteString(theEnv,STDERR,"range ");
      PrintUnsignedInteger(theEnv,STDERR,brb);
      WriteString(theEnv,STDERR,"..");
      PrintUnsignedInteger(theEnv,STDERR,bre);
     }
   WriteString(theEnv,STDERR," out of range 1..");
   PrintUnsignedInteger(theEnv,STDERR,max);
   if (funcName != NULL)
     {
      WriteString(theEnv,STDERR," in function '");
      WriteString(theEnv,STDERR,funcName);
      WriteString(theEnv,STDERR,"'");
     }
   WriteString(theEnv,STDERR,".\n");
  }

#endif /* OBJECT_SYSTEM || MULTIFIELD_FUNCTIONS */
