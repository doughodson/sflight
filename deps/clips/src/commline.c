   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  11/13/17             */
   /*                                                     */
   /*                COMMAND LINE MODULE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides a set of routines for processing        */
/*   commands entered at the top level prompt.               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Refactored several functions and added         */
/*            additional functions for use by an interface   */
/*            layered on top of CLIPS.                       */
/*                                                           */
/*      6.30: Local variables set with the bind function     */
/*            persist until a reset/clear command is issued. */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Metrowerks CodeWarrior (MAC_MCW, IBM_MCW) is   */
/*            no longer supported.                           */
/*                                                           */
/*            UTF-8 support.                                 */
/*                                                           */
/*            Command history and editing support            */
/*                                                           */
/*            Used genstrcpy instead of strcpy.              */
/*                                                           */
/*            Added before command execution callback        */
/*            function.                                      */
/*                                                           */
/*            Fixed RouteCommand return value.               */
/*                                                           */
/*            Added AwaitingInput flag.                      */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Added code to keep track of pointers to        */
/*            constructs that are contained externally to    */
/*            to constructs, DanglingConstructs.             */
/*                                                           */
/*            Added STDOUT and STDIN logical name            */
/*            definitions.                                   */
/*                                                           */
/*      6.40: Added call to FlushParsingMessages to clear    */
/*            message buffer after each command.             */
/*                                                           */
/*            Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*            Added Env prefix to GetHaltExecution and       */
/*            SetHaltExecution functions.                    */
/*                                                           */
/*            Refactored code to reduce header dependencies  */
/*            in sysdep.c.                                   */
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
/*            Eval support for run time and bload only.      */
/*                                                           */
/*            Removed fflush of stdin.                       */
/*                                                           */
/*            Use of ?<var>, $?<var>, ?*<var>, and  $?*var*  */
/*            by itself at the command prompt and within     */
/*            the eval function now consistently returns the */
/*            value of  the variable.                        */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "setup.h"
#include "constant.h"

#include "argacces.h"
#include "constrct.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "fileutil.h"
#include "memalloc.h"
#include "multifld.h"
#include "pprint.h"
#include "prcdrfun.h"
#include "prcdrpsr.h"
#include "prntutil.h"
#include "router.h"
#include "scanner.h"
#include "strngrtr.h"
#include "symbol.h"
#include "sysdep.h"
#include "utility.h"

#include "commline.h"

/***************/
/* DEFINITIONS */
/***************/

#define NO_SWITCH         0
#define BATCH_SWITCH      1
#define BATCH_STAR_SWITCH 2
#define LOAD_SWITCH       3

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if ! RUN_TIME
   static int                     DoString(const char *,int,bool *);
   static int                     DoComment(const char *,int);
   static int                     DoWhiteSpace(const char *,int);
   static void                    DefaultGetNextEvent(Environment *);
#endif
   static void                    DeallocateCommandLineData(Environment *);

/****************************************************/
/* InitializeCommandLineData: Allocates environment */
/*    data for command line functionality.          */
/****************************************************/
void InitializeCommandLineData(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,COMMANDLINE_DATA,sizeof(struct commandLineData),DeallocateCommandLineData);

#if ! RUN_TIME
   CommandLineData(theEnv)->BannerString = BANNER_STRING;
   CommandLineData(theEnv)->EventCallback = DefaultGetNextEvent;
#endif
  }

/*******************************************************/
/* DeallocateCommandLineData: Deallocates environment */
/*    data for the command line functionality.        */
/******************************************************/
static void DeallocateCommandLineData(
  Environment *theEnv)
  {
#if ! RUN_TIME
   if (CommandLineData(theEnv)->CommandString != NULL)
     { rm(theEnv,CommandLineData(theEnv)->CommandString,CommandLineData(theEnv)->MaximumCharacters); }

   if (CommandLineData(theEnv)->CurrentCommand != NULL)
     { ReturnExpression(theEnv,CommandLineData(theEnv)->CurrentCommand); }
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }

/*************************************************/
/* RerouteStdin: Processes the -f, -f2, and -l   */
/*   options available on machines which support */
/*   argc and arv command line options.          */
/*************************************************/
void RerouteStdin(
  Environment *theEnv,
  int argc,
  char *argv[])
  {
   int i;
   int theSwitch = NO_SWITCH;

   /*======================================*/
   /* If there aren't enough arguments for */
   /* the -f argument, then return.        */
   /*======================================*/

   if (argc < 3)
     { return; }

   /*=====================================*/
   /* If argv was not passed then return. */
   /*=====================================*/

   if (argv == NULL) return;

   /*=============================================*/
   /* Process each of the command line arguments. */
   /*=============================================*/

   for (i = 1 ; i < argc ; i++)
     {
      if (strcmp(argv[i],"-f") == 0) theSwitch = BATCH_SWITCH;
#if ! RUN_TIME
      else if (strcmp(argv[i],"-f2") == 0) theSwitch = BATCH_STAR_SWITCH;
      else if (strcmp(argv[i],"-l") == 0) theSwitch = LOAD_SWITCH;
#endif
      else if (theSwitch == NO_SWITCH)
        {
         PrintErrorID(theEnv,"SYSDEP",2,false);
         WriteString(theEnv,STDERR,"Invalid option '");
         WriteString(theEnv,STDERR,argv[i]);
         WriteString(theEnv,STDERR,"'.\n");
        }

      if (i > (argc-1))
        {
         PrintErrorID(theEnv,"SYSDEP",1,false);
         WriteString(theEnv,STDERR,"No file found for '");

         switch(theSwitch)
           {
            case BATCH_SWITCH:
               WriteString(theEnv,STDERR,"-f");
               break;

            case BATCH_STAR_SWITCH:
               WriteString(theEnv,STDERR,"-f2");
               break;

            case LOAD_SWITCH:
               WriteString(theEnv,STDERR,"-l");
           }

         WriteString(theEnv,STDERR,"' option.\n");
         return;
        }

      switch(theSwitch)
        {
         case BATCH_SWITCH:
            OpenBatch(theEnv,argv[++i],true);
            break;

#if (! RUN_TIME) && (! BLOAD_ONLY)
         case BATCH_STAR_SWITCH:
            BatchStar(theEnv,argv[++i]);
            break;

         case LOAD_SWITCH:
            Load(theEnv,argv[++i]);
            break;
#endif
        }
     }
  }

#if ! RUN_TIME

/***************************************************/
/* ExpandCommandString: Appends a character to the */
/*   command string. Returns true if the command   */
/*   string was successfully expanded, otherwise   */
/*   false. Expanding the string also includes     */
/*   adding a backspace character which reduces    */
/*   string's length.                              */
/***************************************************/
bool ExpandCommandString(
  Environment *theEnv,
  int inchar)
  {
   size_t k;

   k = RouterData(theEnv)->CommandBufferInputCount;
   CommandLineData(theEnv)->CommandString = ExpandStringWithChar(theEnv,inchar,CommandLineData(theEnv)->CommandString,&RouterData(theEnv)->CommandBufferInputCount,
                                        &CommandLineData(theEnv)->MaximumCharacters,CommandLineData(theEnv)->MaximumCharacters+80);
   return((RouterData(theEnv)->CommandBufferInputCount != k) ? true : false);
  }

/******************************************************************/
/* FlushCommandString: Empties the contents of the CommandString. */
/******************************************************************/
void FlushCommandString(
  Environment *theEnv)
  {
   if (CommandLineData(theEnv)->CommandString != NULL) rm(theEnv,CommandLineData(theEnv)->CommandString,CommandLineData(theEnv)->MaximumCharacters);
   CommandLineData(theEnv)->CommandString = NULL;
   CommandLineData(theEnv)->MaximumCharacters = 0;
   RouterData(theEnv)->CommandBufferInputCount = 0;
   RouterData(theEnv)->InputUngets = 0;
   RouterData(theEnv)->AwaitingInput = true;
  }

/*********************************************************************************/
/* SetCommandString: Sets the contents of the CommandString to a specific value. */
/*********************************************************************************/
void SetCommandString(
  Environment *theEnv,
  const char *str)
  {
   size_t length;

   FlushCommandString(theEnv);
   length = strlen(str);
   CommandLineData(theEnv)->CommandString = (char *)
                   genrealloc(theEnv,CommandLineData(theEnv)->CommandString,
                              CommandLineData(theEnv)->MaximumCharacters,
                              CommandLineData(theEnv)->MaximumCharacters + length + 1);

   genstrcpy(CommandLineData(theEnv)->CommandString,str);
   CommandLineData(theEnv)->MaximumCharacters += (length + 1);
   RouterData(theEnv)->CommandBufferInputCount += length;
  }

/*************************************************************/
/* SetNCommandString: Sets the contents of the CommandString */
/*   to a specific value up to N characters.                 */
/*************************************************************/
void SetNCommandString(
  Environment *theEnv,
  const char *str,
  unsigned length)
  {
   FlushCommandString(theEnv);
   CommandLineData(theEnv)->CommandString = (char *)
                   genrealloc(theEnv,CommandLineData(theEnv)->CommandString,
                              CommandLineData(theEnv)->MaximumCharacters,
                              CommandLineData(theEnv)->MaximumCharacters + length + 1);

   genstrncpy(CommandLineData(theEnv)->CommandString,str,length);
   CommandLineData(theEnv)->CommandString[CommandLineData(theEnv)->MaximumCharacters + length] = 0;
   CommandLineData(theEnv)->MaximumCharacters += (length + 1);
   RouterData(theEnv)->CommandBufferInputCount += length;
  }

/******************************************************************************/
/* AppendCommandString: Appends a value to the contents of the CommandString. */
/******************************************************************************/
void AppendCommandString(
  Environment *theEnv,
  const char *str)
  {
   CommandLineData(theEnv)->CommandString = AppendToString(theEnv,str,CommandLineData(theEnv)->CommandString,&RouterData(theEnv)->CommandBufferInputCount,&CommandLineData(theEnv)->MaximumCharacters);
  }

/******************************************************************************/
/* InsertCommandString: Inserts a value in the contents of the CommandString. */
/******************************************************************************/
void InsertCommandString(
  Environment *theEnv,
  const char *str,
  unsigned int position)
  {
   CommandLineData(theEnv)->CommandString =
      InsertInString(theEnv,str,position,CommandLineData(theEnv)->CommandString,
                     &RouterData(theEnv)->CommandBufferInputCount,&CommandLineData(theEnv)->MaximumCharacters);
  }

/************************************************************/
/* AppendNCommandString: Appends a value up to N characters */
/*   to the contents of the CommandString.                  */
/************************************************************/
void AppendNCommandString(
  Environment *theEnv,
  const char *str,
  unsigned length)
  {
   CommandLineData(theEnv)->CommandString = AppendNToString(theEnv,str,CommandLineData(theEnv)->CommandString,length,&RouterData(theEnv)->CommandBufferInputCount,&CommandLineData(theEnv)->MaximumCharacters);
  }

/*****************************************************************************/
/* GetCommandString: Returns a pointer to the contents of the CommandString. */
/*****************************************************************************/
char *GetCommandString(
  Environment *theEnv)
  {
   return(CommandLineData(theEnv)->CommandString);
  }

/**************************************************************************/
/* CompleteCommand: Determines whether a string forms a complete command. */
/*   A complete command is either a constant, a variable, or a function   */
/*   call which is followed (at some point) by a carriage return. Once a  */
/*   complete command is found (not including the parenthesis),           */
/*   extraneous parenthesis and other tokens are ignored. If a complete   */
/*   command exists, then 1 is returned. 0 is returned if the command was */
/*   not complete and without errors. -1 is returned if the command       */
/*   contains an error.                                                   */
/**************************************************************************/
int CompleteCommand(
  const char *mstring)
  {
   int i;
   char inchar;
   int depth = 0;
   bool moreThanZero = false;
   bool complete;
   bool error = false;

   if (mstring == NULL) return 0;

   /*===================================================*/
   /* Loop through each character of the command string */
   /* to determine if there is a complete command.      */
   /*===================================================*/

   i = 0;
   while ((inchar = mstring[i++]) != EOS)
     {
      switch(inchar)
        {
         /*======================================================*/
         /* If a carriage return or line feed is found, there is */
         /* at least one completed token in the command buffer,  */
         /* and parentheses are balanced, then a complete        */
         /* command has been found. Otherwise, remove all white  */
         /* space beginning with the current character.          */
         /*======================================================*/

         case '\n' :
         case '\r' :
           if (error) return(-1);
           if (moreThanZero && (depth == 0)) return 1;
           i = DoWhiteSpace(mstring,i);
           break;

         /*=====================*/
         /* Remove white space. */
         /*=====================*/

         case ' ' :
         case '\f' :
         case '\t' :
           i = DoWhiteSpace(mstring,i);
           break;

         /*======================================================*/
         /* If the opening quotation of a string is encountered, */
         /* determine if the closing quotation of the string is  */
         /* in the command buffer. Until the closing quotation   */
         /* is found, a complete command can not be made.        */
         /*======================================================*/

         case '"' :
           i = DoString(mstring,i,&complete);
           if ((depth == 0) && complete) moreThanZero = true;
           break;

         /*====================*/
         /* Process a comment. */
         /*====================*/

         case ';' :
           i = DoComment(mstring,i);
           if (moreThanZero && (depth == 0) && (mstring[i] != EOS))
             {
              if (error) return -1;
              else return 1;
             }
           else if (mstring[i] != EOS) i++;
           break;

         /*====================================================*/
         /* A left parenthesis increases the nesting depth of  */
         /* the current command by 1. Don't bother to increase */
         /* the depth if the first token encountered was not   */
         /* a parenthesis (e.g. for the command string         */
         /* "red (+ 3 4", the symbol red already forms a       */
         /* complete command, so the next carriage return will */
         /* cause evaluation of red--the closing parenthesis   */
         /* for "(+ 3 4" does not have to be found).           */
         /*====================================================*/

         case '(' :
           if ((depth > 0) || (moreThanZero == false))
             {
              depth++;
              moreThanZero = true;
             }
           break;

         /*====================================================*/
         /* A right parenthesis decreases the nesting depth of */
         /* the current command by 1. If the parenthesis is    */
         /* the first token of the command, then an error is   */
         /* generated.                                         */
         /*====================================================*/

         case ')' :
           if (depth > 0) depth--;
           else if (moreThanZero == false) error = true;
           break;

         /*=====================================================*/
         /* If the command begins with any other character and  */
         /* an opening parenthesis hasn't yet been found, then  */
         /* skip all characters on the same line. If a carriage */
         /* return or line feed is found, then a complete       */
         /* command exists.                                     */
         /*=====================================================*/

         default:
           if (depth == 0)
             {
              if (IsUTF8MultiByteStart(inchar) || isprint(inchar))
                {
                 while ((inchar = mstring[i++]) != EOS)
                   {
                    if ((inchar == '\n') || (inchar == '\r'))
                      {
                       if (error) return -1;
                       else return 1;
                      }
                   }
                 return 0;
                }
             }
           break;
        }
     }

   /*====================================================*/
   /* Return 0 because a complete command was not found. */
   /*====================================================*/

   return 0;
  }

/***********************************************************/
/* DoString: Skips over a string contained within a string */
/*   until the closing quotation mark is encountered.      */
/***********************************************************/
static int DoString(
  const char *str,
  int pos,
  bool *complete)
  {
   int inchar;

   /*=================================================*/
   /* Process the string character by character until */
   /* the closing quotation mark is found.            */
   /*=================================================*/

   inchar = str[pos];
   while (inchar  != '"')
     {
      /*=====================================================*/
      /* If a \ is found, then the next character is ignored */
      /* even if it is a closing quotation mark.             */
      /*=====================================================*/

      if (inchar == '\\')
        {
         pos++;
         inchar = str[pos];
        }

      /*===================================================*/
      /* If the end of input is reached before the closing */
      /* quotation mark is found, the return the last      */
      /* position that was reached and indicate that a     */
      /* complete string was not found.                    */
      /*===================================================*/

      if (inchar == EOS)
        {
         *complete = false;
         return(pos);
        }

      /*================================*/
      /* Move on to the next character. */
      /*================================*/

      pos++;
      inchar = str[pos];
     }

   /*======================================================*/
   /* Indicate that a complete string was found and return */
   /* the position of the closing quotation mark.          */
   /*======================================================*/

   pos++;
   *complete = true;
   return(pos);
  }

/*************************************************************/
/* DoComment: Skips over a comment contained within a string */
/*   until a line feed or carriage return is encountered.    */
/*************************************************************/
static int DoComment(
  const char *str,
  int pos)
  {
   int inchar;

   inchar = str[pos];
   while ((inchar != '\n') && (inchar != '\r'))
     {
      if (inchar == EOS)
        { return(pos); }

      pos++;
      inchar = str[pos];
     }

   return(pos);
  }

/**************************************************************/
/* DoWhiteSpace: Skips over white space consisting of spaces, */
/*   tabs, and form feeds that is contained within a string.  */
/**************************************************************/
static int DoWhiteSpace(
  const char *str,
  int pos)
  {
   int inchar;

   inchar = str[pos];
   while ((inchar == ' ') || (inchar == '\f') || (inchar == '\t'))
     {
      pos++;
      inchar = str[pos];
     }

   return(pos);
  }

/********************************************************************/
/* CommandLoop: Endless loop which waits for user commands and then */
/*   executes them. The command loop will bypass the EventFunction  */
/*   if there is an active batch file.                              */
/********************************************************************/
void CommandLoop(
  Environment *theEnv)
  {
   int inchar;

   WriteString(theEnv,STDOUT,CommandLineData(theEnv)->BannerString);
   SetHaltExecution(theEnv,false);
   SetEvaluationError(theEnv,false);

   CleanCurrentGarbageFrame(theEnv,NULL);
   CallPeriodicTasks(theEnv);

   PrintPrompt(theEnv);
   RouterData(theEnv)->CommandBufferInputCount = 0;
   RouterData(theEnv)->InputUngets = 0;
   RouterData(theEnv)->AwaitingInput = true;

   while (true)
     {
      /*===================================================*/
      /* If a batch file is active, grab the command input */
      /* directly from the batch file, otherwise call the  */
      /* event function.                                   */
      /*===================================================*/

      if (BatchActive(theEnv) == true)
        {
         inchar = LLGetcBatch(theEnv,STDIN,true);
         if (inchar == EOF)
           { (*CommandLineData(theEnv)->EventCallback)(theEnv); }
         else
           { ExpandCommandString(theEnv,(char) inchar); }
        }
      else
        { (*CommandLineData(theEnv)->EventCallback)(theEnv); }

      /*=================================================*/
      /* If execution was halted, then remove everything */
      /* from the command buffer.                        */
      /*=================================================*/

      if (GetHaltExecution(theEnv) == true)
        {
         SetHaltExecution(theEnv,false);
         SetEvaluationError(theEnv,false);
         FlushCommandString(theEnv);
         WriteString(theEnv,STDOUT,"\n");
         PrintPrompt(theEnv);
        }

      /*=========================================*/
      /* If a complete command is in the command */
      /* buffer, then execute it.                */
      /*=========================================*/

      ExecuteIfCommandComplete(theEnv);
     }
  }

/***********************************************************/
/* CommandLoopBatch: Loop which waits for commands from a  */
/*   batch file and then executes them. Returns when there */
/*   are no longer any active batch files.                 */
/***********************************************************/
void CommandLoopBatch(
  Environment *theEnv)
  {
   SetHaltExecution(theEnv,false);
   SetEvaluationError(theEnv,false);

   CleanCurrentGarbageFrame(theEnv,NULL);
   CallPeriodicTasks(theEnv);

   PrintPrompt(theEnv);
   RouterData(theEnv)->CommandBufferInputCount = 0;
   RouterData(theEnv)->InputUngets = 0;
   RouterData(theEnv)->AwaitingInput = true;

   CommandLoopBatchDriver(theEnv);
  }

/************************************************************/
/* CommandLoopOnceThenBatch: Loop which waits for commands  */
/*   from a batch file and then executes them. Returns when */
/*   there are no longer any active batch files.            */
/************************************************************/
void CommandLoopOnceThenBatch(
  Environment *theEnv)
  {
   if (! ExecuteIfCommandComplete(theEnv)) return;

   CommandLoopBatchDriver(theEnv);
  }

/*********************************************************/
/* CommandLoopBatchDriver: Loop which waits for commands */
/*   from a batch file and then executes them. Returns   */
/*   when there are no longer any active batch files.    */
/*********************************************************/
void CommandLoopBatchDriver(
  Environment *theEnv)
  {
   int inchar;

   while (true)
     {
      if (GetHaltCommandLoopBatch(theEnv) == true)
        {
         CloseAllBatchSources(theEnv);
         SetHaltCommandLoopBatch(theEnv,false);
        }

      /*===================================================*/
      /* If a batch file is active, grab the command input */
      /* directly from the batch file, otherwise call the  */
      /* event function.                                   */
      /*===================================================*/

      if (BatchActive(theEnv) == true)
        {
         inchar = LLGetcBatch(theEnv,STDIN,true);
         if (inchar == EOF)
           { return; }
         else
           { ExpandCommandString(theEnv,(char) inchar); }
        }
      else
        { return; }

      /*=================================================*/
      /* If execution was halted, then remove everything */
      /* from the command buffer.                        */
      /*=================================================*/

      if (GetHaltExecution(theEnv) == true)
        {
         SetHaltExecution(theEnv,false);
         SetEvaluationError(theEnv,false);
         FlushCommandString(theEnv);
         WriteString(theEnv,STDOUT,"\n");
         PrintPrompt(theEnv);
        }

      /*=========================================*/
      /* If a complete command is in the command */
      /* buffer, then execute it.                */
      /*=========================================*/

      ExecuteIfCommandComplete(theEnv);
     }
  }

/**********************************************************/
/* ExecuteIfCommandComplete: Checks to determine if there */
/*   is a completed command and if so executes it.        */
/**********************************************************/
bool ExecuteIfCommandComplete(
  Environment *theEnv)
  {
   if ((CompleteCommand(CommandLineData(theEnv)->CommandString) == 0) ||
       (RouterData(theEnv)->CommandBufferInputCount == 0) ||
       (RouterData(theEnv)->AwaitingInput == false))
     { return false; }

   if (CommandLineData(theEnv)->BeforeCommandExecutionCallback != NULL)
     {
      if (! (*CommandLineData(theEnv)->BeforeCommandExecutionCallback)(theEnv))
        { return false; }
     }

   FlushPPBuffer(theEnv);
   SetPPBufferStatus(theEnv,false);
   RouterData(theEnv)->CommandBufferInputCount = 0;
   RouterData(theEnv)->InputUngets = 0;
   RouterData(theEnv)->AwaitingInput = false;
   RouteCommand(theEnv,CommandLineData(theEnv)->CommandString,true);
   FlushPPBuffer(theEnv);
#if (! BLOAD_ONLY)
   FlushParsingMessages(theEnv);
#endif
   SetHaltExecution(theEnv,false);
   SetEvaluationError(theEnv,false);
   FlushCommandString(theEnv);

   CleanCurrentGarbageFrame(theEnv,NULL);
   CallPeriodicTasks(theEnv);

   PrintPrompt(theEnv);

   return true;
  }

/*******************************/
/* CommandCompleteAndNotEmpty: */
/*******************************/
bool CommandCompleteAndNotEmpty(
  Environment *theEnv)
  {
   if ((CompleteCommand(CommandLineData(theEnv)->CommandString) == 0) ||
       (RouterData(theEnv)->CommandBufferInputCount == 0) ||
       (RouterData(theEnv)->AwaitingInput == false))
     { return false; }

   return true;
  }

/*******************************************/
/* PrintPrompt: Prints the command prompt. */
/*******************************************/
void PrintPrompt(
   Environment *theEnv)
   {
    WriteString(theEnv,STDOUT,COMMAND_PROMPT);

    if (CommandLineData(theEnv)->AfterPromptCallback != NULL)
      { (*CommandLineData(theEnv)->AfterPromptCallback)(theEnv); }
   }

/*****************************************/
/* PrintBanner: Prints the CLIPS banner. */
/*****************************************/
void PrintBanner(
   Environment *theEnv)
   {
    WriteString(theEnv,STDOUT,CommandLineData(theEnv)->BannerString);
   }

/************************************************/
/* SetAfterPromptFunction: Replaces the current */
/*   value of AfterPromptFunction.              */
/************************************************/
void SetAfterPromptFunction(
  Environment *theEnv,
  AfterPromptFunction *funptr)
  {
   CommandLineData(theEnv)->AfterPromptCallback = funptr;
  }

/***********************************************************/
/* SetBeforeCommandExecutionFunction: Replaces the current */
/*   value of BeforeCommandExecutionFunction.              */
/***********************************************************/
void SetBeforeCommandExecutionFunction(
  Environment *theEnv,
  BeforeCommandExecutionFunction *funptr)
  {
   CommandLineData(theEnv)->BeforeCommandExecutionCallback = funptr;
  }

/*********************************************************/
/* RouteCommand: Processes a completed command. Returns  */
/*   true if a command could be parsed, otherwise false. */
/*********************************************************/
bool RouteCommand(
  Environment *theEnv,
  const char *command,
  bool printResult)
  {
   UDFValue returnValue;
   struct expr *top;
   const char *commandName;
   struct token theToken;
   int danglingConstructs;

   if (command == NULL)
     { return false; }

   /*========================================*/
   /* Open a string input source and get the */
   /* first token from that source.          */
   /*========================================*/

   OpenStringSource(theEnv,"command",command,0);

   GetToken(theEnv,"command",&theToken);

   /*=====================*/
   /* Evaluate constants. */
   /*=====================*/

   if ((theToken.tknType == SYMBOL_TOKEN) || (theToken.tknType == STRING_TOKEN) ||
       (theToken.tknType == FLOAT_TOKEN) || (theToken.tknType == INTEGER_TOKEN) ||
       (theToken.tknType == INSTANCE_NAME_TOKEN))
     {
      CloseStringSource(theEnv,"command");
      if (printResult)
        {
         PrintAtom(theEnv,STDOUT,TokenTypeToType(theToken.tknType),theToken.value);
         WriteString(theEnv,STDOUT,"\n");
        }
      return true;
     }

   /*=====================*/
   /* Evaluate variables. */
   /*=====================*/

   if ((theToken.tknType == GBL_VARIABLE_TOKEN) ||
       (theToken.tknType == MF_GBL_VARIABLE_TOKEN) ||
       (theToken.tknType == SF_VARIABLE_TOKEN) ||
       (theToken.tknType == MF_VARIABLE_TOKEN))
     {
      CloseStringSource(theEnv,"command");
      top = GenConstant(theEnv,TokenTypeToType(theToken.tknType),theToken.value);
      EvaluateExpression(theEnv,top,&returnValue);
      rtn_struct(theEnv,expr,top);
      if (printResult)
        {
         WriteUDFValue(theEnv,STDOUT,&returnValue);
         WriteString(theEnv,STDOUT,"\n");
        }
      return true;
     }

   /*========================================================*/
   /* If the next token isn't the beginning left parenthesis */
   /* of a command or construct, then whatever was entered   */
   /* cannot be evaluated at the command prompt.             */
   /*========================================================*/

   if (theToken.tknType != LEFT_PARENTHESIS_TOKEN)
     {
      PrintErrorID(theEnv,"COMMLINE",1,false);
      WriteString(theEnv,STDERR,"Expected a '(', constant, or variable.\n");
      CloseStringSource(theEnv,"command");
      return false;
     }

   /*===========================================================*/
   /* The next token must be a function name or construct type. */
   /*===========================================================*/

   GetToken(theEnv,"command",&theToken);
   if (theToken.tknType != SYMBOL_TOKEN)
     {
      PrintErrorID(theEnv,"COMMLINE",2,false);
      WriteString(theEnv,STDERR,"Expected a command.\n");
      CloseStringSource(theEnv,"command");
      return false;
     }

   commandName = theToken.lexemeValue->contents;

   /*======================*/
   /* Evaluate constructs. */
   /*======================*/

#if (! RUN_TIME) && (! BLOAD_ONLY)
   {
    BuildError errorFlag;

    errorFlag = ParseConstruct(theEnv,commandName,"command");
    if (errorFlag != BE_CONSTRUCT_NOT_FOUND_ERROR)
      {
       CloseStringSource(theEnv,"command");
       if (errorFlag == BE_PARSING_ERROR)
         {
          WriteString(theEnv,STDERR,"\nERROR:\n");
          WriteString(theEnv,STDERR,GetPPBuffer(theEnv));
          WriteString(theEnv,STDERR,"\n");
         }
       DestroyPPBuffer(theEnv);
       
       SetWarningFileName(theEnv,NULL);
       SetErrorFileName(theEnv,NULL);

       if (errorFlag == BE_NO_ERROR) return true;
       else return false;
      }
   }
#endif

   /*========================*/
   /* Parse a function call. */
   /*========================*/

   danglingConstructs = ConstructData(theEnv)->DanglingConstructs;
   CommandLineData(theEnv)->ParsingTopLevelCommand = true;
   top = Function2Parse(theEnv,"command",commandName);
   CommandLineData(theEnv)->ParsingTopLevelCommand = false;
   ClearParsedBindNames(theEnv);

   /*================================*/
   /* Close the string input source. */
   /*================================*/

   CloseStringSource(theEnv,"command");

   /*=========================*/
   /* Evaluate function call. */
   /*=========================*/

   if (top == NULL)
     {
      SetWarningFileName(theEnv,NULL);
      SetErrorFileName(theEnv,NULL);
      ConstructData(theEnv)->DanglingConstructs = danglingConstructs;
      return false;
     }

   ExpressionInstall(theEnv,top);

   CommandLineData(theEnv)->EvaluatingTopLevelCommand = true;
   CommandLineData(theEnv)->CurrentCommand = top;
   EvaluateExpression(theEnv,top,&returnValue);
   CommandLineData(theEnv)->CurrentCommand = NULL;
   CommandLineData(theEnv)->EvaluatingTopLevelCommand = false;

   ExpressionDeinstall(theEnv,top);
   ReturnExpression(theEnv,top);
   ConstructData(theEnv)->DanglingConstructs = danglingConstructs;
   
   SetWarningFileName(theEnv,NULL);
   SetErrorFileName(theEnv,NULL);

   /*=================================================*/
   /* Print the return value of the function/command. */
   /*=================================================*/

   if ((returnValue.header->type != VOID_TYPE) && printResult)
     {
      WriteUDFValue(theEnv,STDOUT,&returnValue);
      WriteString(theEnv,STDOUT,"\n");
     }

   return true;
  }

/*****************************************************************/
/* DefaultGetNextEvent: Default event-handling function. Handles */
/*   only keyboard events by first calling ReadRouter to get a   */
/*   character and then calling ExpandCommandString to add the   */
/*   character to the CommandString.                             */
/*****************************************************************/
static void DefaultGetNextEvent(
  Environment *theEnv)
  {
   int inchar;

   inchar = ReadRouter(theEnv,STDIN);

   if (inchar == EOF) inchar = '\n';

   ExpandCommandString(theEnv,(char) inchar);
  }

/*************************************/
/* SetEventFunction: Replaces the    */
/*   current value of EventFunction. */
/*************************************/
EventFunction *SetEventFunction(
  Environment *theEnv,
  EventFunction *theFunction)
  {
   EventFunction *tmp_ptr;

   tmp_ptr = CommandLineData(theEnv)->EventCallback;
   CommandLineData(theEnv)->EventCallback = theFunction;
   return tmp_ptr;
  }

/****************************************/
/* TopLevelCommand: Indicates whether a */
/*   top-level command is being parsed. */
/****************************************/
bool TopLevelCommand(
  Environment *theEnv)
  {
   return(CommandLineData(theEnv)->ParsingTopLevelCommand);
  }

/***********************************************************/
/* GetCommandCompletionString: Returns the last token in a */
/*   string if it is a valid token for command completion. */
/***********************************************************/
const char *GetCommandCompletionString(
  Environment *theEnv,
  const char *theString,
  size_t maxPosition)
  {
   struct token lastToken;
   struct token theToken;
   char lastChar;
   const char *rs;
   size_t length;

   /*=========================*/
   /* Get the command string. */
   /*=========================*/

   if (theString == NULL) return("");

   /*=========================================================================*/
   /* If the last character in the command string is a space, character       */
   /* return, or quotation mark, then the command completion can be anything. */
   /*=========================================================================*/

   lastChar = theString[maxPosition - 1];
   if ((lastChar == ' ') || (lastChar == '"') ||
       (lastChar == '\t') || (lastChar == '\f') ||
       (lastChar == '\n') || (lastChar == '\r'))
     { return(""); }

   /*============================================*/
   /* Find the last token in the command string. */
   /*============================================*/

   OpenTextSource(theEnv,"CommandCompletion",theString,0,maxPosition);
   ScannerData(theEnv)->IgnoreCompletionErrors = true;
   GetToken(theEnv,"CommandCompletion",&theToken);
   CopyToken(&lastToken,&theToken);
   while (theToken.tknType != STOP_TOKEN)
     {
      CopyToken(&lastToken,&theToken);
      GetToken(theEnv,"CommandCompletion",&theToken);
     }
   CloseStringSource(theEnv,"CommandCompletion");
   ScannerData(theEnv)->IgnoreCompletionErrors = false;

   /*===============================================*/
   /* Determine if the last token can be completed. */
   /*===============================================*/

   if (lastToken.tknType == SYMBOL_TOKEN)
     {
      rs = lastToken.lexemeValue->contents;
      if (rs[0] == '[') return (&rs[1]);
      return lastToken.lexemeValue->contents;
     }
   else if (lastToken.tknType == SF_VARIABLE_TOKEN)
     { return lastToken.lexemeValue->contents; }
   else if (lastToken.tknType == MF_VARIABLE_TOKEN)
     { return lastToken.lexemeValue->contents; }
   else if ((lastToken.tknType == GBL_VARIABLE_TOKEN) ||
            (lastToken.tknType == MF_GBL_VARIABLE_TOKEN) ||
            (lastToken.tknType == INSTANCE_NAME_TOKEN))
     { return NULL; }
   else if (lastToken.tknType == STRING_TOKEN)
     {
      length = strlen(lastToken.lexemeValue->contents);
      return GetCommandCompletionString(theEnv,lastToken.lexemeValue->contents,length);
     }
   else if ((lastToken.tknType == FLOAT_TOKEN) ||
            (lastToken.tknType == INTEGER_TOKEN))
     { return NULL; }

   return("");
  }

/****************************************************************/
/* SetHaltCommandLoopBatch: Sets the HaltCommandLoopBatch flag. */
/****************************************************************/
void SetHaltCommandLoopBatch(
  Environment *theEnv,
  bool value)
  {
   CommandLineData(theEnv)->HaltCommandLoopBatch = value;
  }

/*******************************************************************/
/* GetHaltCommandLoopBatch: Returns the HaltCommandLoopBatch flag. */
/*******************************************************************/
bool GetHaltCommandLoopBatch(
  Environment *theEnv)
  {
   return(CommandLineData(theEnv)->HaltCommandLoopBatch);
  }

#endif

