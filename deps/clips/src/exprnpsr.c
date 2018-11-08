   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/15/18             */
   /*                                                     */
   /*              EXPRESSION PARSER MODULE               */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides routines for parsing expressions.       */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Module specifier can be used within an         */
/*            expression to refer to a deffunction or        */
/*            defgeneric exported by the specified module,   */
/*            but not necessarily imported by the current    */
/*            module.                                        */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Changed find construct functionality so that   */
/*            imported modules are search when locating a    */
/*            named construct.                               */
/*                                                           */
/*      6.31: Fixed bug where sequence expansion was always  */
/*            occurring when using $ with global variables.  */
/*                                                           */
/*            Fast router used for ParseConstantArguments.   */
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
/*            Static constraint checking is always enabled.  */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Eval support for run time and bload only.      */
/*                                                           */
/*************************************************************/

#include "setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "argacces.h"
#include "constant.h"
#include "cstrnchk.h"
#include "envrnmnt.h"
#include "expressn.h"
#include "memalloc.h"
#include "modulutl.h"
#include "pprint.h"
#include "prcdrfun.h"
#include "prntutil.h"
#include "router.h"
#include "scanner.h"
#include "strngrtr.h"

#if DEFRULE_CONSTRUCT
#include "network.h"
#endif

#if DEFGENERIC_CONSTRUCT
#include "genrccom.h"
#endif

#if DEFFUNCTION_CONSTRUCT
#include "dffnxfun.h"
#endif

#include "exprnpsr.h"

#if (! RUN_TIME)

/***************************************************/
/* Function0Parse: Parses a function. Assumes that */
/*   none of the function has been parsed yet.     */
/***************************************************/
struct expr *Function0Parse(
  Environment *theEnv,
  const char *logicalName)
  {
   struct token theToken;
   struct expr *top;

   /*=================================*/
   /* All functions begin with a '('. */
   /*=================================*/

   GetToken(theEnv,logicalName,&theToken);
   if (theToken.tknType != LEFT_PARENTHESIS_TOKEN)
     {
      SyntaxErrorMessage(theEnv,"function calls");
      return NULL;
     }

   /*=================================*/
   /* Parse the rest of the function. */
   /*=================================*/

   top = Function1Parse(theEnv,logicalName);
   return(top);
  }

#endif

/*******************************************************/
/* Function1Parse: Parses a function. Assumes that the */
/*   opening left parenthesis has already been parsed. */
/*******************************************************/
struct expr *Function1Parse(
  Environment *theEnv,
  const char *logicalName)
  {
   struct token theToken;
   struct expr *top;

   /*========================*/
   /* Get the function name. */
   /*========================*/

   GetToken(theEnv,logicalName,&theToken);
   if (theToken.tknType != SYMBOL_TOKEN)
     {
      PrintErrorID(theEnv,"EXPRNPSR",1,true);
      WriteString(theEnv,STDERR,"A function name must be a symbol.\n");
      return NULL;
     }

   /*=================================*/
   /* Parse the rest of the function. */
   /*=================================*/

   top = Function2Parse(theEnv,logicalName,theToken.lexemeValue->contents);
   return(top);
  }

/****************************************************/
/* Function2Parse: Parses a function. Assumes that  */
/*   the opening left parenthesis and function name */
/*   have already been parsed.                      */
/****************************************************/
struct expr *Function2Parse(
  Environment *theEnv,
  const char *logicalName,
  const char *name)
  {
   struct functionDefinition *theFunction;
   struct expr *top;
   bool moduleSpecified = false;
   unsigned position;
   CLIPSLexeme *moduleName = NULL, *constructName = NULL;
#if DEFGENERIC_CONSTRUCT
   Defgeneric *gfunc;
#endif
#if DEFFUNCTION_CONSTRUCT
   Deffunction *dptr;
#endif

   /*==============================*/
   /* Look for a module specifier. */
   /*==============================*/

   if ((position = FindModuleSeparator(name)) != 0)
     {
      moduleName = ExtractModuleName(theEnv,position,name);
      constructName = ExtractConstructName(theEnv,position,name,SYMBOL_TYPE);
      moduleSpecified = true;
     }

   /*================================*/
   /* Has the function been defined? */
   /*================================*/

   theFunction = FindFunction(theEnv,name);

#if DEFGENERIC_CONSTRUCT
   if (moduleSpecified)
     {
      if (ConstructExported(theEnv,"defgeneric",moduleName,constructName) ||
          GetCurrentModule(theEnv) == FindDefmodule(theEnv,moduleName->contents))
        { gfunc = FindDefgenericInModule(theEnv,name); }
      else
        { gfunc = NULL; }
     }
   else
     { gfunc = LookupDefgenericInScope(theEnv,name); }
#endif

#if DEFFUNCTION_CONSTRUCT
#if DEFGENERIC_CONSTRUCT
   if ((theFunction == NULL)
        && (gfunc == NULL))
#else
   if (theFunction == NULL)
#endif
     if (moduleSpecified)
       {
        if (ConstructExported(theEnv,"deffunction",moduleName,constructName) ||
            GetCurrentModule(theEnv) == FindDefmodule(theEnv,moduleName->contents))
          { dptr = FindDeffunctionInModule(theEnv,name); }
        else
          { dptr = NULL; }
       }
     else
       { dptr = LookupDeffunctionInScope(theEnv,name); }
   else
     dptr = NULL;
#endif

   /*=============================*/
   /* Define top level structure. */
   /*=============================*/

#if DEFFUNCTION_CONSTRUCT
   if (dptr != NULL)
     top = GenConstant(theEnv,PCALL,dptr);
   else
#endif
#if DEFGENERIC_CONSTRUCT
   if (gfunc != NULL)
     top = GenConstant(theEnv,GCALL,gfunc);
   else
#endif
   if (theFunction != NULL)
     top = GenConstant(theEnv,FCALL,theFunction);
   else
     {
      PrintErrorID(theEnv,"EXPRNPSR",3,true);
      WriteString(theEnv,STDERR,"Missing function declaration for '");
      WriteString(theEnv,STDERR,name);
      WriteString(theEnv,STDERR,"'.\n");
      return NULL;
     }

   /*=======================================================*/
   /* Check to see if function has its own parsing routine. */
   /*=======================================================*/

   PushRtnBrkContexts(theEnv);
   ExpressionData(theEnv)->ReturnContext = false;
   ExpressionData(theEnv)->BreakContext = false;

#if DEFGENERIC_CONSTRUCT || DEFFUNCTION_CONSTRUCT
   if (top->type == FCALL)
#endif
     {
      if (theFunction->parser != NULL)
        {
         top = (*theFunction->parser)(theEnv,top,logicalName);
         PopRtnBrkContexts(theEnv);
         if (top == NULL) return NULL;
         if (ReplaceSequenceExpansionOps(theEnv,top->argList,top,FindFunction(theEnv,"(expansion-call)"),
                                         FindFunction(theEnv,"expand$")))
           {
            ReturnExpression(theEnv,top);
            return NULL;
           }
         return(top);
        }
     }

   /*========================================*/
   /* Default parsing routine for functions. */
   /*========================================*/

   top = CollectArguments(theEnv,top,logicalName);
   PopRtnBrkContexts(theEnv);
   if (top == NULL) return NULL;

   if (ReplaceSequenceExpansionOps(theEnv,top->argList,top,FindFunction(theEnv,"(expansion-call)"),
                                    FindFunction(theEnv,"expand$")))
     {
      ReturnExpression(theEnv,top);
      return NULL;
     }

   /*============================================================*/
   /* If the function call uses the sequence expansion operator, */
   /* its arguments cannot be checked until runtime.             */
   /*============================================================*/

   if (top->value == FindFunction(theEnv,"(expansion-call)"))
     { return(top); }

   /*============================*/
   /* Check for argument errors. */
   /*============================*/

   if (top->type == FCALL)
     {
      if (CheckExpressionAgainstRestrictions(theEnv,top,theFunction,name))
        {
         ReturnExpression(theEnv,top);
         return NULL;
        }
     }

#if DEFFUNCTION_CONSTRUCT
   else if (top->type == PCALL)
     {
      if (CheckDeffunctionCall(theEnv,(Deffunction *) top->value,CountArguments(top->argList)) == false)
        {
         ReturnExpression(theEnv,top);
         return NULL;
        }
     }
#endif

   /*========================*/
   /* Return the expression. */
   /*========================*/

   return(top);
  }

/***********************************************************************
  NAME         : ReplaceSequenceExpansionOps
  DESCRIPTION  : Replaces function calls which have multifield
                   references as arguments into a call to a
                   special function which expands the multifield
                   into single arguments at run-time.
                 Multifield references which are not function
                   arguments are errors
  INPUTS       : 1) The expression
                 2) The current function call
                 3) The address of the internal H/L function
                    (expansion-call)
                 4) The address of the H/L function expand$
  RETURNS      : False if OK, true on errors
  SIDE EFFECTS : Function call expressions modified, if necessary
  NOTES        : Function calls which truly want a multifield
                   to be passed need use only a single-field
                   refernce (i.e. ? instead of $? - the $ is
                   being treated as a special expansion operator)
 **********************************************************************/
bool ReplaceSequenceExpansionOps(
  Environment *theEnv,
  Expression *actions,
  Expression *fcallexp,
  void *expcall,
  void *expmult)
  {
   Expression *theExp;

   while (actions != NULL)
     {
      if ((ExpressionData(theEnv)->SequenceOpMode == false) &&
          ((actions->type == MF_VARIABLE) || (actions->type == MF_GBL_VARIABLE)))
        {
         if (actions->type == MF_VARIABLE)
           { actions->type = SF_VARIABLE; }
         else if (actions->type == MF_GBL_VARIABLE)
           { actions->type = GBL_VARIABLE; }
        }
        
      if ((actions->type == MF_VARIABLE) || (actions->type == MF_GBL_VARIABLE) ||
          (actions->value == expmult))
        {
         if ((fcallexp->type != FCALL) ? false :
             (fcallexp->functionValue->sequenceuseok == false))
           {
            PrintErrorID(theEnv,"EXPRNPSR",4,false);
            WriteString(theEnv,STDERR,"$ Sequence operator not a valid argument for function '");
            WriteString(theEnv,STDERR,fcallexp->functionValue->callFunctionName->contents);
            WriteString(theEnv,STDERR,"'.\n");
            return true;
           }
         if (fcallexp->value != expcall)
           {
            theExp = GenConstant(theEnv,fcallexp->type,fcallexp->value);
            theExp->argList = fcallexp->argList;
            theExp->nextArg = NULL;
            fcallexp->type = FCALL;
            fcallexp->value = expcall;
            fcallexp->argList = theExp;
           }
         if (actions->value != expmult)
           {
            theExp = GenConstant(theEnv,SF_VARIABLE,actions->value);
            if (actions->type == MF_GBL_VARIABLE)
              theExp->type = GBL_VARIABLE;
            actions->argList = theExp;
            actions->type = FCALL;
            actions->value = expmult;
           }
        }
      if (actions->argList != NULL)
        {
         if ((actions->type == GCALL) ||
             (actions->type == PCALL) ||
             (actions->type == FCALL))
           theExp = actions;
         else
           theExp = fcallexp;
         if (ReplaceSequenceExpansionOps(theEnv,actions->argList,theExp,expcall,expmult))
           return true;
        }
      actions = actions->nextArg;
     }
   return false;
  }

/*************************************************/
/* PushRtnBrkContexts: Saves the current context */
/*   for the break/return functions.             */
/*************************************************/
void PushRtnBrkContexts(
  Environment *theEnv)
  {
   SavedContexts *svtmp;

   svtmp = get_struct(theEnv,savedContexts);
   svtmp->rtn = ExpressionData(theEnv)->ReturnContext;
   svtmp->brk = ExpressionData(theEnv)->BreakContext;
   svtmp->nxt = ExpressionData(theEnv)->svContexts;
   ExpressionData(theEnv)->svContexts = svtmp;
  }

/***************************************************/
/* PopRtnBrkContexts: Restores the current context */
/*   for the break/return functions.               */
/***************************************************/
void PopRtnBrkContexts(
  Environment *theEnv)
  {
   SavedContexts *svtmp;

   ExpressionData(theEnv)->ReturnContext = ExpressionData(theEnv)->svContexts->rtn;
   ExpressionData(theEnv)->BreakContext = ExpressionData(theEnv)->svContexts->brk;
   svtmp = ExpressionData(theEnv)->svContexts;
   ExpressionData(theEnv)->svContexts = ExpressionData(theEnv)->svContexts->nxt;
   rtn_struct(theEnv,savedContexts,svtmp);
  }
  
#if (! RUN_TIME)

/**********************/
/* RestrictionExists: */
/**********************/
bool RestrictionExists(
   const char *restrictionString,
   int position)
   {
    int i = 0, currentPosition = 0;

    if (restrictionString == NULL)
      { return false; }
      
    while (restrictionString[i] != '\0')
      {
       if (restrictionString[i] == ';')
         {
          if (currentPosition == position) return true;
          currentPosition++;
         }
       i++;
      }

    if (position == currentPosition) return true;

    return false;
   }

#endif

/*****************************************************************/
/* CheckExpressionAgainstRestrictions: Compares the arguments to */
/*   a function to the set of restrictions for that function to  */
/*   determine if any incompatibilities exist. If so, the value  */
/*   true is returned, otherwise false is returned.              */
/*****************************************************************/
FunctionArgumentsError CheckExpressionAgainstRestrictions(
  Environment *theEnv,
  struct expr *theExpression,
  struct functionDefinition *theFunction,
  const char *functionName)
  {
   unsigned int j = 1;
   unsigned short number1, number2;
   unsigned short argCount;
   struct expr *argPtr;
   const char *restrictions;
   unsigned defaultRestriction2, argRestriction2;

   if (theFunction->restrictions == NULL)
     { restrictions = NULL; }
   else
     { restrictions = theFunction->restrictions->contents; }

   /*=========================================*/
   /* Count the number of function arguments. */
   /*=========================================*/

   argCount = CountArguments(theExpression->argList);

   /*======================================*/
   /* Get the minimum number of arguments. */
   /*======================================*/

   number1 = theFunction->minArgs;

   /*======================================*/
   /* Get the maximum number of arguments. */
   /*======================================*/

   number2 = theFunction->maxArgs;

   /*============================================*/
   /* Check for the correct number of arguments. */
   /*============================================*/

   if ((number1 == UNBOUNDED) && (number2 == UNBOUNDED))
     { /* Any number of arguments allowed. */ }
   else if (number1 == number2)
     {
      if (argCount != number1)
        {
         ExpectedCountError(theEnv,functionName,EXACTLY,number1);
         return FAE_COUNT_ERROR;
        }
     }
   else if (argCount < number1)
     {
      ExpectedCountError(theEnv,functionName,AT_LEAST,number1);
      return FAE_COUNT_ERROR;
     }
   else if ((number2 != UNBOUNDED) && (argCount > number2))
     {
      ExpectedCountError(theEnv,functionName,NO_MORE_THAN,number2);
      return FAE_COUNT_ERROR;
     }

   /*===============================================*/
   /* Return if there are no argument restrictions. */
   /*===============================================*/

   if (restrictions == NULL) return FAE_NO_ERROR;

   /*=======================================*/
   /* Check for the default argument types. */
   /*=======================================*/

   PopulateRestriction(theEnv,&defaultRestriction2,ANY_TYPE_BITS,restrictions,0);

   /*======================*/
   /* Check each argument. */
   /*======================*/

   for (argPtr = theExpression->argList;
        argPtr != NULL;
        argPtr = argPtr->nextArg)
     {
      PopulateRestriction(theEnv,&argRestriction2,defaultRestriction2,restrictions,j);

      if (CheckArgumentAgainstRestriction(theEnv,argPtr,argRestriction2))
        {
         ExpectedTypeError0(theEnv,functionName,j);
         PrintTypesString(theEnv,STDERR,argRestriction2,true);
         return FAE_TYPE_ERROR;
        }

      j++;
     }

   return FAE_NO_ERROR;
  }

/*******************************************************/
/* CollectArguments: Parses and groups together all of */
/*   the arguments for a function call expression.     */
/*******************************************************/
struct expr *CollectArguments(
  Environment *theEnv,
  struct expr *top,
  const char *logicalName)
  {
   bool errorFlag;
   struct expr *lastOne, *nextOne;

   /*========================================*/
   /* Default parsing routine for functions. */
   /*========================================*/

   lastOne = NULL;

   while (true)
     {
      SavePPBuffer(theEnv," ");

      errorFlag = false;
      nextOne = ArgumentParse(theEnv,logicalName,&errorFlag);

      if (errorFlag == true)
        {
         ReturnExpression(theEnv,top);
         return NULL;
        }

      if (nextOne == NULL)
        {
         PPBackup(theEnv);
         PPBackup(theEnv);
         SavePPBuffer(theEnv,")");
         return(top);
        }

      if (lastOne == NULL)
        { top->argList = nextOne; }
      else
        { lastOne->nextArg = nextOne; }

      lastOne = nextOne;
     }
  }

/********************************************/
/* ArgumentParse: Parses an argument within */
/*   a function call expression.            */
/********************************************/
struct expr *ArgumentParse(
  Environment *theEnv,
  const char *logicalName,
  bool *errorFlag)
  {
   struct expr *top;
   struct token theToken;

   /*===============*/
   /* Grab a token. */
   /*===============*/

   GetToken(theEnv,logicalName,&theToken);

   /*============================*/
   /* ')' counts as no argument. */
   /*============================*/

   if (theToken.tknType == RIGHT_PARENTHESIS_TOKEN)
     { return NULL; }

   /*================================*/
   /* Parse constants and variables. */
   /*================================*/

   if ((theToken.tknType == SF_VARIABLE_TOKEN) || (theToken.tknType == MF_VARIABLE_TOKEN) ||
       (theToken.tknType == SYMBOL_TOKEN) || (theToken.tknType == STRING_TOKEN) ||
#if DEFGLOBAL_CONSTRUCT
       (theToken.tknType == GBL_VARIABLE_TOKEN) ||
       (theToken.tknType == MF_GBL_VARIABLE_TOKEN) ||
#endif
#if OBJECT_SYSTEM
       (theToken.tknType == INSTANCE_NAME_TOKEN) ||
#endif
       (theToken.tknType == FLOAT_TOKEN) || (theToken.tknType == INTEGER_TOKEN))
     { return(GenConstant(theEnv,TokenTypeToType(theToken.tknType),theToken.value)); }

   /*======================*/
   /* Parse function call. */
   /*======================*/

   if (theToken.tknType != LEFT_PARENTHESIS_TOKEN)
     {
      PrintErrorID(theEnv,"EXPRNPSR",2,true);
      WriteString(theEnv,STDERR,"Expected a constant, variable, or expression.\n");
      *errorFlag = true;
      return NULL;
     }

   top = Function1Parse(theEnv,logicalName);
   if (top == NULL) *errorFlag = true;
   return(top);
  }

/************************************************************/
/* ParseAtomOrExpression: Parses an expression which may be */
/*   a function call, atomic value (string, symbol, etc.),  */
/*   or variable (local or global).                         */
/************************************************************/
struct expr *ParseAtomOrExpression(
  Environment *theEnv,
  const char *logicalName,
  struct token *useToken)
  {
   struct token theToken, *thisToken;
   struct expr *rv;

   if (useToken == NULL)
     {
      thisToken = &theToken;
      GetToken(theEnv,logicalName,thisToken);
     }
   else thisToken = useToken;

   if ((thisToken->tknType == SYMBOL_TOKEN) || (thisToken->tknType == STRING_TOKEN) ||
       (thisToken->tknType == INTEGER_TOKEN) || (thisToken->tknType == FLOAT_TOKEN) ||
#if OBJECT_SYSTEM
       (thisToken->tknType == INSTANCE_NAME_TOKEN) ||
#endif
#if DEFGLOBAL_CONSTRUCT
       (thisToken->tknType == GBL_VARIABLE_TOKEN) ||
       (thisToken->tknType == MF_GBL_VARIABLE_TOKEN) ||
#endif
       (thisToken->tknType == SF_VARIABLE_TOKEN) || (thisToken->tknType == MF_VARIABLE_TOKEN))
     { rv = GenConstant(theEnv,TokenTypeToType(thisToken->tknType),thisToken->value); }
   else if (thisToken->tknType == LEFT_PARENTHESIS_TOKEN)
     {
      rv = Function1Parse(theEnv,logicalName);
      if (rv == NULL) return NULL;
     }
   else
     {
      PrintErrorID(theEnv,"EXPRNPSR",2,true);
      WriteString(theEnv,STDERR,"Expected a constant, variable, or expression.\n");
      return NULL;
     }

   return(rv);
  }

/*********************************************/
/* GroupActions: Groups together a series of */
/*   actions within a progn expression. Used */
/*   for example to parse the RHS of a rule. */
/*********************************************/
struct expr *GroupActions(
  Environment *theEnv,
  const char *logicalName,
  struct token *theToken,
  bool readFirstToken,
  const char *endWord,
  bool functionNameParsed)
  {
   struct expr *top, *nextOne, *lastOne = NULL;

   /*=============================*/
   /* Create the enclosing progn. */
   /*=============================*/

   top = GenConstant(theEnv,FCALL,FindFunction(theEnv,"progn"));

   /*========================================================*/
   /* Continue until all appropriate commands are processed. */
   /*========================================================*/

   while (true)
     {
      /*================================================*/
      /* Skip reading in the token if this is the first */
      /* pass and the initial token was already read    */
      /* before calling this function.                  */
      /*================================================*/

      if (readFirstToken)
        { GetToken(theEnv,logicalName,theToken); }
      else
        { readFirstToken = true; }

      /*=================================================*/
      /* Look to see if a symbol has terminated the list */
      /* of actions (such as "else" in an if function).  */
      /*=================================================*/

      if ((theToken->tknType == SYMBOL_TOKEN) &&
          (endWord != NULL) &&
          (! functionNameParsed))
        {
         if (strcmp(theToken->lexemeValue->contents,endWord) == 0)
           { return(top); }
        }

      /*====================================*/
      /* Process a function if the function */
      /* name has already been read.        */
      /*====================================*/

      if (functionNameParsed)
        {
         nextOne = Function2Parse(theEnv,logicalName,theToken->lexemeValue->contents);
         functionNameParsed = false;
        }

      /*========================================*/
      /* Process a constant or global variable. */
      /*========================================*/

      else if ((theToken->tknType == SYMBOL_TOKEN) || (theToken->tknType == STRING_TOKEN) ||
          (theToken->tknType == INTEGER_TOKEN) || (theToken->tknType == FLOAT_TOKEN) ||
#if DEFGLOBAL_CONSTRUCT
          (theToken->tknType == GBL_VARIABLE_TOKEN) ||
          (theToken->tknType == MF_GBL_VARIABLE_TOKEN) ||
#endif
#if OBJECT_SYSTEM
          (theToken->tknType == INSTANCE_NAME_TOKEN) ||
#endif
          (theToken->tknType == SF_VARIABLE_TOKEN) || (theToken->tknType == MF_VARIABLE_TOKEN))
        { nextOne = GenConstant(theEnv,TokenTypeToType(theToken->tknType),theToken->value); }

      /*=============================*/
      /* Otherwise parse a function. */
      /*=============================*/

      else if (theToken->tknType == LEFT_PARENTHESIS_TOKEN)
        { nextOne = Function1Parse(theEnv,logicalName); }

      /*======================================*/
      /* Otherwise replace sequence expansion */
      /* variables and return the expression. */
      /*======================================*/

      else
        {
         if (ReplaceSequenceExpansionOps(theEnv,top,NULL,
                                         FindFunction(theEnv,"(expansion-call)"),
                                         FindFunction(theEnv,"expand$")))
           {
            ReturnExpression(theEnv,top);
            return NULL;
           }

         return(top);
        }

      /*===========================*/
      /* Add the new action to the */
      /* list of progn arguments.  */
      /*===========================*/

      if (nextOne == NULL)
        {
         theToken->tknType = UNKNOWN_VALUE_TOKEN;
         ReturnExpression(theEnv,top);
         return NULL;
        }

      if (lastOne == NULL)
        { top->argList = nextOne; }
      else
        { lastOne->nextArg = nextOne; }

      lastOne = nextOne;

      PPCRAndIndent(theEnv);
     }
  }

/************************/
/* PopulateRestriction: */
/************************/
void PopulateRestriction(
   Environment *theEnv,
   unsigned *restriction,
   unsigned defaultRestriction,
   const char *restrictionString,
   unsigned int position)
   {
    unsigned int i = 0, currentPosition = 0, valuesRead = 0;
    char buffer[2];

    *restriction = 0;

    if (restrictionString == NULL)
      {
       *restriction = defaultRestriction;
       return;
      }

    while (restrictionString[i] != '\0')
      {
       char theChar = restrictionString[i];

       switch(theChar)
         {
          case ';':
            if (currentPosition == position) return;
            currentPosition++;
            *restriction = 0;
            valuesRead = 0;
            break;

          case 'l':
            *restriction |= INTEGER_BIT;
            valuesRead++;
            break;

          case 'd':
            *restriction |= FLOAT_BIT;
            valuesRead++;
            break;

          case 's':
            *restriction |= STRING_BIT;
            valuesRead++;
            break;

          case 'y':
            *restriction |= SYMBOL_BIT;
            valuesRead++;
            break;

          case 'n':
            *restriction |= INSTANCE_NAME_BIT;
            valuesRead++;
            break;

          case 'm':
            *restriction |= MULTIFIELD_BIT;
            valuesRead++;
            break;

          case 'f':
            *restriction |= FACT_ADDRESS_BIT;
            valuesRead++;
            break;

          case 'i':
            *restriction |= INSTANCE_ADDRESS_BIT;
            valuesRead++;
            break;

          case 'e':
            *restriction |= EXTERNAL_ADDRESS_BIT;
            valuesRead++;
            break;

          case 'v':
            *restriction |= VOID_BIT;
            valuesRead++;
            break;

          case 'b':
            *restriction |= BOOLEAN_BIT;
            valuesRead++;
            break;

          case '*':
            *restriction |= ANY_TYPE_BITS;
            valuesRead++;
            break;

          default:
            buffer[0] = theChar;
            buffer[1] = 0;
            WriteString(theEnv,STDERR,"Invalid argument type character ");
            WriteString(theEnv,STDERR,buffer);
            WriteString(theEnv,STDERR,"\n");
            valuesRead++;
            break;
         }

       i++;
      }

    if (position == currentPosition)
      {
       if (valuesRead == 0)
         { *restriction = defaultRestriction; }
       return;
      }

    *restriction = defaultRestriction;
   }

/*******************************************/
/* ParseConstantArguments: Parses a string */
/*    into a set of constant expressions.  */
/*******************************************/
Expression *ParseConstantArguments(
  Environment *theEnv,
  const char *argstr,
  bool *error)
  {
   Expression *top = NULL,*bot = NULL,*tmp;
   const char *router = "***FNXARGS***";
   struct token tkn;
   const char *oldRouter;
   const char *oldString;
   long oldIndex;

   *error = false;

   if (argstr == NULL) return NULL;

   /*=============================*/
   /* Use the fast router bypass. */
   /*=============================*/

   oldRouter = RouterData(theEnv)->FastCharGetRouter;
   oldString = RouterData(theEnv)->FastCharGetString;
   oldIndex  = RouterData(theEnv)->FastCharGetIndex;

   RouterData(theEnv)->FastCharGetRouter = router;
   RouterData(theEnv)->FastCharGetString = argstr;
   RouterData(theEnv)->FastCharGetIndex  = 0;

   /*======================*/
   /* Parse the constants. */
   /*======================*/

   GetToken(theEnv,router,&tkn);
   while (tkn.tknType != STOP_TOKEN)
     {
      if ((tkn.tknType != SYMBOL_TOKEN) && (tkn.tknType != STRING_TOKEN) &&
          (tkn.tknType != FLOAT_TOKEN) && (tkn.tknType != INTEGER_TOKEN) &&
          (tkn.tknType != INSTANCE_NAME_TOKEN))
        {
         PrintErrorID(theEnv,"EXPRNPSR",6,false);
         WriteString(theEnv,STDERR,"Only constant arguments allowed for external function call.\n");
         ReturnExpression(theEnv,top);
         *error = true;
         CloseStringSource(theEnv,router);
         return NULL;
        }
      tmp = GenConstant(theEnv,TokenTypeToType(tkn.tknType),tkn.value);
      if (top == NULL)
        top = tmp;
      else
        bot->nextArg = tmp;
      bot = tmp;
      GetToken(theEnv,router,&tkn);
     }

   /*===========================================*/
   /* Restore the old state of the fast router. */
   /*===========================================*/

   RouterData(theEnv)->FastCharGetRouter = oldRouter;
   RouterData(theEnv)->FastCharGetString = oldString;
   RouterData(theEnv)->FastCharGetIndex  = oldIndex;

   /*=======================*/
   /* Return the arguments. */
   /*=======================*/

   return(top);
  }

/************************/
/* RemoveUnneededProgn: */
/************************/
struct expr *RemoveUnneededProgn(
  Environment *theEnv,
  struct expr *theExpression)
  {
   struct functionDefinition *fptr;
   struct expr *temp;

   if (theExpression == NULL) return(theExpression);

   if (theExpression->type != FCALL) return(theExpression);

   fptr = theExpression->functionValue;

   if (fptr->functionPointer != PrognFunction)
     { return(theExpression); }

   if ((theExpression->argList != NULL) &&
       (theExpression->argList->nextArg == NULL))
     {
      temp = theExpression;
      theExpression = theExpression->argList;
      temp->argList = NULL;
      temp->nextArg = NULL;
      ReturnExpression(theEnv,temp);
     }

   return(theExpression);
  }

