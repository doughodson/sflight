   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  07/30/16            */
   /*                                                     */
   /*                                                     */
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
/*      6.23: Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Removed IMPERATIVE_MESSAGE_HANDLERS            */
/*                    compilation flag.                      */
/*                                                           */
/*      6.30: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            GetConstructNameAndComment API change.         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Used gensprintf instead of sprintf.            */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Fixed linkage issue when BLOAD_AND_SAVE        */
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
/*************************************************************/

#ifndef _H_msgpsr

#pragma once

#define _H_msgpsr

#if OBJECT_SYSTEM && (! BLOAD_ONLY) && (! RUN_TIME)

#define SELF_STRING     "self"

#include "object.h"

   bool             ParseDefmessageHandler(Environment *,const char *);
   void             CreateGetAndPutHandlers(Environment *,SlotDescriptor *);

#endif /* OBJECT_SYSTEM && (! BLOAD_ONLY) && (! RUN_TIME) */

#endif /* _H_msgpsr */



