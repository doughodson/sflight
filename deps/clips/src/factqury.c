   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  09/28/17             */
   /*                                                     */
   /*                  FACT QUERY MODULE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose: Query Functions for Objects                      */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Gary D. Riley                                        */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Added fact-set queries.                        */
/*                                                           */
/*      6.24: Corrected errors when compiling as a C++ file. */
/*            DR0868                                         */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Changed garbage collection algorithm.          */
/*                                                           */
/*            Fixes for run-time use of query functions.     */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.31: Retrieval for fact query slot function         */
/*            generates an error if the fact has been        */
/*            retracted.                                     */
/*                                                           */
/*            Functions delayed-do-for-all-facts and         */
/*            do-for-fact increment the busy count of        */
/*            matching fact sets so that actions can         */
/*            detect retracted facts.                        */
/*                                                           */
/*            Matching fact sets containing retracted facts  */
/*            are pruned.                                    */
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
/*            Eval support for run time and bload only.      */
/*                                                           */
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if FACT_SET_QUERIES

#include "argacces.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "exprnpsr.h"
#include "modulutl.h"
#include "tmpltutl.h"
#include "insfun.h"
#include "factqpsr.h"
#include "prcdrfun.h"
#include "prntutil.h"
#include "router.h"
#include "utility.h"

#include "factqury.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    PushQueryCore(Environment *);
   static void                    PopQueryCore(Environment *);
   static QUERY_CORE             *FindQueryCore(Environment *,long long);
   static QUERY_TEMPLATE         *DetermineQueryTemplates(Environment *,Expression *,const char *,unsigned *);
   static QUERY_TEMPLATE         *FormChain(Environment *,const char *,Deftemplate *,UDFValue *);
   static void                    DeleteQueryTemplates(Environment *,QUERY_TEMPLATE *);
   static bool                    TestForFirstInChain(Environment *,QUERY_TEMPLATE *,unsigned);
   static bool                    TestForFirstFactInTemplate(Environment *,Deftemplate *,QUERY_TEMPLATE *,unsigned);
   static void                    TestEntireChain(Environment *,QUERY_TEMPLATE *,unsigned);
   static void                    TestEntireTemplate(Environment *,Deftemplate *,QUERY_TEMPLATE *,unsigned);
   static void                    AddSolution(Environment *);
   static void                    PopQuerySoln(Environment *);

/****************************************************
  NAME         : SetupFactQuery
  DESCRIPTION  : Initializes fact query H/L
                   functions and parsers
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Sets up kernel functions and parsers
  NOTES        : None
 ****************************************************/
void SetupFactQuery(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,FACT_QUERY_DATA,sizeof(struct factQueryData),NULL);

#if RUN_TIME
   FactQueryData(theEnv)->QUERY_DELIMITER_SYMBOL = FindSymbolHN(theEnv,QUERY_DELIMITER_STRING,SYMBOL_BIT);
#endif

#if ! RUN_TIME
   FactQueryData(theEnv)->QUERY_DELIMITER_SYMBOL = CreateSymbol(theEnv,QUERY_DELIMITER_STRING);
   IncrementLexemeCount(FactQueryData(theEnv)->QUERY_DELIMITER_SYMBOL);

   AddUDF(theEnv,"(query-fact)","f",0,UNBOUNDED,NULL,GetQueryFact,"GetQueryFact",NULL);

   AddUDF(theEnv,"(query-fact-slot)","*",0,UNBOUNDED,NULL,GetQueryFactSlot,"GetQueryFactSlot",NULL);

   AddUDF(theEnv,"any-factp","b",0,UNBOUNDED,NULL,AnyFacts,"AnyFacts",NULL);

   AddUDF(theEnv,"find-fact","m",0,UNBOUNDED,NULL,QueryFindFact,"QueryFindFact",NULL);

   AddUDF(theEnv,"find-all-facts","m",0,UNBOUNDED,NULL,QueryFindAllFacts,"QueryFindAllFacts",NULL);

   AddUDF(theEnv,"do-for-fact","*",0,UNBOUNDED,NULL,QueryDoForFact,"QueryDoForFact",NULL);

   AddUDF(theEnv,"do-for-all-facts","*",0,UNBOUNDED,NULL,QueryDoForAllFacts,"QueryDoForAllFacts",NULL);

   AddUDF(theEnv,"delayed-do-for-all-facts","*",0,UNBOUNDED,NULL,DelayedQueryDoForAllFacts,"DelayedQueryDoForAllFacts",NULL);
#endif

   AddFunctionParser(theEnv,"any-factp",FactParseQueryNoAction);
   AddFunctionParser(theEnv,"find-fact",FactParseQueryNoAction);
   AddFunctionParser(theEnv,"find-all-facts",FactParseQueryNoAction);
   AddFunctionParser(theEnv,"do-for-fact",FactParseQueryAction);
   AddFunctionParser(theEnv,"do-for-all-facts",FactParseQueryAction);
   AddFunctionParser(theEnv,"delayed-do-for-all-facts",FactParseQueryAction);
  }

/*************************************************************
  NAME         : GetQueryFact
  DESCRIPTION  : Internal function for referring to fact
                    array on fact-queries
  INPUTS       : None
  RETURNS      : The name of the specified fact-set member
  SIDE EFFECTS : None
  NOTES        : H/L Syntax : ((query-fact) <index>)
 *************************************************************/
void GetQueryFact(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_CORE *core;

   core = FindQueryCore(theEnv,GetFirstArgument()->integerValue->contents);

   returnValue->factValue = core->solns[GetFirstArgument()->nextArg->integerValue->contents];
  }

/***************************************************************************
  NAME         : GetQueryFactSlot
  DESCRIPTION  : Internal function for referring to slots of fact in
                    fact array on fact-queries
  INPUTS       : The caller's result buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Caller's result buffer set appropriately
  NOTES        : H/L Syntax : ((query-fact-slot) <index> <slot-name>)
 **************************************************************************/
void GetQueryFactSlot(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Fact *theFact;
   UDFValue temp;
   QUERY_CORE *core;
   unsigned short position;
   const char *varSlot;

   returnValue->lexemeValue = FalseSymbol(theEnv);

   core = FindQueryCore(theEnv,GetFirstArgument()->integerValue->contents);
   theFact = core->solns[GetFirstArgument()->nextArg->integerValue->contents];
   varSlot = GetFirstArgument()->nextArg->nextArg->nextArg->lexemeValue->contents;
   
   /*=========================================*/
   /* Accessing the slot value of a retracted */
   /* fact generates an error.                */
   /*=========================================*/

   if (theFact->garbage)
     {
      FactVarSlotErrorMessage1(theEnv,theFact,varSlot);
      SetEvaluationError(theEnv,true);
      return;
     }
   
   /*=========================*/
   /* Retrieve the slot name. */
   /*=========================*/
   
   EvaluateExpression(theEnv,GetFirstArgument()->nextArg->nextArg,&temp);
   if (temp.header->type != SYMBOL_TYPE)
     {
      InvalidVarSlotErrorMessage(theEnv,varSlot);
      SetEvaluationError(theEnv,true);
      return;
     }

   /*==================================================*/
   /* Make sure the slot exists (the symbol implied is */
   /* used for the implied slot of an ordered fact).   */
   /*==================================================*/

   if (theFact->whichDeftemplate->implied)
     {
      if (strcmp(temp.lexemeValue->contents,"implied") != 0)
        {
         FactVarSlotErrorMessage2(theEnv,theFact,varSlot);
         SetEvaluationError(theEnv,true);
         return;
        }
      position = 0;
     }

   else if (FindSlot(theFact->whichDeftemplate,
                     (CLIPSLexeme *) temp.value,&position) == NULL)
     {
      FactVarSlotErrorMessage2(theEnv,theFact,varSlot);
      SetEvaluationError(theEnv,true);
      return;
     }

   returnValue->value = theFact->theProposition.contents[position].value;
   if (returnValue->header->type == MULTIFIELD_TYPE)
     {
      returnValue->begin = 0;
      returnValue->range = returnValue->multifieldValue->length;
     }
  }

/* =============================================================================
   =============================================================================
   Following are the instance query functions :

     any-factp         : Determines if any facts satisfy the query
     find-fact         : Finds first (set of) fact(s) which satisfies
                               the query and stores it in a multi-field
     find-all-facts    : Finds all (sets of) facts which satisfy the
                               the query and stores them in a multi-field
     do-for-fact       : Executes a given action for the first (set of)
                               fact(s) which satisfy the query
     do-for-all-facts  : Executes an action for all facts which satisfy
                               the query as they are found
     delayed-do-for-all-facts : Same as above - except that the list of facts
                               which satisfy the query is formed before any
                               actions are executed

     Fact candidate search algorithm :

     All permutations of first restriction template facts with other
       restriction template facts (Rightmost are varied first)

     For any one template, fact are examined in the order they were defined

     Example :
     (deftemplate a (slot v))
     (deftemplate b (slot v))
     (deftemplate c (slot v))
     (assert (a (v a1)))
     (assert (a (v a2)))
     (assert (b (v b1)))
     (assert (b (v b2)))
     (assert (c (v c1)))
     (assert (c (v c2)))
     (assert (d (v d1)))
     (assert (d (v d2)))

     (any-factp ((?a a b) (?b c)) <query>)

     The permutations (?a ?b) would be examined in the following order :

     (a1 c1),(a1 c2),(a2 c1),(a2 c2),
     (b1 c1),(b1 c2),(b2 c1),(b2 c2)

   =============================================================================
   ============================================================================= */

/******************************************************************************
  NAME         : AnyFacts
  DESCRIPTION  : Determines if there any existing facts which satisfy
                   the query
  INPUTS       : None
  RETURNS      : True if the query is satisfied, false otherwise
  SIDE EFFECTS : The query template-expressions are evaluated once,
                   and the query boolean-expression is evaluated
                   zero or more times (depending on fact restrictions
                   and how early the expression evaluates to true - if at all).
  NOTES        : H/L Syntax : See FactParseQueryNoAction()
 ******************************************************************************/
void AnyFacts(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_TEMPLATE *qtemplates;
   unsigned rcnt;
   bool testResult;

   qtemplates = DetermineQueryTemplates(theEnv,GetFirstArgument()->nextArg,
                                      "any-factp",&rcnt);
   if (qtemplates == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   PushQueryCore(theEnv);
   FactQueryData(theEnv)->QueryCore = get_struct(theEnv,query_core);
   FactQueryData(theEnv)->QueryCore->solns = (Fact **) gm2(theEnv,(sizeof(Fact *) * rcnt));
   FactQueryData(theEnv)->QueryCore->query = GetFirstArgument();
   testResult = TestForFirstInChain(theEnv,qtemplates,0);
   FactQueryData(theEnv)->AbortQuery = false;
   rm(theEnv,FactQueryData(theEnv)->QueryCore->solns,(sizeof(Fact *) * rcnt));
   rtn_struct(theEnv,query_core,FactQueryData(theEnv)->QueryCore);
   PopQueryCore(theEnv);
   DeleteQueryTemplates(theEnv,qtemplates);
   returnValue->lexemeValue = CreateBoolean(theEnv,testResult);
  }

/******************************************************************************
  NAME         : QueryFindFact
  DESCRIPTION  : Finds the first set of facts which satisfy the query and
                   stores their addresses in the user's multi-field variable
  INPUTS       : Caller's result buffer
  RETURNS      : True if the query is satisfied, false otherwise
  SIDE EFFECTS : The query template-expressions are evaluated once,
                   and the query boolean-expression is evaluated
                   zero or more times (depending on fact restrictions
                   and how early the expression evaulates to true - if at all).
  NOTES        : H/L Syntax : See ParseQueryNoAction()
 ******************************************************************************/
void QueryFindFact(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_TEMPLATE *qtemplates;
   unsigned rcnt,i;

   returnValue->begin = 0;
   returnValue->range = 0;
   qtemplates = DetermineQueryTemplates(theEnv,GetFirstArgument()->nextArg,
                                      "find-fact",&rcnt);
   if (qtemplates == NULL)
     {
      returnValue->value = CreateMultifield(theEnv,0L);
      return;
     }
   PushQueryCore(theEnv);
   FactQueryData(theEnv)->QueryCore = get_struct(theEnv,query_core);
   FactQueryData(theEnv)->QueryCore->solns = (Fact **)
                      gm2(theEnv,(sizeof(Fact *) * rcnt));
   FactQueryData(theEnv)->QueryCore->query = GetFirstArgument();
   if (TestForFirstInChain(theEnv,qtemplates,0) == true)
     {
      returnValue->value = CreateMultifield(theEnv,rcnt);
      returnValue->range = rcnt;
      for (i = 0 ; i < rcnt ; i++)
        {
         returnValue->multifieldValue->contents[i].value = FactQueryData(theEnv)->QueryCore->solns[i];
        }
     }
   else
      returnValue->value = CreateMultifield(theEnv,0L);
   FactQueryData(theEnv)->AbortQuery = false;
   rm(theEnv,FactQueryData(theEnv)->QueryCore->solns,(sizeof(Fact *) * rcnt));
   rtn_struct(theEnv,query_core,FactQueryData(theEnv)->QueryCore);
   PopQueryCore(theEnv);
   DeleteQueryTemplates(theEnv,qtemplates);
  }

/******************************************************************************
  NAME         : QueryFindAllFacts
  DESCRIPTION  : Finds all sets of facts which satisfy the query and
                   stores their names in the user's multi-field variable

                 The sets are stored sequentially :

                   Number of sets = (Multi-field length) / (Set length)

                 The first set is if the first (set length) atoms of the
                   multi-field variable, and so on.
  INPUTS       : Caller's result buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : The query template-expressions are evaluated once,
                   and the query boolean-expression is evaluated
                   once for every fact set.
  NOTES        : H/L Syntax : See ParseQueryNoAction()
 ******************************************************************************/
void QueryFindAllFacts(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_TEMPLATE *qtemplates;
   unsigned rcnt;
   size_t i, j;

   returnValue->begin = 0;
   returnValue->range = 0;
   qtemplates = DetermineQueryTemplates(theEnv,GetFirstArgument()->nextArg,
                                      "find-all-facts",&rcnt);
   if (qtemplates == NULL)
     {
      returnValue->value = CreateMultifield(theEnv,0L);
      return;
     }
   PushQueryCore(theEnv);
   FactQueryData(theEnv)->QueryCore = get_struct(theEnv,query_core);
   FactQueryData(theEnv)->QueryCore->solns = (Fact **) gm2(theEnv,(sizeof(Fact *) * rcnt));
   FactQueryData(theEnv)->QueryCore->query = GetFirstArgument();
   FactQueryData(theEnv)->QueryCore->action = NULL;
   FactQueryData(theEnv)->QueryCore->soln_set = NULL;
   FactQueryData(theEnv)->QueryCore->soln_size = rcnt;
   FactQueryData(theEnv)->QueryCore->soln_cnt = 0;
   TestEntireChain(theEnv,qtemplates,0);
   FactQueryData(theEnv)->AbortQuery = false;
   returnValue->value = CreateMultifield(theEnv,FactQueryData(theEnv)->QueryCore->soln_cnt * rcnt);
   while (FactQueryData(theEnv)->QueryCore->soln_set != NULL)
     {
      for (i = 0 , j = returnValue->range ; i < rcnt ; i++ , j++)
        {
         returnValue->multifieldValue->contents[j].value = FactQueryData(theEnv)->QueryCore->soln_set->soln[i];
        }
      returnValue->range = j;
      PopQuerySoln(theEnv);
     }
   rm(theEnv,FactQueryData(theEnv)->QueryCore->solns,(sizeof(Fact *) * rcnt));
   rtn_struct(theEnv,query_core,FactQueryData(theEnv)->QueryCore);
   PopQueryCore(theEnv);
   DeleteQueryTemplates(theEnv,qtemplates);
  }

/******************************************************************************
  NAME         : QueryDoForFact
  DESCRIPTION  : Finds the first set of facts which satisfy the query and
                   executes a user-action with that set
  INPUTS       : None
  RETURNS      : Caller's result buffer
  SIDE EFFECTS : The query template-expressions are evaluated once,
                   and the query boolean-expression is evaluated
                   zero or more times (depending on fact restrictions
                   and how early the expression evaulates to true - if at all).
                   Also the action expression is executed zero or once.
                 Caller's result buffer holds result of user-action
  NOTES        : H/L Syntax : See ParseQueryAction()
 ******************************************************************************/
void QueryDoForFact(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_TEMPLATE *qtemplates;
   unsigned i, rcnt;

   returnValue->value = FalseSymbol(theEnv);
   qtemplates = DetermineQueryTemplates(theEnv,GetFirstArgument()->nextArg->nextArg,
                                      "do-for-fact",&rcnt);
   if (qtemplates == NULL)
     return;
   PushQueryCore(theEnv);
   FactQueryData(theEnv)->QueryCore = get_struct(theEnv,query_core);
   FactQueryData(theEnv)->QueryCore->solns = (Fact **) gm2(theEnv,(sizeof(Fact *) * rcnt));
   FactQueryData(theEnv)->QueryCore->query = GetFirstArgument();
   FactQueryData(theEnv)->QueryCore->action = GetFirstArgument()->nextArg;
   
   if (TestForFirstInChain(theEnv,qtemplates,0) == true)
     {
      for (i = 0; i < rcnt; i++)
        { FactQueryData(theEnv)->QueryCore->solns[i]->patternHeader.busyCount++; }
      
      EvaluateExpression(theEnv,FactQueryData(theEnv)->QueryCore->action,returnValue);

      for (i = 0; i < rcnt; i++)
        { FactQueryData(theEnv)->QueryCore->solns[i]->patternHeader.busyCount--; }
     }
     
   FactQueryData(theEnv)->AbortQuery = false;
   ProcedureFunctionData(theEnv)->BreakFlag = false;
   rm(theEnv,FactQueryData(theEnv)->QueryCore->solns,(sizeof(Fact *) * rcnt));
   rtn_struct(theEnv,query_core,FactQueryData(theEnv)->QueryCore);
   PopQueryCore(theEnv);
   DeleteQueryTemplates(theEnv,qtemplates);
  }

/******************************************************************************
  NAME         : QueryDoForAllFacts
  DESCRIPTION  : Finds all sets of facts which satisfy the query and
                   executes a user-function for each set as it is found
  INPUTS       : Caller's result buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : The query template-expressions are evaluated once,
                   and the query boolean-expression is evaluated
                   once for every fact set.  Also, the action is
                   executed for every fact set.
                 Caller's result buffer holds result of last action executed.
  NOTES        : H/L Syntax : See FactParseQueryAction()
 ******************************************************************************/
void QueryDoForAllFacts(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_TEMPLATE *qtemplates;
   unsigned rcnt;

   returnValue->lexemeValue = FalseSymbol(theEnv);

   qtemplates = DetermineQueryTemplates(theEnv,GetFirstArgument()->nextArg->nextArg,
                                      "do-for-all-facts",&rcnt);
   if (qtemplates == NULL)
     return;

   PushQueryCore(theEnv);
   FactQueryData(theEnv)->QueryCore = get_struct(theEnv,query_core);
   FactQueryData(theEnv)->QueryCore->solns = (Fact **) gm2(theEnv,(sizeof(Fact *) * rcnt));
   FactQueryData(theEnv)->QueryCore->query = GetFirstArgument();
   FactQueryData(theEnv)->QueryCore->action = GetFirstArgument()->nextArg;
   FactQueryData(theEnv)->QueryCore->result = returnValue;
   RetainUDFV(theEnv,FactQueryData(theEnv)->QueryCore->result);
   TestEntireChain(theEnv,qtemplates,0);
   ReleaseUDFV(theEnv,FactQueryData(theEnv)->QueryCore->result);

   FactQueryData(theEnv)->AbortQuery = false;
   ProcedureFunctionData(theEnv)->BreakFlag = false;
   rm(theEnv,FactQueryData(theEnv)->QueryCore->solns,(sizeof(Fact *) * rcnt));
   rtn_struct(theEnv,query_core,FactQueryData(theEnv)->QueryCore);
   PopQueryCore(theEnv);
   DeleteQueryTemplates(theEnv,qtemplates);
  }

/******************************************************************************
  NAME         : DelayedQueryDoForAllFacts
  DESCRIPTION  : Finds all sets of facts which satisfy the query and
                   and exceutes a user-action for each set

                 This function differs from QueryDoForAllFacts() in
                   that it forms the complete list of query satisfactions
                   BEFORE executing any actions.
  INPUTS       : Caller's result buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : The query template-expressions are evaluated once,
                   and the query boolean-expression is evaluated
                   once for every fact set.  The action is executed
                   for evry query satisfaction.
                 Caller's result buffer holds result of last action executed.
  NOTES        : H/L Syntax : See FactParseQueryNoAction()
 ******************************************************************************/
void DelayedQueryDoForAllFacts(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   QUERY_TEMPLATE *qtemplates;
   unsigned rcnt;
   unsigned i;
   GCBlock gcb;
   QUERY_SOLN *theSet;

   returnValue->value = FalseSymbol(theEnv);
   qtemplates = DetermineQueryTemplates(theEnv,GetFirstArgument()->nextArg->nextArg,
                                      "delayed-do-for-all-facts",&rcnt);
   if (qtemplates == NULL)
     return;

   PushQueryCore(theEnv);
   FactQueryData(theEnv)->QueryCore = get_struct(theEnv,query_core);
   FactQueryData(theEnv)->QueryCore->solns = (Fact **) gm2(theEnv,(sizeof(Fact *) * rcnt));
   FactQueryData(theEnv)->QueryCore->query = GetFirstArgument();
   FactQueryData(theEnv)->QueryCore->action = NULL;
   FactQueryData(theEnv)->QueryCore->soln_set = NULL;
   FactQueryData(theEnv)->QueryCore->soln_size = rcnt;
   FactQueryData(theEnv)->QueryCore->soln_cnt = 0;
   TestEntireChain(theEnv,qtemplates,0);
   FactQueryData(theEnv)->AbortQuery = false;
   FactQueryData(theEnv)->QueryCore->action = GetFirstArgument()->nextArg;

   /*==============================================================*/
   /* Increment the busy count for all facts in the solution sets. */
   /*==============================================================*/
   
   GCBlockStart(theEnv,&gcb);

   for (theSet = FactQueryData(theEnv)->QueryCore->soln_set;
        theSet != NULL;
        theSet = theSet->nxt)
     {
      for (i = 0; i < rcnt; i++)
        { theSet->soln[i]->patternHeader.busyCount++; }
     }

   /*=====================*/
   /* Perform the action. */
   /*=====================*/

   for (theSet = FactQueryData(theEnv)->QueryCore->soln_set;
        theSet != NULL; )
     {
      for (i = 0 ; i < rcnt ; i++)
        {
         if (theSet->soln[i]->garbage)
           { goto nextSet; }
         FactQueryData(theEnv)->QueryCore->solns[i] = theSet->soln[i];
        }
        
      EvaluateExpression(theEnv,FactQueryData(theEnv)->QueryCore->action,returnValue);

      if (EvaluationData(theEnv)->HaltExecution || ProcedureFunctionData(theEnv)->BreakFlag || ProcedureFunctionData(theEnv)->ReturnFlag)
        { break; }

      CleanCurrentGarbageFrame(theEnv,NULL);
      CallPeriodicTasks(theEnv);
      
      nextSet: theSet = theSet->nxt;
     }

   /*==============================================================*/
   /* Decrement the busy count for all facts in the solution sets. */
   /*==============================================================*/
   
   for (theSet = FactQueryData(theEnv)->QueryCore->soln_set;
        theSet != NULL;
        theSet = theSet->nxt)
     {
      for (i = 0; i < rcnt; i++)
        { theSet->soln[i]->patternHeader.busyCount--; }
     }

   GCBlockEndUDF(theEnv,&gcb,returnValue);
   CallPeriodicTasks(theEnv);

   /*==================================*/
   /* Deallocate the query structures. */
   /*==================================*/
   
   while (FactQueryData(theEnv)->QueryCore->soln_set != NULL)
     { PopQuerySoln(theEnv); }

   ProcedureFunctionData(theEnv)->BreakFlag = false;
   rm(theEnv,FactQueryData(theEnv)->QueryCore->solns,(sizeof(Fact *) * rcnt));
   rtn_struct(theEnv,query_core,FactQueryData(theEnv)->QueryCore);
   PopQueryCore(theEnv);
   DeleteQueryTemplates(theEnv,qtemplates);
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/*******************************************************
  NAME         : PushQueryCore
  DESCRIPTION  : Pushes the current QueryCore onto stack
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Allocates new stack node and changes
                   QueryCoreStack
  NOTES        : None
 *******************************************************/
static void PushQueryCore(
  Environment *theEnv)
  {
   QUERY_STACK *qptr;

   qptr = get_struct(theEnv,query_stack);
   qptr->core = FactQueryData(theEnv)->QueryCore;
   qptr->nxt = FactQueryData(theEnv)->QueryCoreStack;
   FactQueryData(theEnv)->QueryCoreStack = qptr;
  }

/******************************************************
  NAME         : PopQueryCore
  DESCRIPTION  : Pops top of QueryCore stack and
                   restores QueryCore to this core
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Stack node deallocated, QueryCoreStack
                   changed and QueryCore reset
  NOTES        : Assumes stack is not empty
 ******************************************************/
static void PopQueryCore(
  Environment *theEnv)
  {
   QUERY_STACK *qptr;

   FactQueryData(theEnv)->QueryCore = FactQueryData(theEnv)->QueryCoreStack->core;
   qptr = FactQueryData(theEnv)->QueryCoreStack;
   FactQueryData(theEnv)->QueryCoreStack = FactQueryData(theEnv)->QueryCoreStack->nxt;
   rtn_struct(theEnv,query_stack,qptr);
  }

/***************************************************
  NAME         : FindQueryCore
  DESCRIPTION  : Looks up a QueryCore Stack Frame
                    Depth 0 is current frame
                    1 is next deepest, etc.
  INPUTS       : Depth
  RETURNS      : Address of query core stack frame
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
static QUERY_CORE *FindQueryCore(
  Environment *theEnv,
  long long depth)
  {
   QUERY_STACK *qptr;

   if (depth == 0)
     return FactQueryData(theEnv)->QueryCore;
     
   qptr = FactQueryData(theEnv)->QueryCoreStack;
   while (depth > 1)
     {
      qptr = qptr->nxt;
      depth--;
     }
     
   return qptr->core;
  }

/**********************************************************
  NAME         : DetermineQueryTemplates
  DESCRIPTION  : Builds a list of templates to be used in
                   fact queries - uses parse form.
  INPUTS       : 1) The parse template expression chain
                 2) The name of the function being executed
                 3) Caller's buffer for restriction count
                    (# of separate lists)
  RETURNS      : The query list, or NULL on errors
  SIDE EFFECTS : Memory allocated for list
                 Busy count incremented for all templates
  NOTES        : Each restriction is linked by nxt pointer,
                   multiple templates in a restriction are
                   linked by the chain pointer.
                 Rcnt caller's buffer is set to reflect the
                   total number of chains
                 Assumes templateExp is not NULL and that each
                   restriction chain is terminated with
                   the QUERY_DELIMITER_SYMBOL "(QDS)"
 **********************************************************/
static QUERY_TEMPLATE *DetermineQueryTemplates(
  Environment *theEnv,
  Expression *templateExp,
  const char *func,
  unsigned *rcnt)
  {
   QUERY_TEMPLATE *clist = NULL, *cnxt = NULL, *cchain = NULL, *tmp;
   bool new_list = false;
   UDFValue temp;
   Deftemplate *theDeftemplate;

   *rcnt = 0;
   while (templateExp != NULL)
     {
      theDeftemplate = NULL;
      
      if (templateExp->type == DEFTEMPLATE_PTR)
        { theDeftemplate = (Deftemplate *) templateExp->value; }
      else if (EvaluateExpression(theEnv,templateExp,&temp))
        {
         DeleteQueryTemplates(theEnv,clist);
         return NULL;
        }
        
      if ((theDeftemplate == NULL) &&
          (temp.value == (void *) FactQueryData(theEnv)->QUERY_DELIMITER_SYMBOL))
        {
         new_list = true;
         (*rcnt)++;
        }
      else if ((tmp = FormChain(theEnv,func,theDeftemplate,&temp)) != NULL)
        {
         if (clist == NULL)
           { clist = cnxt = cchain = tmp; }
         else if (new_list == true)
           {
            new_list = false;
            cnxt->nxt = tmp;
            cnxt = cchain = tmp;
           }
         else
           { cchain->chain = tmp; }
           
         while (cchain->chain != NULL)
           { cchain = cchain->chain; }
        }
      else
        {
         SyntaxErrorMessage(theEnv,"fact-set query class restrictions");
         DeleteQueryTemplates(theEnv,clist);
         SetEvaluationError(theEnv,true);
         return NULL;
        }
        
      templateExp = templateExp->nextArg;
     }
     
   return clist;
  }

/*************************************************************
  NAME         : FormChain
  DESCRIPTION  : Builds a list of deftemplates to be used in
                   fact queries - uses parse form.
  INPUTS       : 1) Name of calling function for error msgs
                 2) Data object - must be a symbol or a
                      multifield value containing all symbols
                 The symbols must be names of existing templates
  RETURNS      : The query chain, or NULL on errors
  SIDE EFFECTS : Memory allocated for chain
                 Busy count incremented for all templates
  NOTES        : None
 *************************************************************/
static QUERY_TEMPLATE *FormChain(
  Environment *theEnv,
  const char *func,
  Deftemplate *theDeftemplate,
  UDFValue *val)
  {
   Deftemplate *templatePtr;
   QUERY_TEMPLATE *head, *bot, *tmp;
   size_t i; /* 6.04 Bug Fix */
   const char *templateName;
   unsigned int count;

   if (theDeftemplate != NULL)
     {
      IncrementDeftemplateBusyCount(theEnv,theDeftemplate);
      head = get_struct(theEnv,query_template);
      head->templatePtr = theDeftemplate;

      head->chain = NULL;
      head->nxt = NULL;
      return head;
     }

   if (val->header->type == SYMBOL_TYPE)
     {
      /*============================================*/
      /* Allow instance-set query restrictions to   */
      /* have module specifier as part of the class */
      /* name, but search imported defclasses too   */
      /* if a module specifier is not given.        */
      /*============================================*/

      templatePtr = (Deftemplate *)
                       FindImportedConstruct(theEnv,"deftemplate",NULL,val->lexemeValue->contents,
                                             &count,true,NULL);
         
      if (templatePtr == NULL)
        {
         CantFindItemInFunctionErrorMessage(theEnv,"deftemplate",val->lexemeValue->contents,func,true);
         return NULL;
        }
        
      IncrementDeftemplateBusyCount(theEnv,templatePtr);
      head = get_struct(theEnv,query_template);
      head->templatePtr = templatePtr;

      head->chain = NULL;
      head->nxt = NULL;
      return head;
     }
     
   if (val->header->type == MULTIFIELD_TYPE)
     {
      head = bot = NULL;

      for (i = val->begin ; i < (val->begin + val->range) ; i++)
        {
         if (val->multifieldValue->contents[i].header->type == SYMBOL_TYPE)
           {
            templateName = val->multifieldValue->contents[i].lexemeValue->contents;

            templatePtr = (Deftemplate *)
                          FindImportedConstruct(theEnv,"deftemplate",NULL,templateName,
                                                &count,true,NULL);

            if (templatePtr == NULL)
              {
               CantFindItemInFunctionErrorMessage(theEnv,"deftemplate",templateName,func,true);
               DeleteQueryTemplates(theEnv,head);
               return NULL;
              }
           }
         else
           {
            DeleteQueryTemplates(theEnv,head);
            return NULL;
           }
           
         IncrementDeftemplateBusyCount(theEnv,templatePtr);
         tmp = get_struct(theEnv,query_template);
         tmp->templatePtr = templatePtr;

         tmp->chain = NULL;
         tmp->nxt = NULL;
         
         if (head == NULL)
           { head = tmp; }
         else
           { bot->chain = tmp; }
         
         bot = tmp;
        }
        
      return head;
     }
   return NULL;
  }

/******************************************************
  NAME         : DeleteQueryTemplates
  DESCRIPTION  : Deletes a query class-list
  INPUTS       : The query list address
  RETURNS      : Nothing useful
  SIDE EFFECTS : Nodes deallocated
                 Busy count decremented for all templates
  NOTES        : None
 ******************************************************/
static void DeleteQueryTemplates(
  Environment *theEnv,
  QUERY_TEMPLATE *qlist)
  {
   QUERY_TEMPLATE *tmp;

   while (qlist != NULL)
     {
      while (qlist->chain != NULL)
        {
         tmp = qlist->chain;
         qlist->chain = qlist->chain->chain;
         DecrementDeftemplateBusyCount(theEnv,tmp->templatePtr);
         rtn_struct(theEnv,query_template,tmp);
        }
      tmp = qlist;
      qlist = qlist->nxt;
      DecrementDeftemplateBusyCount(theEnv,tmp->templatePtr);
      rtn_struct(theEnv,query_template,tmp);
     }
  }

/************************************************************
  NAME         : TestForFirstInChain
  DESCRIPTION  : Processes all templates in a restriction chain
                   until success or done
  INPUTS       : 1) The current chain
                 2) The index of the chain restriction
                     (e.g. the 4th query-variable)
  RETURNS      : True if query succeeds, false otherwise
  SIDE EFFECTS : Sets current restriction class
                 Fact variable values set
  NOTES        : None
 ************************************************************/
static bool TestForFirstInChain(
  Environment *theEnv,
  QUERY_TEMPLATE *qchain,
  unsigned indx)
  {
   QUERY_TEMPLATE *qptr;

   FactQueryData(theEnv)->AbortQuery = true;
   for (qptr = qchain ; qptr != NULL ; qptr = qptr->chain)
     {
      FactQueryData(theEnv)->AbortQuery = false;

      if (TestForFirstFactInTemplate(theEnv,qptr->templatePtr,qchain,indx))
        { return true; }

      if ((EvaluationData(theEnv)->HaltExecution == true) || (FactQueryData(theEnv)->AbortQuery == true))
        return false;
     }
   return false;
  }

/*****************************************************************
  NAME         : TestForFirstFactInTemplate
  DESCRIPTION  : Processes all facts in a template
  INPUTS       : 1) Visitation traversal id
                 2) The template
                 3) The current template restriction chain
                 4) The index of the current restriction
  RETURNS      : True if query succeeds, false otherwise
  SIDE EFFECTS : Fact variable values set
  NOTES        : None
 *****************************************************************/
static bool TestForFirstFactInTemplate(
  Environment *theEnv,
  Deftemplate *templatePtr,
  QUERY_TEMPLATE *qchain,
  unsigned indx)
  {
   Fact *theFact;
   UDFValue temp;
   GCBlock gcb;
   unsigned j;

   GCBlockStart(theEnv,&gcb);

   theFact = templatePtr->factList;
   while (theFact != NULL)
     {
      FactQueryData(theEnv)->QueryCore->solns[indx] = theFact;
      if (qchain->nxt != NULL)
        {
         theFact->patternHeader.busyCount++;
         if (TestForFirstInChain(theEnv,qchain->nxt,indx+1) == true)
           {
            theFact->patternHeader.busyCount--;
            break;
           }
         theFact->patternHeader.busyCount--;
         if ((EvaluationData(theEnv)->HaltExecution == true) || (FactQueryData(theEnv)->AbortQuery == true))
           break;
        }
      else
        {
         for (j = 0; j < indx; j++)
           {
            if (FactQueryData(theEnv)->QueryCore->solns[j]->garbage)
              {
               theFact = NULL;
               goto endTest;
              }
           }
           
         theFact->patternHeader.busyCount++;
         
         EvaluateExpression(theEnv,FactQueryData(theEnv)->QueryCore->query,&temp);

         CleanCurrentGarbageFrame(theEnv,NULL);
         CallPeriodicTasks(theEnv);

         theFact->patternHeader.busyCount--;
         
         if (EvaluationData(theEnv)->HaltExecution == true)
           { break; }
           
         if (temp.value != FalseSymbol(theEnv))
           { break; }
        }
        
      /*================================================*/
      /* Get the next fact that has not been retracted. */
      /*================================================*/
      
      theFact = theFact->nextTemplateFact;
      while ((theFact != NULL) ? (theFact->garbage == 1) : false)
        { theFact = theFact->nextTemplateFact; }
     }
     
   endTest:
   
   GCBlockEnd(theEnv,&gcb);
   CallPeriodicTasks(theEnv);

   if (theFact != NULL)
     return(((EvaluationData(theEnv)->HaltExecution == true) || (FactQueryData(theEnv)->AbortQuery == true))
             ? false : true);

   return false;
  }

/************************************************************
  NAME         : TestEntireChain
  DESCRIPTION  : Processes all templates in a restriction chain
                   until done
  INPUTS       : 1) The current chain
                 2) The index of the chain restriction
                     (i.e. the 4th query-variable)
  RETURNS      : Nothing useful
  SIDE EFFECTS : Sets current restriction template
                 Query fact variables set
                 Solution sets stored in global list
  NOTES        : None
 ************************************************************/
static void TestEntireChain(
  Environment *theEnv,
  QUERY_TEMPLATE *qchain,
  unsigned indx)
  {
   QUERY_TEMPLATE *qptr;

   FactQueryData(theEnv)->AbortQuery = true;
   for (qptr = qchain ; qptr != NULL ; qptr = qptr->chain)
     {
      FactQueryData(theEnv)->AbortQuery = false;

      TestEntireTemplate(theEnv,qptr->templatePtr,qchain,indx);

      if ((EvaluationData(theEnv)->HaltExecution == true) || (FactQueryData(theEnv)->AbortQuery == true))
        return;
     }
  }

/*****************************************************************
  NAME         : TestEntireTemplate
  DESCRIPTION  : Processes all facts in a template
  INPUTS       : 1) The module for which templates tested must be
                    in scope
                 3) The template
                 4) The current template restriction chain
                 5) The index of the current restriction
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance variable values set
                 Solution sets stored in global list
  NOTES        : None
 *****************************************************************/
static void TestEntireTemplate(
  Environment *theEnv,
  Deftemplate *templatePtr,
  QUERY_TEMPLATE *qchain,
  unsigned indx)
  {
   Fact *theFact;
   UDFValue temp;
   GCBlock gcb;
   unsigned j;

   GCBlockStart(theEnv,&gcb);

   theFact = templatePtr->factList;
   while (theFact != NULL)
     {
      FactQueryData(theEnv)->QueryCore->solns[indx] = theFact;
      if (qchain->nxt != NULL)
        {
         theFact->patternHeader.busyCount++;
         TestEntireChain(theEnv,qchain->nxt,indx+1);
         theFact->patternHeader.busyCount--;
         if ((EvaluationData(theEnv)->HaltExecution == true) || (FactQueryData(theEnv)->AbortQuery == true))
           break;
        }
      else
        {
         for (j = 0; j < indx; j++)
           {
            if (FactQueryData(theEnv)->QueryCore->solns[j]->garbage)
              { goto endTest; }
           }
           
         theFact->patternHeader.busyCount++;

         EvaluateExpression(theEnv,FactQueryData(theEnv)->QueryCore->query,&temp);

         theFact->patternHeader.busyCount--;
         if (EvaluationData(theEnv)->HaltExecution == true)
           break;
         if (temp.value != FalseSymbol(theEnv))
           {
            if (FactQueryData(theEnv)->QueryCore->action != NULL)
              {
               theFact->patternHeader.busyCount++;
               ReleaseUDFV(theEnv,FactQueryData(theEnv)->QueryCore->result);
               EvaluateExpression(theEnv,FactQueryData(theEnv)->QueryCore->action,FactQueryData(theEnv)->QueryCore->result);
               RetainUDFV(theEnv,FactQueryData(theEnv)->QueryCore->result);

               theFact->patternHeader.busyCount--;
               if (ProcedureFunctionData(theEnv)->BreakFlag || ProcedureFunctionData(theEnv)->ReturnFlag)
                 {
                  FactQueryData(theEnv)->AbortQuery = true;
                  break;
                 }
               if (EvaluationData(theEnv)->HaltExecution == true)
                 break;
              }
            else
              AddSolution(theEnv);
           }
        }

      theFact = theFact->nextTemplateFact;
      while ((theFact != NULL) ? (theFact->garbage == 1) : false)
       { theFact = theFact->nextTemplateFact; }

      CleanCurrentGarbageFrame(theEnv,NULL);
      CallPeriodicTasks(theEnv);
     }

   endTest:
   
   GCBlockEnd(theEnv,&gcb);
   CallPeriodicTasks(theEnv);
  }

/***************************************************************************
  NAME         : AddSolution
  DESCRIPTION  : Adds the current fact set to a global list of
                   solutions
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Global list and count updated
  NOTES        : Solutions are stored as sequential arrays of Fact *
 ***************************************************************************/
static void AddSolution(
  Environment *theEnv)
  {
   QUERY_SOLN *new_soln;
   unsigned i;

   new_soln = (QUERY_SOLN *) gm2(theEnv,sizeof(QUERY_SOLN));
   new_soln->soln = (Fact **)
                    gm2(theEnv,(sizeof(Fact *) * (FactQueryData(theEnv)->QueryCore->soln_size)));
   for (i = 0 ; i < FactQueryData(theEnv)->QueryCore->soln_size ; i++)
     new_soln->soln[i] = FactQueryData(theEnv)->QueryCore->solns[i];
   new_soln->nxt = NULL;
   if (FactQueryData(theEnv)->QueryCore->soln_set == NULL)
     FactQueryData(theEnv)->QueryCore->soln_set = new_soln;
   else
     FactQueryData(theEnv)->QueryCore->soln_bottom->nxt = new_soln;
   FactQueryData(theEnv)->QueryCore->soln_bottom = new_soln;
   FactQueryData(theEnv)->QueryCore->soln_cnt++;
  }

/***************************************************
  NAME         : PopQuerySoln
  DESCRIPTION  : Deallocates the topmost solution
                   set for an fact-set query
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Solution set deallocated
  NOTES        : Assumes QueryCore->soln_set != 0
 ***************************************************/
static void PopQuerySoln(
  Environment *theEnv)
  {
   FactQueryData(theEnv)->QueryCore->soln_bottom = FactQueryData(theEnv)->QueryCore->soln_set;
   FactQueryData(theEnv)->QueryCore->soln_set = FactQueryData(theEnv)->QueryCore->soln_set->nxt;
   rm(theEnv,FactQueryData(theEnv)->QueryCore->soln_bottom->soln,
      (sizeof(Fact *) * FactQueryData(theEnv)->QueryCore->soln_size));
   rm(theEnv,FactQueryData(theEnv)->QueryCore->soln_bottom,sizeof(QUERY_SOLN));
  }

#endif


