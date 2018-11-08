   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/05/18             */
   /*                                                     */
   /*                I/O FUNCTIONS MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Contains the code for several I/O functions      */
/*   including printout, read, open, close, remove, rename,  */
/*   format, and readline.                                   */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*      Gary D. Riley                                        */
/*      Bebe Ly                                              */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Added the get-char, set-locale, and            */
/*            read-number functions.                         */
/*                                                           */
/*            Modified printing of floats in the format      */
/*            function to use the locale from the set-locale */
/*            function.                                      */
/*                                                           */
/*            Moved IllegalLogicalNameMessage function to    */
/*            argacces.c.                                    */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Removed the undocumented use of t in the       */
/*            printout command to perform the same function  */
/*            as crlf.                                       */
/*                                                           */
/*            Replaced EXT_IO and BASIC_IO compiler flags    */
/*            with IO_FUNCTIONS compiler flag.               */
/*                                                           */
/*            Added rb and ab and removed r+ modes for the   */
/*            open function.                                 */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Used gensprintf instead of sprintf.            */
/*                                                           */
/*            Added put-char function.                       */
/*                                                           */
/*            Added SetFullCRLF which allows option to       */
/*            specify crlf as \n or \r\n.                    */
/*                                                           */
/*            Added AwaitingInput flag.                      */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Added STDOUT and STDIN logical name            */
/*            definitions.                                   */
/*                                                           */
/*      6.40: Modified ReadTokenFromStdin to capture         */
/*            carriage returns in the input buffer so that   */
/*            input buffer count will accurately reflect     */
/*            the number of characters typed for GUI         */
/*            interfaces that support deleting carriage      */
/*            returns.                                       */
/*                                                           */
/*            Added Env prefix to GetEvaluationError and     */
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
/*            UDF redesign.                                  */
/*                                                           */
/*            Added print and println functions.             */
/*                                                           */
/*            Read function now returns symbols for tokens   */
/*            that are not primitive values.                 */
/*                                                           */
/*            Added unget-char function.                     */
/*                                                           */
/*            Added r+, w+, a+, rb+, wb+, and ab+ file       */
/*            access modes for the open function.            */
/*                                                           */
/*            Added flush, rewind, tell, seek, and chdir     */
/*            functions.                                     */
/*                                                           */
/*            Changed error return value of read, readline,  */
/*            and read-number functions to FALSE and added   */
/*            an error code for read.                        */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if IO_FUNCTIONS
#include <locale.h>
#include <stdlib.h>
#include <ctype.h>
#endif

#include <stdio.h>
#include <string.h>

#include "argacces.h"
#include "commline.h"
#include "constant.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "filertr.h"
#include "memalloc.h"
#include "miscfun.h"
#include "prntutil.h"
#include "router.h"
#include "scanner.h"
#include "strngrtr.h"
#include "sysdep.h"
#include "utility.h"

#include "iofun.h"

/***************/
/* DEFINITIONS */
/***************/

#define FORMAT_MAX 512
#define FLAG_MAX    80

/********************/
/* ENVIRONMENT DATA */
/********************/

#define IO_FUNCTION_DATA 64

struct IOFunctionData
  {
   CLIPSLexeme *locale;
   bool useFullCRLF;
  };

#define IOFunctionData(theEnv) ((struct IOFunctionData *) GetEnvironmentData(theEnv,IO_FUNCTION_DATA))

/****************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS  */
/****************************************/

#if IO_FUNCTIONS
   static void             ReadTokenFromStdin(Environment *,struct token *);
   static const char      *ControlStringCheck(UDFContext *,unsigned int);
   static char             FindFormatFlag(const char *,size_t *,char *,size_t);
   static const char      *PrintFormatFlag(UDFContext *,const char *,unsigned int,int);
   static char            *FillBuffer(Environment *,const char *,size_t *,size_t *);
   static void             ReadNumber(Environment *,const char *,struct token *,bool);
   static void             PrintDriver(UDFContext *,const char *,bool);
#endif

/**************************************/
/* IOFunctionDefinitions: Initializes */
/*   the I/O functions.               */
/**************************************/
void IOFunctionDefinitions(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,IO_FUNCTION_DATA,sizeof(struct IOFunctionData),NULL);

#if IO_FUNCTIONS
   IOFunctionData(theEnv)->useFullCRLF = false;
   IOFunctionData(theEnv)->locale = CreateSymbol(theEnv,setlocale(LC_ALL,NULL));
   IncrementLexemeCount(IOFunctionData(theEnv)->locale);
#endif

#if ! RUN_TIME
#if IO_FUNCTIONS
   AddUDF(theEnv,"printout","v",1,UNBOUNDED,"*;ldsyn",PrintoutFunction,"PrintoutFunction",NULL);
   AddUDF(theEnv,"print","v",0,UNBOUNDED,NULL,PrintFunction,"PrintFunction",NULL);
   AddUDF(theEnv,"println","v",0,UNBOUNDED,NULL,PrintlnFunction,"PrintlnFunction",NULL);
   AddUDF(theEnv,"read","synldfie",0,1,";ldsyn",ReadFunction,"ReadFunction",NULL);
   AddUDF(theEnv,"open","b",2,3,"*;sy;ldsyn;s",OpenFunction,"OpenFunction",NULL);
   AddUDF(theEnv,"close","b",0,1,"ldsyn",CloseFunction,"CloseFunction",NULL);
   AddUDF(theEnv,"flush","b",0,1,"ldsyn",FlushFunction,"FlushFunction",NULL);
   AddUDF(theEnv,"rewind","b",1,1,";ldsyn",RewindFunction,"RewindFunction",NULL);
   AddUDF(theEnv,"tell","lb",1,1,";ldsyn",TellFunction,"TellFunction",NULL);
   AddUDF(theEnv,"seek","b",3,3,";ldsyn;l;y",SeekFunction,"SeekFunction",NULL);
   AddUDF(theEnv,"get-char","l",0,1,";ldsyn",GetCharFunction,"GetCharFunction",NULL);
   AddUDF(theEnv,"unget-char","l",1,2,";ldsyn;l",UngetCharFunction,"UngetCharFunction",NULL);
   AddUDF(theEnv,"put-char","v",1,2,";ldsyn;l",PutCharFunction,"PutCharFunction",NULL);
   AddUDF(theEnv,"remove","b",1,1,"sy",RemoveFunction,"RemoveFunction",NULL);
   AddUDF(theEnv,"rename","b",2,2,"sy",RenameFunction,"RenameFunction",NULL);
   AddUDF(theEnv,"format","s",2,UNBOUNDED,"*;ldsyn;s",FormatFunction,"FormatFunction",NULL);
   AddUDF(theEnv,"readline","sy",0,1,";ldsyn",ReadlineFunction,"ReadlineFunction",NULL);
   AddUDF(theEnv,"set-locale","sy",0,1,";s",SetLocaleFunction,"SetLocaleFunction",NULL);
   AddUDF(theEnv,"read-number","syld",0,1,";ldsyn",ReadNumberFunction,"ReadNumberFunction",NULL);
   AddUDF(theEnv,"chdir","b",0,1,"sy",ChdirFunction,"ChdirFunction",NULL);
#endif
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }

#if IO_FUNCTIONS

/******************************************/
/* PrintoutFunction: H/L access routine   */
/*   for the printout function.           */
/******************************************/
void PrintoutFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;

   /*=====================================================*/
   /* Get the logical name to which output is to be sent. */
   /*=====================================================*/

   logicalName = GetLogicalName(context,STDOUT);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"printout");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      return;
     }

   /*============================================================*/
   /* Determine if any router recognizes the output destination. */
   /*============================================================*/

   if (strcmp(logicalName,"nil") == 0)
     { return; }
   else if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      return;
     }

   /*========================*/
   /* Call the print driver. */
   /*========================*/

   PrintDriver(context,logicalName,false);
  }

/*************************************/
/* PrintFunction: H/L access routine */
/*   for the print function.         */
/*************************************/
void PrintFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   PrintDriver(context,STDOUT,false);
  }

/*************************************/
/* PrintlnFunction: H/L access routine */
/*   for the println function.         */
/*************************************/
void PrintlnFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   PrintDriver(context,STDOUT,true);
  }

/*************************************************/
/* PrintDriver: Driver routine for the printout, */
/*   print, and println functions.               */
/*************************************************/
static void PrintDriver(
  UDFContext *context,
  const char *logicalName,
  bool endCRLF)
  {
   UDFValue theArg;
   Environment *theEnv = context->environment;

   /*==============================*/
   /* Print each of the arguments. */
   /*==============================*/

   while (UDFHasNextArgument(context))
     {
      if (! UDFNextArgument(context,ANY_TYPE_BITS,&theArg))
        { break; }

      if (EvaluationData(theEnv)->HaltExecution) break;

      switch(theArg.header->type)
        {
         case SYMBOL_TYPE:
           if (strcmp(theArg.lexemeValue->contents,"crlf") == 0)
             {
              if (IOFunctionData(theEnv)->useFullCRLF)
                { WriteString(theEnv,logicalName,"\r\n"); }
              else
                { WriteString(theEnv,logicalName,"\n"); }
             }
           else if (strcmp(theArg.lexemeValue->contents,"tab") == 0)
             { WriteString(theEnv,logicalName,"\t"); }
           else if (strcmp(theArg.lexemeValue->contents,"vtab") == 0)
             { WriteString(theEnv,logicalName,"\v"); }
           else if (strcmp(theArg.lexemeValue->contents,"ff") == 0)
             { WriteString(theEnv,logicalName,"\f"); }
           else
             { WriteString(theEnv,logicalName,theArg.lexemeValue->contents); }
           break;

         case STRING_TYPE:
           WriteString(theEnv,logicalName,theArg.lexemeValue->contents);
           break;

         default:
           WriteUDFValue(theEnv,logicalName,&theArg);
           break;
        }
     }

   if (endCRLF)
     {
      if (IOFunctionData(theEnv)->useFullCRLF)
        { WriteString(theEnv,logicalName,"\r\n"); }
      else
        { WriteString(theEnv,logicalName,"\n"); }
     }
  }

/*****************************************************/
/* SetFullCRLF: Set the flag which indicates whether */
/*   crlf is treated just as '\n' or '\r\n'.         */
/*****************************************************/
bool SetFullCRLF(
  Environment *theEnv,
  bool value)
  {
   bool oldValue = IOFunctionData(theEnv)->useFullCRLF;

   IOFunctionData(theEnv)->useFullCRLF = value;

   return(oldValue);
  }

/*************************************************************/
/* ReadFunction: H/L access routine for the read function.   */
/*************************************************************/
void ReadFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   struct token theToken;
   const char *logicalName = NULL;

   ClearErrorValue(theEnv);

   /*======================================================*/
   /* Determine the logical name from which input is read. */
   /*======================================================*/

   if (! UDFHasNextArgument(context))
     { logicalName = STDIN; }
   else
     {
      logicalName = GetLogicalName(context,STDIN);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"read");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         SetErrorValue(theEnv,&CreateSymbol(theEnv,"LOGICAL_NAME_ERROR")->header);
         returnValue->lexemeValue = FalseSymbol(theEnv);
         return;
        }
     }

   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      SetErrorValue(theEnv,&CreateSymbol(theEnv,"LOGICAL_NAME_ERROR")->header);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*=======================================*/
   /* Collect input into string if the read */
   /* source is stdin, else just get token. */
   /*=======================================*/

   if (strcmp(logicalName,STDIN) == 0)
     { ReadTokenFromStdin(theEnv,&theToken); }
   else
     { GetToken(theEnv,logicalName,&theToken); }

   /*====================================================*/
   /* Copy the token to the return value data structure. */
   /*====================================================*/

   if ((theToken.tknType == FLOAT_TOKEN) || (theToken.tknType == STRING_TOKEN) ||
#if OBJECT_SYSTEM
       (theToken.tknType == INSTANCE_NAME_TOKEN) ||
#endif
       (theToken.tknType == SYMBOL_TOKEN) || (theToken.tknType == INTEGER_TOKEN))
     { returnValue->value = theToken.value; }
   else if (theToken.tknType == STOP_TOKEN)
     {
      SetErrorValue(theEnv,&CreateSymbol(theEnv,"EOF")->header);
      returnValue->value = CreateSymbol(theEnv,"EOF");
     }
   else if (theToken.tknType == UNKNOWN_VALUE_TOKEN)
     {
      SetErrorValue(theEnv,&CreateSymbol(theEnv,"READ_ERROR")->header);
      returnValue->lexemeValue = FalseSymbol(theEnv);
     }
   else
     { returnValue->value = CreateSymbol(theEnv,theToken.printForm); }
  }

/********************************************************/
/* ReadTokenFromStdin: Special routine used by the read */
/*   function to read a token from standard input.      */
/********************************************************/
static void ReadTokenFromStdin(
  Environment *theEnv,
  struct token *theToken)
  {
   char *inputString;
   size_t inputStringSize;
   int inchar;

   /*===========================================*/
   /* Initialize the variables used for storing */
   /* the characters retrieved from stdin.      */
   /*===========================================*/

   inputString = NULL;
   RouterData(theEnv)->CommandBufferInputCount = 0;
   RouterData(theEnv)->InputUngets = 0;
   RouterData(theEnv)->AwaitingInput = true;
   inputStringSize = 0;

   /*=============================================*/
   /* Continue processing until a token is found. */
   /*=============================================*/

   theToken->tknType = STOP_TOKEN;
   while (theToken->tknType == STOP_TOKEN)
     {
      /*========================================================*/
      /* Continue reading characters until a carriage return is */
      /* entered or the user halts execution (usually with      */
      /* control-c). Waiting for the carriage return prevents   */
      /* the input from being prematurely parsed (such as when  */
      /* a space is entered after a symbol has been typed).     */
      /*========================================================*/

      inchar = ReadRouter(theEnv,STDIN);

      while ((inchar != '\n') && (inchar != '\r') && (inchar != EOF) &&
             (! GetHaltExecution(theEnv)))
        {
         inputString = ExpandStringWithChar(theEnv,inchar,inputString,&RouterData(theEnv)->CommandBufferInputCount,
                                            &inputStringSize,inputStringSize + 80);
         inchar = ReadRouter(theEnv,STDIN);
        }

      /*====================================================*/
      /* Add the final carriage return to the input buffer. */
      /*====================================================*/

      if  ((inchar == '\n') || (inchar == '\r'))
        {
         inputString = ExpandStringWithChar(theEnv,inchar,inputString,&RouterData(theEnv)->CommandBufferInputCount,
                                            &inputStringSize,inputStringSize + 80);
        }

      /*==================================================*/
      /* Open a string input source using the characters  */
      /* retrieved from stdin and extract the first token */
      /* contained in the string.                         */
      /*==================================================*/

      OpenStringSource(theEnv,"read",inputString,0);
      GetToken(theEnv,"read",theToken);
      CloseStringSource(theEnv,"read");

      /*===========================================*/
      /* Pressing control-c (or comparable action) */
      /* aborts the read function.                 */
      /*===========================================*/

      if (GetHaltExecution(theEnv))
        {
         SetErrorValue(theEnv,&CreateSymbol(theEnv,"READ_ERROR")->header);
         theToken->tknType = SYMBOL_TOKEN;
         theToken->value = FalseSymbol(theEnv);
        }

      /*====================================================*/
      /* Return the EOF symbol if the end of file for stdin */
      /* has been encountered. This typically won't occur,  */
      /* but is possible (for example by pressing control-d */
      /* in the UNIX operating system).                     */
      /*====================================================*/

      if ((theToken->tknType == STOP_TOKEN) && (inchar == EOF))
        {
         theToken->tknType = SYMBOL_TOKEN;
         theToken->value = CreateSymbol(theEnv,"EOF");
        }
     }

   if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
   
   RouterData(theEnv)->CommandBufferInputCount = 0;
   RouterData(theEnv)->InputUngets = 0;
   RouterData(theEnv)->AwaitingInput = false;
  }

/*************************************************************/
/* OpenFunction: H/L access routine for the open function.   */
/*************************************************************/
void OpenFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *fileName, *logicalName, *accessMode = NULL;
   UDFValue theArg;

   /*====================*/
   /* Get the file name. */
   /*====================*/

   if ((fileName = GetFileName(context)) == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*=======================================*/
   /* Get the logical name to be associated */
   /* with the opened file.                 */
   /*=======================================*/

   logicalName = GetLogicalName(context,NULL);
   if (logicalName == NULL)
     {
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      IllegalLogicalNameMessage(theEnv,"open");
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*==================================*/
   /* Check to see if the logical name */
   /* is already in use.               */
   /*==================================*/

   if (FindFile(theEnv,logicalName,NULL))
     {
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      PrintErrorID(theEnv,"IOFUN",2,false);
      WriteString(theEnv,STDERR,"Logical name '");
      WriteString(theEnv,STDERR,logicalName);
      WriteString(theEnv,STDERR,"' already in use.\n");
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*===========================*/
   /* Get the file access mode. */
   /*===========================*/

   if (! UDFHasNextArgument(context))
     { accessMode = "r"; }
   else
     {
      if (! UDFNextArgument(context,STRING_BIT,&theArg))
        { return; }
      accessMode = theArg.lexemeValue->contents;
     }

   /*=====================================*/
   /* Check for a valid file access mode. */
   /*=====================================*/

   if ((strcmp(accessMode,"r") != 0) &&
       (strcmp(accessMode,"r+") != 0) &&
       (strcmp(accessMode,"w") != 0) &&
       (strcmp(accessMode,"w+") != 0) &&
       (strcmp(accessMode,"a") != 0) &&
       (strcmp(accessMode,"a+") != 0) &&
       (strcmp(accessMode,"rb") != 0) &&
       (strcmp(accessMode,"r+b") != 0) &&
       (strcmp(accessMode,"rb+") != 0) &&
       (strcmp(accessMode,"wb") != 0) &&
       (strcmp(accessMode,"w+b") != 0) &&
       (strcmp(accessMode,"wb+") != 0) &&
       (strcmp(accessMode,"ab") != 0) &&
       (strcmp(accessMode,"a+b") != 0) &&
       (strcmp(accessMode,"ab+")))
     {
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      ExpectedTypeError1(theEnv,"open",3,"'file access mode string'");
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*================================================*/
   /* Open the named file and associate it with the  */
   /* specified logical name. Return TRUE if the     */
   /* file was opened successfully, otherwise FALSE. */
   /*================================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,OpenAFile(theEnv,fileName,accessMode,logicalName));
  }

/*************************************************************/
/* CloseFunction: H/L access routine for the close function. */
/*************************************************************/
void CloseFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;

   /*=====================================================*/
   /* If no arguments are specified, then close all files */
   /* opened with the open command. Return true if all    */
   /* files were closed successfully, otherwise false.    */
   /*=====================================================*/

   if (! UDFHasNextArgument(context))
     {
      returnValue->lexemeValue = CreateBoolean(theEnv,CloseAllFiles(theEnv));
      return;
     }

   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   logicalName = GetLogicalName(context,NULL);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"close");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*========================================================*/
   /* Close the file associated with the specified logical   */
   /* name. Return true if the file was closed successfully, */
   /* otherwise false.                                       */
   /*========================================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,CloseFile(theEnv,logicalName));
  }

/*************************************************************/
/* FlushFunction: H/L access routine for the flush function. */
/*************************************************************/
void FlushFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;

   /*=====================================================*/
   /* If no arguments are specified, then flush all files */
   /* opened with the open command. Return true if all    */
   /* files were flushed successfully, otherwise false.   */
   /*=====================================================*/

   if (! UDFHasNextArgument(context))
     {
      returnValue->lexemeValue = CreateBoolean(theEnv,FlushAllFiles(theEnv));
      return;
     }

   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   logicalName = GetLogicalName(context,NULL);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"flush");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*=========================================================*/
   /* Flush the file associated with the specified logical    */
   /* name. Return true if the file was flushed successfully, */
   /* otherwise false.                                        */
   /*=========================================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,FlushFile(theEnv,logicalName));
  }

/***************************************************************/
/* RewindFunction: H/L access routine for the rewind function. */
/***************************************************************/
void RewindFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;

   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   logicalName = GetLogicalName(context,NULL);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"flush");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
     
   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*=========================================================*/
   /* Rewind the file associated with the specified logical   */
   /* name. Return true if the file was rewound successfully, */
   /* otherwise false.                                        */
   /*=========================================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,RewindFile(theEnv,logicalName));
  }

/***********************************************************/
/* TellFunction: H/L access routine for the tell function. */
/***********************************************************/
void TellFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;
   long long rv;

   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   logicalName = GetLogicalName(context,NULL);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"tell");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
     
   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*===========================*/
   /* Return the file position. */
   /*===========================*/

   rv = TellFile(theEnv,logicalName);

   if (rv == LLONG_MIN)
     { returnValue->lexemeValue = FalseSymbol(theEnv); }
   else
     { returnValue->integerValue = CreateInteger(theEnv,rv); }
  }

/***********************************************************/
/* SeekFunction: H/L access routine for the seek function. */
/***********************************************************/
void SeekFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;
   UDFValue theArg;
   long offset;
   const char *seekCode;

   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   logicalName = GetLogicalName(context,NULL);
   if (logicalName == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"seek");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
     
   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
     
   /*=================*/
   /* Get the offset. */
   /*=================*/

   if (! UDFNextArgument(context,INTEGER_BIT,&theArg))
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
     
   offset = (long) theArg.integerValue->contents;

   /*====================*/
   /* Get the seek code. */
   /*====================*/

   if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
     
   seekCode = theArg.lexemeValue->contents;

   if (strcmp(seekCode,"seek-set") == 0)
     { returnValue->lexemeValue = CreateBoolean(theEnv,SeekFile(theEnv,logicalName,offset,SEEK_SET)); }
   else if (strcmp(seekCode,"seek-cur") == 0)
     { returnValue->lexemeValue = CreateBoolean(theEnv,SeekFile(theEnv,logicalName,offset,SEEK_CUR)); }
   else if (strcmp(seekCode,"seek-end") == 0)
     { returnValue->lexemeValue = CreateBoolean(theEnv,SeekFile(theEnv,logicalName,offset,SEEK_END)); }
   else
     {
      UDFInvalidArgumentMessage(context,
         "symbol with value seek-set, seek-cur, or seek-end");
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }
  }

/***************************************/
/* GetCharFunction: H/L access routine */
/*   for the get-char function.        */
/***************************************/
void GetCharFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *logicalName;
   
   /*================================*/
   /* Get the logical name argument. */
   /*================================*/

   if (! UDFHasNextArgument(context))
     { logicalName = STDIN; }
   else
     {
      logicalName = GetLogicalName(context,STDIN);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"get-char");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         returnValue->integerValue = CreateInteger(theEnv,-1);
         return;
        }
     }
     
   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->integerValue = CreateInteger(theEnv,-1);
      return;
     }

   if (strcmp(logicalName,STDIN) == 0)
     {
      if (RouterData(theEnv)->InputUngets > 0)
        {
         returnValue->integerValue = CreateInteger(theEnv,ReadRouter(theEnv,logicalName));
         RouterData(theEnv)->InputUngets--;
        }
      else
        {
         int theChar;
         
         RouterData(theEnv)->AwaitingInput = true;
         theChar = ReadRouter(theEnv,logicalName);
         RouterData(theEnv)->AwaitingInput = false;

         if (theChar == '\b')
           {
            if (RouterData(theEnv)->CommandBufferInputCount > 0)
              { RouterData(theEnv)->CommandBufferInputCount--; }
           }
         else
           { RouterData(theEnv)->CommandBufferInputCount++; }

         returnValue->integerValue = CreateInteger(theEnv,theChar);
        }
      
      return;
     }
      
   returnValue->integerValue = CreateInteger(theEnv,ReadRouter(theEnv,logicalName));
  }

/*****************************************/
/* UngetCharFunction: H/L access routine */
/*   for the unget-char function.        */
/*****************************************/
void UngetCharFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   unsigned int numberOfArguments;
   const char *logicalName;
   UDFValue theArg;
   long long theChar;

   numberOfArguments = UDFArgumentCount(context);

   /*=======================*/
   /* Get the logical name. */
   /*=======================*/

   if (numberOfArguments == 1)
     { logicalName = STDIN; }
   else
     {
      logicalName = GetLogicalName(context,STDIN);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"ungetc-char");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         returnValue->integerValue = CreateInteger(theEnv,-1);
         return;
        }
     }

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->integerValue = CreateInteger(theEnv,-1);
      return;
     }

   /*=============================*/
   /* Get the character to unget. */
   /*=============================*/

   if (! UDFNextArgument(context,INTEGER_BIT,&theArg))
     { return; }

   theChar = theArg.integerValue->contents;
   if (theChar == -1)
     {
      returnValue->integerValue = CreateInteger(theEnv,-1);
      return;
     }

   /*=======================*/
   /* Ungetc the character. */
   /*=======================*/

   if (strcmp(logicalName,STDIN) == 0)
     { RouterData(theEnv)->InputUngets++; }
   
   returnValue->integerValue = CreateInteger(theEnv,UnreadRouter(theEnv,logicalName,(int) theChar));
  }

/***************************************/
/* PutCharFunction: H/L access routine */
/*   for the put-char function.        */
/***************************************/
void PutCharFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   unsigned int numberOfArguments;
   const char *logicalName;
   UDFValue theArg;
   long long theChar;
   FILE *theFile;

   numberOfArguments = UDFArgumentCount(context);

   /*=======================*/
   /* Get the logical name. */
   /*=======================*/

   if (numberOfArguments == 1)
     { logicalName = STDOUT; }
   else
     {
      logicalName = GetLogicalName(context,STDOUT);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"put-char");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         return;
        }
     }

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      return;
     }

   /*===========================*/
   /* Get the character to put. */
   /*===========================*/

   if (! UDFNextArgument(context,INTEGER_BIT,&theArg))
     { return; }

   theChar = theArg.integerValue->contents;

   /*===================================================*/
   /* If the "fast load" option is being used, then the */
   /* logical name is actually a pointer to a file and  */
   /* we can bypass the router and directly output the  */
   /* value.                                            */
   /*===================================================*/

   theFile = FindFptr(theEnv,logicalName);
   if (theFile != NULL)
     { putc((int) theChar,theFile); }
  }

/****************************************/
/* RemoveFunction: H/L access routine   */
/*   for the remove function.           */
/****************************************/
void RemoveFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *theFileName;

   /*====================*/
   /* Get the file name. */
   /*====================*/

   if ((theFileName = GetFileName(context)) == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*==============================================*/
   /* Remove the file. Return true if the file was */
   /* sucessfully removed, otherwise false.        */
   /*==============================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,genremove(theEnv,theFileName));
  }

/****************************************/
/* RenameFunction: H/L access routine   */
/*   for the rename function.           */
/****************************************/
void RenameFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *oldFileName, *newFileName;

   /*===========================*/
   /* Check for the file names. */
   /*===========================*/

   if ((oldFileName = GetFileName(context)) == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   if ((newFileName = GetFileName(context)) == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*==============================================*/
   /* Rename the file. Return true if the file was */
   /* sucessfully renamed, otherwise false.        */
   /*==============================================*/

   returnValue->lexemeValue = CreateBoolean(theEnv,genrename(theEnv,oldFileName,newFileName));
  }

/*************************************/
/* ChdirFunction: H/L access routine */
/*   for the chdir function.         */
/*************************************/
void ChdirFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   const char *theFileName;
   int success;

   /*===============================================*/
   /* If called with no arguments, the return value */
   /* indicates whether chdir is supported.         */
   /*===============================================*/
   
   if (! UDFHasNextArgument(context))
     {
      if (genchdir(theEnv,NULL))
        { returnValue->lexemeValue = TrueSymbol(theEnv); }
      else
        { returnValue->lexemeValue = FalseSymbol(theEnv); }
        
      return;
     }

   /*====================*/
   /* Get the file name. */
   /*====================*/

   if ((theFileName = GetFileName(context)) == NULL)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*==================================================*/
   /* Change the directory. Return TRUE if successful, */
   /* FALSE if unsuccessful, and UNSUPPORTED if the    */
   /* chdir functionality is not implemented.          */
   /*==================================================*/

   success = genchdir(theEnv,theFileName);
   
   switch (success)
     {
      case 1:
        returnValue->lexemeValue = TrueSymbol(theEnv);
        break;
        
      case 0:
        returnValue->lexemeValue = FalseSymbol(theEnv);
        break;

      default:
        WriteString(theEnv,STDERR,"The chdir function is not supported on this system.\n");
        SetHaltExecution(theEnv,true);
        SetEvaluationError(theEnv,true);
        returnValue->lexemeValue = FalseSymbol(theEnv);
        break;
     }
  }

/****************************************/
/* FormatFunction: H/L access routine   */
/*   for the format function.           */
/****************************************/
void FormatFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   unsigned int argCount;
   size_t start_pos;
   const char *formatString;
   const char *logicalName;
   char formatFlagType;
   unsigned int f_cur_arg = 3;
   size_t form_pos = 0;
   char percentBuffer[FLAG_MAX];
   char *fstr = NULL;
   size_t fmaxm = 0;
   size_t fpos = 0;
   void *hptr;
   const char *theString;

   /*======================================*/
   /* Set default return value for errors. */
   /*======================================*/

   hptr = CreateString(theEnv,"");

   /*=========================================*/
   /* Format requires at least two arguments: */
   /* a logical name and a format string.     */
   /*=========================================*/

   argCount = UDFArgumentCount(context);

   /*========================================*/
   /* First argument must be a logical name. */
   /*========================================*/

   if ((logicalName = GetLogicalName(context,STDOUT)) == NULL)
     {
      IllegalLogicalNameMessage(theEnv,"format");
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->value = hptr;
      return;
     }

   if (strcmp(logicalName,"nil") == 0)
     { /* do nothing */ }
   else if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      returnValue->value = hptr;
      return;
     }

   /*=====================================================*/
   /* Second argument must be a string.  The appropriate  */
   /* number of arguments specified by the string must be */
   /* present in the argument list.                       */
   /*=====================================================*/

   if ((formatString = ControlStringCheck(context,argCount)) == NULL)
     {
      returnValue->value = hptr;
      return;
     }

   /*========================================*/
   /* Search the format string, printing the */
   /* format flags as they are encountered.  */
   /*========================================*/

   while (formatString[form_pos] != '\0')
     {
      if (formatString[form_pos] != '%')
        {
         start_pos = form_pos;
         while ((formatString[form_pos] != '%') &&
                (formatString[form_pos] != '\0'))
           { form_pos++; }
         fstr = AppendNToString(theEnv,&formatString[start_pos],fstr,form_pos-start_pos,&fpos,&fmaxm);
        }
      else
        {
		 form_pos++;
         formatFlagType = FindFormatFlag(formatString,&form_pos,percentBuffer,FLAG_MAX);
         if (formatFlagType != ' ')
           {
            if ((theString = PrintFormatFlag(context,percentBuffer,f_cur_arg,formatFlagType)) == NULL)
              {
               if (fstr != NULL) rm(theEnv,fstr,fmaxm);
               returnValue->value = hptr;
               return;
              }
            fstr = AppendToString(theEnv,theString,fstr,&fpos,&fmaxm);
            if (fstr == NULL)
              {
               returnValue->value = hptr;
               return;
              }
            f_cur_arg++;
           }
         else
           {
            fstr = AppendToString(theEnv,percentBuffer,fstr,&fpos,&fmaxm);
            if (fstr == NULL)
              {
               returnValue->value = hptr;
               return;
              }
           }
        }
     }

   if (fstr != NULL)
     {
      hptr = CreateString(theEnv,fstr);
      if (strcmp(logicalName,"nil") != 0) WriteString(theEnv,logicalName,fstr);
      rm(theEnv,fstr,fmaxm);
     }
   else
     { hptr = CreateString(theEnv,""); }

   returnValue->value = hptr;
  }

/*********************************************************************/
/* ControlStringCheck:  Checks the 2nd parameter which is the format */
/*   control string to see if there are enough matching arguments.   */
/*********************************************************************/
static const char *ControlStringCheck(
  UDFContext *context,
  unsigned int argCount)
  {
   UDFValue t_ptr;
   const char *str_array;
   char print_buff[FLAG_MAX];
   size_t i;
   unsigned int per_count;
   char formatFlag;
   Environment *theEnv = context->environment;

   if (! UDFNthArgument(context,2,STRING_BIT,&t_ptr))
     { return NULL; }

   per_count = 0;
   str_array = t_ptr.lexemeValue->contents;
   for (i = 0; str_array[i] != '\0' ; )
     {
      if (str_array[i] == '%')
        {
         i++;
         formatFlag = FindFormatFlag(str_array,&i,print_buff,FLAG_MAX);
         if (formatFlag == '-')
           {
            PrintErrorID(theEnv,"IOFUN",3,false);
            WriteString(theEnv,STDERR,"Invalid format flag \"");
            WriteString(theEnv,STDERR,print_buff);
            WriteString(theEnv,STDERR,"\" specified in format function.\n");
            SetEvaluationError(theEnv,true);
            return (NULL);
           }
         else if (formatFlag != ' ')
           { per_count++; }
        }
      else
        { i++; }
     }

   if ((per_count + 2) != argCount)
     {
      ExpectedCountError(theEnv,"format",EXACTLY,per_count+2);
      SetEvaluationError(theEnv,true);
      return (NULL);
     }

   return(str_array);
  }

/***********************************************/
/* FindFormatFlag:  This function searches for */
/*   a format flag in the format string.       */
/***********************************************/
static char FindFormatFlag(
  const char *formatString,
  size_t *a,
  char *formatBuffer,
  size_t bufferMax)
  {
   char inchar, formatFlagType;
   size_t copy_pos = 0;

   /*====================================================*/
   /* Set return values to the default value. A blank    */
   /* character indicates that no format flag was found  */
   /* which requires a parameter.                        */
   /*====================================================*/

   formatFlagType = ' ';

   /*=====================================================*/
   /* The format flags for carriage returns, line feeds,  */
   /* horizontal and vertical tabs, and the percent sign, */
   /* do not require a parameter.                         */
   /*=====================================================*/

   if (formatString[*a] == 'n')
     {
      gensprintf(formatBuffer,"\n");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == 'r')
     {
      gensprintf(formatBuffer,"\r");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == 't')
     {
      gensprintf(formatBuffer,"\t");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == 'v')
     {
      gensprintf(formatBuffer,"\v");
      (*a)++;
      return(formatFlagType);
     }
   else if (formatString[*a] == '%')
     {
      gensprintf(formatBuffer,"%%");
      (*a)++;
      return(formatFlagType);
     }

   /*======================================================*/
   /* Identify the format flag which requires a parameter. */
   /*======================================================*/

   formatBuffer[copy_pos++] = '%';
   formatBuffer[copy_pos] = '\0';
   while ((formatString[*a] != '%') &&
          (formatString[*a] != '\0') &&
          (copy_pos < (bufferMax - 5)))
     {
      inchar = formatString[*a];
      (*a)++;

      if ( (inchar == 'd') ||
           (inchar == 'o') ||
           (inchar == 'x') ||
           (inchar == 'u'))
        {
         formatFlagType = inchar;
         formatBuffer[copy_pos++] = 'l';
         formatBuffer[copy_pos++] = 'l';
         formatBuffer[copy_pos++] = inchar;
         formatBuffer[copy_pos] = '\0';
         return(formatFlagType);
        }
      else if ( (inchar == 'c') ||
                (inchar == 's') ||
                (inchar == 'e') ||
                (inchar == 'f') ||
                (inchar == 'g') )
        {
         formatBuffer[copy_pos++] = inchar;
         formatBuffer[copy_pos] = '\0';
         formatFlagType = inchar;
         return(formatFlagType);
        }

      /*=======================================================*/
      /* If the type hasn't been read, then this should be the */
      /* -M.N part of the format specification (where M and N  */
      /* are integers).                                        */
      /*=======================================================*/

      if ( (! isdigit(inchar)) &&
           (inchar != '.') &&
           (inchar != '-') )
        {
         formatBuffer[copy_pos++] = inchar;
         formatBuffer[copy_pos] = '\0';
         return('-');
        }

      formatBuffer[copy_pos++] = inchar;
      formatBuffer[copy_pos] = '\0';
     }

   return(formatFlagType);
  }

/**********************************************************************/
/* PrintFormatFlag:  Prints out part of the total format string along */
/*   with the argument for that part of the format string.            */
/**********************************************************************/
static const char *PrintFormatFlag(
  UDFContext *context,
  const char *formatString,
  unsigned int whichArg,
  int formatType)
  {
   UDFValue theResult;
   const char *theString;
   char *printBuffer;
   size_t theLength;
   CLIPSLexeme *oldLocale;
   Environment *theEnv = context->environment;

   /*=================*/
   /* String argument */
   /*=================*/

   switch (formatType)
     {
      case 's':
        if (! UDFNthArgument(context,whichArg,LEXEME_BITS,&theResult))
          { return(NULL); }
        theLength = strlen(formatString) + strlen(theResult.lexemeValue->contents) + 200;
        printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
        gensprintf(printBuffer,formatString,theResult.lexemeValue->contents);
        break;

      case 'c':
        UDFNthArgument(context,whichArg,ANY_TYPE_BITS,&theResult);
        if ((theResult.header->type == STRING_TYPE) ||
            (theResult.header->type == SYMBOL_TYPE))
          {
           theLength = strlen(formatString) + 200;
           printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
           gensprintf(printBuffer,formatString,theResult.lexemeValue->contents[0]);
          }
        else if (theResult.header->type == INTEGER_TYPE)
          {
           theLength = strlen(formatString) + 200;
           printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));
           gensprintf(printBuffer,formatString,(char) theResult.integerValue->contents);
          }
        else
          {
           ExpectedTypeError1(theEnv,"format",whichArg,"symbol, string, or integer");
           return NULL;
          }
        break;

      case 'd':
      case 'x':
      case 'o':
      case 'u':
        if (! UDFNthArgument(context,whichArg,NUMBER_BITS,&theResult))
          { return(NULL); }
        theLength = strlen(formatString) + 200;
        printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));

        oldLocale = CreateSymbol(theEnv,setlocale(LC_NUMERIC,NULL));
        setlocale(LC_NUMERIC,IOFunctionData(theEnv)->locale->contents);

        if (theResult.header->type == FLOAT_TYPE)
          { gensprintf(printBuffer,formatString,(long long) theResult.floatValue->contents); }
        else
          { gensprintf(printBuffer,formatString,theResult.integerValue->contents); }

        setlocale(LC_NUMERIC,oldLocale->contents);
        break;

      case 'f':
      case 'g':
      case 'e':
        if (! UDFNthArgument(context,whichArg,NUMBER_BITS,&theResult))
          { return(NULL); }
        theLength = strlen(formatString) + 200;
        printBuffer = (char *) gm2(theEnv,(sizeof(char) * theLength));

        oldLocale = CreateSymbol(theEnv,setlocale(LC_NUMERIC,NULL));

        setlocale(LC_NUMERIC,IOFunctionData(theEnv)->locale->contents);

        if (theResult.header->type == FLOAT_TYPE)
          { gensprintf(printBuffer,formatString,theResult.floatValue->contents); }
        else
          { gensprintf(printBuffer,formatString,(double) theResult.integerValue->contents); }

        setlocale(LC_NUMERIC,oldLocale->contents);

        break;

      default:
         WriteString(theEnv,STDERR," Error in format, the conversion character");
         WriteString(theEnv,STDERR," for formatted output is not valid\n");
         return NULL;
     }

   theString = CreateString(theEnv,printBuffer)->contents;
   rm(theEnv,printBuffer,sizeof(char) * theLength);
   return(theString);
  }

/******************************************/
/* ReadlineFunction: H/L access routine   */
/*   for the readline function.           */
/******************************************/
void ReadlineFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   char *buffer;
   size_t line_max = 0;
   const char *logicalName;

   if (! UDFHasNextArgument(context))
     { logicalName = STDIN; }
   else
     {
      logicalName = GetLogicalName(context,STDIN);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"readline");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         returnValue->lexemeValue = FalseSymbol(theEnv);
         return;
        }
     }

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   if (strcmp(logicalName,STDIN) == 0)
     {
      RouterData(theEnv)->CommandBufferInputCount = 0;
      RouterData(theEnv)->InputUngets = 0;
      RouterData(theEnv)->AwaitingInput = true;
   
      buffer = FillBuffer(theEnv,logicalName,&RouterData(theEnv)->CommandBufferInputCount,&line_max);
   
      RouterData(theEnv)->CommandBufferInputCount = 0;
      RouterData(theEnv)->InputUngets = 0;
      RouterData(theEnv)->AwaitingInput = false;
     }
   else
     {
      size_t currentPos = 0;
      
      buffer = FillBuffer(theEnv,logicalName,&currentPos,&line_max);
     }

   if (GetHaltExecution(theEnv))
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      if (buffer != NULL) rm(theEnv,buffer,sizeof (char) * line_max);
      return;
     }

   if (buffer == NULL)
     {
      returnValue->lexemeValue = CreateSymbol(theEnv,"EOF");
      return;
     }

   returnValue->lexemeValue = CreateString(theEnv,buffer);
   rm(theEnv,buffer,sizeof (char) * line_max);
   return;
  }

/*************************************************************/
/* FillBuffer: Read characters from a specified logical name */
/*   and places them into a buffer until a carriage return   */
/*   or end-of-file character is read.                       */
/*************************************************************/
static char *FillBuffer(
  Environment *theEnv,
  const char *logicalName,
  size_t *currentPosition,
  size_t *maximumSize)
  {
   int c;
   char *buf = NULL;

   /*================================*/
   /* Read until end of line or eof. */
   /*================================*/

   c = ReadRouter(theEnv,logicalName);

   if (c == EOF)
     { return NULL; }

   /*==================================*/
   /* Grab characters until cr or eof. */
   /*==================================*/

   while ((c != '\n') && (c != '\r') && (c != EOF) &&
          (! GetHaltExecution(theEnv)))
     {
      buf = ExpandStringWithChar(theEnv,c,buf,currentPosition,maximumSize,*maximumSize+80);
      c = ReadRouter(theEnv,logicalName);
     }

   /*==================*/
   /* Add closing EOS. */
   /*==================*/

   buf = ExpandStringWithChar(theEnv,EOS,buf,currentPosition,maximumSize,*maximumSize+80);
   return (buf);
  }

/*****************************************/
/* SetLocaleFunction: H/L access routine */
/*   for the set-locale function.        */
/*****************************************/
void SetLocaleFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;

   /*=================================*/
   /* If there are no arguments, just */
   /* return the current locale.      */
   /*=================================*/

   if (! UDFHasNextArgument(context))
     {
      returnValue->value = IOFunctionData(theEnv)->locale;
      return;
     }

   /*=================*/
   /* Get the locale. */
   /*=================*/

   if (! UDFFirstArgument(context,STRING_BIT,&theArg))
     { return; }

   /*=====================================*/
   /* Return the old value of the locale. */
   /*=====================================*/

   returnValue->value = IOFunctionData(theEnv)->locale;

   /*======================================================*/
   /* Change the value of the locale to the one specified. */
   /*======================================================*/

   ReleaseLexeme(theEnv,IOFunctionData(theEnv)->locale);
   IOFunctionData(theEnv)->locale = theArg.lexemeValue;
   IncrementLexemeCount(IOFunctionData(theEnv)->locale);
  }

/******************************************/
/* ReadNumberFunction: H/L access routine */
/*   for the read-number function.        */
/******************************************/
void ReadNumberFunction(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   struct token theToken;
   const char *logicalName = NULL;

   /*======================================================*/
   /* Determine the logical name from which input is read. */
   /*======================================================*/

   if (! UDFHasNextArgument(context))
     { logicalName = STDIN; }
   else
     {
      logicalName = GetLogicalName(context,STDIN);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"read");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         returnValue->lexemeValue = FalseSymbol(theEnv);
         return;
        }
     }

   /*============================================*/
   /* Check to see that the logical name exists. */
   /*============================================*/

   if (QueryRouters(theEnv,logicalName) == false)
     {
      UnrecognizedRouterMessage(theEnv,logicalName);
      SetHaltExecution(theEnv,true);
      SetEvaluationError(theEnv,true);
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   /*=======================================*/
   /* Collect input into string if the read */
   /* source is stdin, else just get token. */
   /*=======================================*/

   if (strcmp(logicalName,STDIN) == 0)
     {
      RouterData(theEnv)->CommandBufferInputCount = 0;
      RouterData(theEnv)->InputUngets = 0;
      RouterData(theEnv)->AwaitingInput = true;
      
      ReadNumber(theEnv,logicalName,&theToken,true);
      
      RouterData(theEnv)->CommandBufferInputCount = 0;
      RouterData(theEnv)->InputUngets = 0;
      RouterData(theEnv)->AwaitingInput = false;
     }
   else
     { ReadNumber(theEnv,logicalName,&theToken,false); }

   /*====================================================*/
   /* Copy the token to the return value data structure. */
   /*====================================================*/

   if ((theToken.tknType == FLOAT_TOKEN) || (theToken.tknType == STRING_TOKEN) ||
#if OBJECT_SYSTEM
       (theToken.tknType == INSTANCE_NAME_TOKEN) ||
#endif
       (theToken.tknType == SYMBOL_TOKEN) || (theToken.tknType == INTEGER_TOKEN))
     { returnValue->value = theToken.value; }
   else if (theToken.tknType == STOP_TOKEN)
     { returnValue->value = CreateSymbol(theEnv,"EOF"); }
   else if (theToken.tknType == UNKNOWN_VALUE_TOKEN)
     { returnValue->lexemeValue = FalseSymbol(theEnv); }
   else
     { returnValue->value = CreateString(theEnv,theToken.printForm); }

   return;
  }

/********************************************/
/* ReadNumber: Special routine used by the  */
/*   read-number function to read a number. */
/********************************************/
static void ReadNumber(
  Environment *theEnv,
  const char *logicalName,
  struct token *theToken,
  bool isStdin)
  {
   char *inputString;
   char *charPtr = NULL;
   size_t inputStringSize;
   int inchar;
   long long theLong;
   double theDouble;
   CLIPSLexeme *oldLocale;

   theToken->tknType = STOP_TOKEN;

   /*===========================================*/
   /* Initialize the variables used for storing */
   /* the characters retrieved from stdin.      */
   /*===========================================*/

   inputString = NULL;
   inputStringSize = 0;
   inchar = ReadRouter(theEnv,logicalName);

   /*====================================*/
   /* Skip whitespace before any number. */
   /*====================================*/

   while (isspace(inchar) && (inchar != EOF) &&
          (! GetHaltExecution(theEnv)))
     { inchar = ReadRouter(theEnv,logicalName); }

   /*=============================================================*/
   /* Continue reading characters until whitespace is found again */
   /* (for anything other than stdin) or a CR/LF (for stdin).     */
   /*=============================================================*/

   while ((((! isStdin) && (! isspace(inchar))) ||
          (isStdin && (inchar != '\n') && (inchar != '\r'))) &&
          (inchar != EOF) &&
          (! GetHaltExecution(theEnv)))
     {
      inputString = ExpandStringWithChar(theEnv,inchar,inputString,&RouterData(theEnv)->CommandBufferInputCount,
                                         &inputStringSize,inputStringSize + 80);
      inchar = ReadRouter(theEnv,logicalName);
     }

   /*===========================================*/
   /* Pressing control-c (or comparable action) */
   /* aborts the read-number function.          */
   /*===========================================*/

   if (GetHaltExecution(theEnv))
     {
      theToken->tknType = SYMBOL_TOKEN;
      theToken->value = FalseSymbol(theEnv);
      SetErrorValue(theEnv,&CreateSymbol(theEnv,"READ_ERROR")->header);
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      return;
     }

   /*====================================================*/
   /* Return the EOF symbol if the end of file for stdin */
   /* has been encountered. This typically won't occur,  */
   /* but is possible (for example by pressing control-d */
   /* in the UNIX operating system).                     */
   /*====================================================*/

   if (inchar == EOF)
     {
      theToken->tknType = SYMBOL_TOKEN;
      theToken->value = CreateSymbol(theEnv,"EOF");
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      return;
     }

   /*==================================================*/
   /* Open a string input source using the characters  */
   /* retrieved from stdin and extract the first token */
   /* contained in the string.                         */
   /*==================================================*/

   /*=======================================*/
   /* Change the locale so that numbers are */
   /* converted using the localized format. */
   /*=======================================*/

   oldLocale = CreateSymbol(theEnv,setlocale(LC_NUMERIC,NULL));
   setlocale(LC_NUMERIC,IOFunctionData(theEnv)->locale->contents);

   /*========================================*/
   /* Try to parse the number as a long. The */
   /* terminating character must either be   */
   /* white space or the string terminator.  */
   /*========================================*/

#if WIN_MVC
   theLong = _strtoi64(inputString,&charPtr,10);
#else
   theLong = strtoll(inputString,&charPtr,10);
#endif

   if ((charPtr != inputString) &&
       (isspace(*charPtr) || (*charPtr == '\0')))
     {
      theToken->tknType = INTEGER_TOKEN;
      theToken->value = CreateInteger(theEnv,theLong);
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      setlocale(LC_NUMERIC,oldLocale->contents);
      return;
     }

   /*==========================================*/
   /* Try to parse the number as a double. The */
   /* terminating character must either be     */
   /* white space or the string terminator.    */
   /*==========================================*/

   theDouble = strtod(inputString,&charPtr);
   if ((charPtr != inputString) &&
       (isspace(*charPtr) || (*charPtr == '\0')))
     {
      theToken->tknType = FLOAT_TOKEN;
      theToken->value = CreateFloat(theEnv,theDouble);
      if (inputStringSize > 0) rm(theEnv,inputString,inputStringSize);
      setlocale(LC_NUMERIC,oldLocale->contents);
      return;
     }

   /*============================================*/
   /* Restore the "C" locale so that any parsing */
   /* of numbers uses the C format.              */
   /*============================================*/

   setlocale(LC_NUMERIC,oldLocale->contents);

   /*=========================================*/
   /* Return "*** READ ERROR ***" to indicate */
   /* a number was not successfully parsed.   */
   /*=========================================*/

   theToken->tknType = SYMBOL_TOKEN;
   theToken->value = FalseSymbol(theEnv);
   SetErrorValue(theEnv,&CreateSymbol(theEnv,"READ_ERROR")->header);
  }

#endif

