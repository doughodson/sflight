   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  08/25/16            */
   /*                                                     */
   /*           MULTIFIELD_TYPE FUNCTIONS HEADER FILE          */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary Riley and Brian Dantes                          */
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
/*      6.40: Removed LOCALE definition.                     */
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
/*************************************************************/

#ifndef _H_multifun

#pragma once

#define _H_multifun

#include "evaluatn.h"

#define VALUE_NOT_FOUND SIZE_MAX

   void                    MultifieldFunctionDefinitions(Environment *);
#if MULTIFIELD_FUNCTIONS
   void                    DeleteFunction(Environment *,UDFContext *,UDFValue *);
   void                    ReplaceFunction(Environment *,UDFContext *,UDFValue *);
   void                    DeleteMemberFunction(Environment *,UDFContext *,UDFValue *);
   void                    ReplaceMemberFunction(Environment *,UDFContext *,UDFValue *);
   void                    InsertFunction(Environment *,UDFContext *,UDFValue *);
   void                    ExplodeFunction(Environment *,UDFContext *,UDFValue *);
   void                    ImplodeFunction(Environment *,UDFContext *,UDFValue *);
   void                    SubseqFunction(Environment *,UDFContext *,UDFValue *);
   void                    FirstFunction(Environment *,UDFContext *,UDFValue *);
   void                    RestFunction(Environment *,UDFContext *,UDFValue *);
   void                    NthFunction(Environment *,UDFContext *,UDFValue *);
   void                    SubsetpFunction(Environment *,UDFContext *,UDFValue *);
   void                    MemberFunction(Environment *,UDFContext *,UDFValue *);
   void                    MultifieldPrognFunction(Environment *,UDFContext *,UDFValue *);
   void                    ForeachFunction(Environment *,UDFContext *,UDFValue *);
   void                    GetMvPrognField(Environment *,UDFContext *,UDFValue *);
   void                    GetMvPrognIndex(Environment *,UDFContext *,UDFValue *);
   bool                    FindDOsInSegment(UDFValue *,unsigned int,UDFValue *,
                                            size_t *,size_t *,size_t *,unsigned int);
#endif
   bool                    ReplaceMultiValueFieldSizet(Environment *,UDFValue *,UDFValue *,
                                                  size_t,size_t,UDFValue *,const char *);
   bool                    InsertMultiValueField(Environment *,UDFValue *,UDFValue *,
                                                 size_t,UDFValue *,const char *);
   void                    MVRangeError(Environment *,long long,long long,size_t,const char *);
   size_t                  FindValueInMultifield(UDFValue *,UDFValue *);

#endif /* _H_multifun */

