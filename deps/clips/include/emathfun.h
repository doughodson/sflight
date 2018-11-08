   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  08/25/16            */
   /*                                                     */
   /*          EXTENDED MATH FUNCTIONS HEADER FILE        */
   /*******************************************************/

/*************************************************************/
/* Purpose: Contains the code for numerous extended math     */
/*   functions including cos, sin, tan, sec, csc, cot, acos, */
/*   asin, atan, asec, acsc, acot, cosh, sinh, tanh, sech,   */
/*   csch, coth, acosh, asinh, atanh, asech, acsch, acoth,   */
/*   mod, exp, log, log10, sqrt, pi, deg-rad, rad-deg,       */
/*   deg-grad, grad-deg, **, and round.                      */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Renamed EX_MATH compiler flag to               */
/*            EXTENDED_MATH_FUNCTIONS.                       */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.40: Removed LOCALE definition.                     */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#ifndef _H_emathfun

#pragma once

#define _H_emathfun

   void                           ExtendedMathFunctionDefinitions(Environment *);
#if EXTENDED_MATH_FUNCTIONS
   void                           CosFunction(Environment *,UDFContext *,UDFValue *);
   void                           SinFunction(Environment *,UDFContext *,UDFValue *);
   void                           TanFunction(Environment *,UDFContext *,UDFValue *);
   void                           SecFunction(Environment *,UDFContext *,UDFValue *);
   void                           CscFunction(Environment *,UDFContext *,UDFValue *);
   void                           CotFunction(Environment *,UDFContext *,UDFValue *);
   void                           AcosFunction(Environment *,UDFContext *,UDFValue *);
   void                           AsinFunction(Environment *,UDFContext *,UDFValue *);
   void                           AtanFunction(Environment *,UDFContext *,UDFValue *);
   void                           AsecFunction(Environment *,UDFContext *,UDFValue *);
   void                           AcscFunction(Environment *,UDFContext *,UDFValue *);
   void                           AcotFunction(Environment *,UDFContext *,UDFValue *);
   void                           CoshFunction(Environment *,UDFContext *,UDFValue *);
   void                           SinhFunction(Environment *,UDFContext *,UDFValue *);
   void                           TanhFunction(Environment *,UDFContext *,UDFValue *);
   void                           SechFunction(Environment *,UDFContext *,UDFValue *);
   void                           CschFunction(Environment *,UDFContext *,UDFValue *);
   void                           CothFunction(Environment *,UDFContext *,UDFValue *);
   void                           AcoshFunction(Environment *,UDFContext *,UDFValue *);
   void                           AsinhFunction(Environment *,UDFContext *,UDFValue *);
   void                           AtanhFunction(Environment *,UDFContext *,UDFValue *);
   void                           AsechFunction(Environment *,UDFContext *,UDFValue *);
   void                           AcschFunction(Environment *,UDFContext *,UDFValue *);
   void                           AcothFunction(Environment *,UDFContext *,UDFValue *);
   void                           RoundFunction(Environment *,UDFContext *,UDFValue *);
   void                           ModFunction(Environment *,UDFContext *,UDFValue *);
   void                           ExpFunction(Environment *,UDFContext *,UDFValue *);
   void                           LogFunction(Environment *,UDFContext *,UDFValue *);
   void                           Log10Function(Environment *,UDFContext *,UDFValue *);
   void                           SqrtFunction(Environment *,UDFContext *,UDFValue *);
   void                           PiFunction(Environment *,UDFContext *,UDFValue *);
   void                           DegRadFunction(Environment *,UDFContext *,UDFValue *);
   void                           RadDegFunction(Environment *,UDFContext *,UDFValue *);
   void                           DegGradFunction(Environment *,UDFContext *,UDFValue *);
   void                           GradDegFunction(Environment *,UDFContext *,UDFValue *);
   void                           PowFunction(Environment *,UDFContext *,UDFValue *);
#endif

#endif /* _H_emathfun */



