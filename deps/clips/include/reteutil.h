   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  07/30/16            */
   /*                                                     */
   /*              RETE UTILITY HEADER FILE               */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides a set of utility functions useful to    */
/*   other modules.                                          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed INCREMENTAL_RESET compilation flag.    */
/*                                                           */
/*            Rule with exists CE has incorrect activation.  */
/*            DR0867                                         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Support for join network changes.              */
/*                                                           */
/*            Support for using an asterick (*) to indicate  */
/*            that existential patterns are matched.         */
/*                                                           */
/*            Support for partial match changes.             */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Added support for hashed memories.             */
/*                                                           */
/*            Removed pseudo-facts used in not CEs.          */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
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

#ifndef _H_reteutil

#pragma once

#define _H_reteutil

#include "evaluatn.h"
#include "match.h"
#include "network.h"

#define NETWORK_ASSERT  0
#define NETWORK_RETRACT 1

   void                           PrintPartialMatch(Environment *,const char *,struct partialMatch *);
   struct partialMatch           *CopyPartialMatch(Environment *,struct partialMatch *);
   struct partialMatch           *MergePartialMatches(Environment *,struct partialMatch *,struct partialMatch *);
   long                           IncrementPseudoFactIndex(void);
   struct partialMatch           *GetAlphaMemory(Environment *,struct patternNodeHeader *,unsigned long);
   struct partialMatch           *GetLeftBetaMemory(struct joinNode *,unsigned long);
   struct partialMatch           *GetRightBetaMemory(struct joinNode *,unsigned long);
   void                           ReturnLeftMemory(Environment *,struct joinNode *);
   void                           ReturnRightMemory(Environment *,struct joinNode *);
   void                           DestroyBetaMemory(Environment *,struct joinNode *,int);
   void                           FlushBetaMemory(Environment *,struct joinNode *,int);
   bool                           BetaMemoryNotEmpty(struct joinNode *);
   void                           RemoveAlphaMemoryMatches(Environment *,struct patternNodeHeader *,struct partialMatch *,
                                                                  struct alphaMatch *);
   void                           DestroyAlphaMemory(Environment *,struct patternNodeHeader *,bool);
   void                           FlushAlphaMemory(Environment *,struct patternNodeHeader *);
   void                           FlushAlphaBetaMemory(Environment *,struct partialMatch *);
   void                           DestroyAlphaBetaMemory(Environment *,struct partialMatch *);
   int                            GetPatternNumberFromJoin(struct joinNode *);
   struct multifieldMarker       *CopyMultifieldMarkers(Environment *,struct multifieldMarker *);
   struct partialMatch           *CreateAlphaMatch(Environment *,void *,struct multifieldMarker *,
                                                          struct patternNodeHeader *,unsigned long);
   void                           TraceErrorToRule(Environment *,struct joinNode *,const char *);
   void                           InitializePatternHeader(Environment *,struct patternNodeHeader *);
   void                           MarkRuleNetwork(Environment *,bool);
   void                           TagRuleNetwork(Environment *,unsigned long *,unsigned long *,unsigned long *,unsigned long *);
   bool                           FindEntityInPartialMatch(struct patternEntity *,struct partialMatch *);
   unsigned long                  ComputeRightHashValue(Environment *,struct patternNodeHeader *);
   void                           UpdateBetaPMLinks(Environment *,struct partialMatch *,struct partialMatch *,struct partialMatch *,
                                                       struct joinNode *,unsigned long,int);
   void                           UnlinkBetaPMFromNodeAndLineage(Environment *,struct joinNode *,struct partialMatch *,int);
   void                           UnlinkNonLeftLineage(Environment *,struct joinNode *,struct partialMatch *,int);
   struct partialMatch           *CreateEmptyPartialMatch(Environment *);
   void                           MarkRuleJoins(struct joinNode *,bool);
   void                           AddBlockedLink(struct partialMatch *,struct partialMatch *);
   void                           RemoveBlockedLink(struct partialMatch *);
   unsigned long                  PrintBetaMemory(Environment *,const char *,struct betaMemory *,bool,const char *,int);

#endif /* _H_reteutil */



