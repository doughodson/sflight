   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/29/18             */
   /*                                                     */
   /*                PRINT UTILITY MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Utility routines for printing various items      */
/*   and messages.                                           */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Link error occurs for the SlotExistError       */
/*            function when OBJECT_SYSTEM is set to 0 in     */
/*            setup.h. DR0865                                */
/*                                                           */
/*            Added DataObjectToString function.             */
/*                                                           */
/*            Added SlotExistError function.                 */
/*                                                           */
/*      6.30: Support for long long integers.                */
/*                                                           */
/*            Support for DATA_OBJECT_ARRAY primitive.       */
/*                                                           */
/*            Support for typed EXTERNAL_ADDRESS_TYPE.       */
/*                                                           */
/*            Used gensprintf and genstrcat instead of       */
/*            sprintf and strcat.                            */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Added code for capturing errors/warnings.      */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Fixed linkage issue when BLOAD_ONLY compiler   */
/*            flag is set to 1.                              */
/*                                                           */
/*      6.31: Added additional error messages for retracted  */
/*            facts, deleted instances, and invalid slots.   */
/*                                                           */
/*            Added under/overflow error message.            */
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
/*            Removed DATA_OBJECT_ARRAY primitive type.      */
/*                                                           */
/*            File name/line count displayed for errors      */
/*            and warnings during load command.              */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <string.h>

#include "setup.h"

#include "argacces.h"
#include "constant.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "evaluatn.h"
#include "factmngr.h"
#include "inscom.h"
#include "insmngr.h"
#include "memalloc.h"
#include "multifun.h"
#include "router.h"
#include "scanner.h"
#include "strngrtr.h"
#include "symbol.h"
#include "sysdep.h"
#include "utility.h"

#include "prntutil.h"

/*****************************************************/
/* InitializePrintUtilityData: Allocates environment */
/*    data for print utility routines.               */
/*****************************************************/
void InitializePrintUtilityData(
  Environment *theEnv)
  {
   AllocateEnvironmentData(theEnv,PRINT_UTILITY_DATA,sizeof(struct printUtilityData),NULL);
  }

/************************************************************/
/* WriteFloat: Controls printout of floating point numbers. */
/************************************************************/
void WriteFloat(
  Environment *theEnv,
  const char *fileid,
  double number)
  {
   const char *theString;

   theString = FloatToString(theEnv,number);
   WriteString(theEnv,fileid,theString);
  }

/************************************************/
/* WriteInteger: Controls printout of integers. */
/************************************************/
void WriteInteger(
  Environment *theEnv,
  const char *logicalName,
  long long number)
  {
   char printBuffer[32];

   gensprintf(printBuffer,"%lld",number);
   WriteString(theEnv,logicalName,printBuffer);
  }

/*******************************************/
/* PrintUnsignedInteger: Controls printout */
/*   of unsigned integers.                 */
/*******************************************/
void PrintUnsignedInteger(
  Environment *theEnv,
  const char *logicalName,
  unsigned long long number)
  {
   char printBuffer[32];

   gensprintf(printBuffer,"%llu",number);
   WriteString(theEnv,logicalName,printBuffer);
  }

/**************************************/
/* PrintAtom: Prints an atomic value. */
/**************************************/
void PrintAtom(
  Environment *theEnv,
  const char *logicalName,
  unsigned short type,
  void *value)
  {
   CLIPSExternalAddress *theAddress;
   char buffer[20];

   switch (type)
     {
      case FLOAT_TYPE:
        WriteFloat(theEnv,logicalName,((CLIPSFloat *) value)->contents);
        break;
      case INTEGER_TYPE:
        WriteInteger(theEnv,logicalName,((CLIPSInteger *) value)->contents);
        break;
      case SYMBOL_TYPE:
        WriteString(theEnv,logicalName,((CLIPSLexeme *) value)->contents);
        break;
      case STRING_TYPE:
        if (PrintUtilityData(theEnv)->PreserveEscapedCharacters)
          { WriteString(theEnv,logicalName,StringPrintForm(theEnv,((CLIPSLexeme *) value)->contents)); }
        else
          {
           WriteString(theEnv,logicalName,"\"");
           WriteString(theEnv,logicalName,((CLIPSLexeme *) value)->contents);
           WriteString(theEnv,logicalName,"\"");
          }
        break;

      case EXTERNAL_ADDRESS_TYPE:
        theAddress = (CLIPSExternalAddress *) value;

        if (PrintUtilityData(theEnv)->AddressesToStrings) WriteString(theEnv,logicalName,"\"");

        if ((EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type] != NULL) &&
            (EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type]->longPrintFunction != NULL))
          { (*EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type]->longPrintFunction)(theEnv,logicalName,value); }
        else
          {
           WriteString(theEnv,logicalName,"<Pointer-");

           gensprintf(buffer,"%d-",theAddress->type);
           WriteString(theEnv,logicalName,buffer);

           gensprintf(buffer,"%p",((CLIPSExternalAddress *) value)->contents);
           WriteString(theEnv,logicalName,buffer);
           WriteString(theEnv,logicalName,">");
          }

        if (PrintUtilityData(theEnv)->AddressesToStrings) WriteString(theEnv,logicalName,"\"");
        break;

#if OBJECT_SYSTEM
      case INSTANCE_NAME_TYPE:
        WriteString(theEnv,logicalName,"[");
        WriteString(theEnv,logicalName,((CLIPSLexeme *) value)->contents);
        WriteString(theEnv,logicalName,"]");
        break;
#endif

      case VOID_TYPE:
        break;

      default:
        if (EvaluationData(theEnv)->PrimitivesArray[type] == NULL) break;
        if (EvaluationData(theEnv)->PrimitivesArray[type]->longPrintFunction == NULL)
          {
           WriteString(theEnv,logicalName,"<unknown atom type>");
           break;
          }
        (*EvaluationData(theEnv)->PrimitivesArray[type]->longPrintFunction)(theEnv,logicalName,value);
        break;
     }
  }

/**********************************************************/
/* PrintTally: Prints a tally count indicating the number */
/*   of items that have been displayed. Used by functions */
/*   such as list-defrules.                               */
/**********************************************************/
void PrintTally(
  Environment *theEnv,
  const char *logicalName,
  unsigned long long count,
  const char *singular,
  const char *plural)
  {
   if (count == 0) return;

   WriteString(theEnv,logicalName,"For a total of ");
   PrintUnsignedInteger(theEnv,logicalName,count);
   WriteString(theEnv,logicalName," ");

   if (count == 1) WriteString(theEnv,logicalName,singular);
   else WriteString(theEnv,logicalName,plural);

   WriteString(theEnv,logicalName,".\n");
  }

/********************************************/
/* PrintErrorID: Prints the module name and */
/*   error ID for an error message.         */
/********************************************/
void PrintErrorID(
  Environment *theEnv,
  const char *module,
  int errorID,
  bool printCR)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)
   FlushParsingMessages(theEnv);
   SetErrorFileName(theEnv,GetParsingFileName(theEnv));
   ConstructData(theEnv)->ErrLineNumber = GetLineCount(theEnv);
#endif
   if (printCR) WriteString(theEnv,STDERR,"\n");
   WriteString(theEnv,STDERR,"[");
   WriteString(theEnv,STDERR,module);
   WriteInteger(theEnv,STDERR,errorID);
   WriteString(theEnv,STDERR,"] ");

   /*==================================================*/
   /* Print the file name and line number if available */
   /* and there is no callback for errors/warnings.    */
   /*==================================================*/

#if (! RUN_TIME) && (! BLOAD_ONLY)
   if ((ConstructData(theEnv)->ParserErrorCallback == NULL) &&
       (GetLoadInProgress(theEnv) == true))
     {
      const char *fileName;

      fileName = GetParsingFileName(theEnv);
      if (fileName != NULL)
        {
         WriteString(theEnv,STDERR,fileName);
         WriteString(theEnv,STDERR,", Line ");
         WriteInteger(theEnv,STDERR,GetLineCount(theEnv));
         WriteString(theEnv,STDERR,": ");
        }
     }
#endif
  }

/**********************************************/
/* PrintWarningID: Prints the module name and */
/*   warning ID for a warning message.        */
/**********************************************/
void PrintWarningID(
  Environment *theEnv,
  const char *module,
  int warningID,
  bool printCR)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)
   FlushParsingMessages(theEnv);
   SetWarningFileName(theEnv,GetParsingFileName(theEnv));
   ConstructData(theEnv)->WrnLineNumber = GetLineCount(theEnv);
#endif
   if (printCR) WriteString(theEnv,STDWRN,"\n");
   WriteString(theEnv,STDWRN,"[");
   WriteString(theEnv,STDWRN,module);
   WriteInteger(theEnv,STDWRN,warningID);
   WriteString(theEnv,STDWRN,"] ");

   /*==================================================*/
   /* Print the file name and line number if available */
   /* and there is no callback for errors/warnings.    */
   /*==================================================*/

#if (! RUN_TIME) && (! BLOAD_ONLY)
   if ((ConstructData(theEnv)->ParserErrorCallback == NULL) &&
       (GetLoadInProgress(theEnv) == true))
     {
      const char *fileName;

      fileName = GetParsingFileName(theEnv);
      if (fileName != NULL)
        {
         WriteString(theEnv,STDERR,fileName);
         WriteString(theEnv,STDERR,", Line ");
         WriteInteger(theEnv,STDERR,GetLineCount(theEnv));
         WriteString(theEnv,STDERR,", ");
        }
     }
#endif

   WriteString(theEnv,STDWRN,"WARNING: ");
  }

/***************************************************/
/* CantFindItemErrorMessage: Generic error message */
/*  when an "item" can not be found.               */
/***************************************************/
void CantFindItemErrorMessage(
  Environment *theEnv,
  const char *itemType,
  const char *itemName,
  bool useQuotes)
  {
   PrintErrorID(theEnv,"PRNTUTIL",1,false);
   WriteString(theEnv,STDERR,"Unable to find ");
   WriteString(theEnv,STDERR,itemType);
   WriteString(theEnv,STDERR," ");
   if (useQuotes) WriteString(theEnv,STDERR,"'");
   WriteString(theEnv,STDERR,itemName);
   if (useQuotes) WriteString(theEnv,STDERR,"'");
   WriteString(theEnv,STDERR,".\n");
  }

/*****************************************************/
/* CantFindItemInFunctionErrorMessage: Generic error */
/*  message when an "item" can not be found.         */
/*****************************************************/
void CantFindItemInFunctionErrorMessage(
  Environment *theEnv,
  const char *itemType,
  const char *itemName,
  const char *func,
  bool useQuotes)
  {
   PrintErrorID(theEnv,"PRNTUTIL",1,false);
   WriteString(theEnv,STDERR,"Unable to find ");
   WriteString(theEnv,STDERR,itemType);
   WriteString(theEnv,STDERR," ");
   if (useQuotes) WriteString(theEnv,STDERR,"'");
   WriteString(theEnv,STDERR,itemName);
   if (useQuotes) WriteString(theEnv,STDERR,"'");
   WriteString(theEnv,STDERR," in function '");
   WriteString(theEnv,STDERR,func);
   WriteString(theEnv,STDERR,"'.\n");
  }

/*****************************************************/
/* CantDeleteItemErrorMessage: Generic error message */
/*  when an "item" can not be deleted.               */
/*****************************************************/
void CantDeleteItemErrorMessage(
  Environment *theEnv,
  const char *itemType,
  const char *itemName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",4,false);
   WriteString(theEnv,STDERR,"Unable to delete ");
   WriteString(theEnv,STDERR,itemType);
   WriteString(theEnv,STDERR," '");
   WriteString(theEnv,STDERR,itemName);
   WriteString(theEnv,STDERR,"'.\n");
  }

/****************************************************/
/* AlreadyParsedErrorMessage: Generic error message */
/*  when an "item" has already been parsed.         */
/****************************************************/
void AlreadyParsedErrorMessage(
  Environment *theEnv,
  const char *itemType,
  const char *itemName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",5,true);
   WriteString(theEnv,STDERR,"The ");
   if (itemType != NULL) WriteString(theEnv,STDERR,itemType);
   if (itemName != NULL)
     {
      WriteString(theEnv,STDERR,"'");
      WriteString(theEnv,STDERR,itemName);
      WriteString(theEnv,STDERR,"'");
     }
   WriteString(theEnv,STDERR," has already been parsed.\n");
  }

/*********************************************************/
/* SyntaxErrorMessage: Generalized syntax error message. */
/*********************************************************/
void SyntaxErrorMessage(
  Environment *theEnv,
  const char *location)
  {
   PrintErrorID(theEnv,"PRNTUTIL",2,true);
   WriteString(theEnv,STDERR,"Syntax Error");
   if (location != NULL)
     {
      WriteString(theEnv,STDERR,":  Check appropriate syntax for ");
      WriteString(theEnv,STDERR,location);
     }

   WriteString(theEnv,STDERR,".\n");
   SetEvaluationError(theEnv,true);
  }

/****************************************************/
/* LocalVariableErrorMessage: Generic error message */
/*  when a local variable is accessed by an "item"  */
/*  which can not access local variables.           */
/****************************************************/
void LocalVariableErrorMessage(
  Environment *theEnv,
  const char *byWhat)
  {
   PrintErrorID(theEnv,"PRNTUTIL",6,true);
   WriteString(theEnv,STDERR,"Local variables can not be accessed by ");
   WriteString(theEnv,STDERR,byWhat);
   WriteString(theEnv,STDERR,".\n");
  }

/******************************************/
/* SystemError: Generalized error message */
/*   for major internal errors.           */
/******************************************/
void SystemError(
  Environment *theEnv,
  const char *module,
  int errorID)
  {
   PrintErrorID(theEnv,"PRNTUTIL",3,true);

   WriteString(theEnv,STDERR,"\n*** ");
   WriteString(theEnv,STDERR,APPLICATION_NAME);
   WriteString(theEnv,STDERR," SYSTEM ERROR ***\n");

   WriteString(theEnv,STDERR,"ID = ");
   WriteString(theEnv,STDERR,module);
   WriteInteger(theEnv,STDERR,errorID);
   WriteString(theEnv,STDERR,"\n");

   WriteString(theEnv,STDERR,APPLICATION_NAME);
   WriteString(theEnv,STDERR," data structures are in an inconsistent or corrupted state.\n");
   WriteString(theEnv,STDERR,"This error may have occurred from errors in user defined code.\n");
   WriteString(theEnv,STDERR,"**************************\n");
  }

/*******************************************************/
/* DivideByZeroErrorMessage: Generalized error message */
/*   for when a function attempts to divide by zero.   */
/*******************************************************/
void DivideByZeroErrorMessage(
  Environment *theEnv,
  const char *functionName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",7,false);
   WriteString(theEnv,STDERR,"Attempt to divide by zero in '");
   WriteString(theEnv,STDERR,functionName);
   WriteString(theEnv,STDERR,"' function.\n");
  }

/********************************************************/
/* ArgumentOverUnderflowErrorMessage: Generalized error */
/*   message for an integer under or overflow.          */
/********************************************************/
void ArgumentOverUnderflowErrorMessage(
  Environment *theEnv,
  const char *functionName,
  bool error)
  {
   if (error)
     {
      PrintErrorID(theEnv,"PRNTUTIL",17,false);
      WriteString(theEnv,STDERR,"Over or underflow of long long integer in '");
      WriteString(theEnv,STDERR,functionName);
      WriteString(theEnv,STDERR,"' function.\n");
     }
   else
     {
      PrintWarningID(theEnv,"PRNTUTIL",17,false);
      WriteString(theEnv,STDWRN,"Over or underflow of long long integer in '");
      WriteString(theEnv,STDWRN,functionName);
      WriteString(theEnv,STDWRN,"' function.\n");
     }
  }

/*******************************************************/
/* FloatToString: Converts number to KB string format. */
/*******************************************************/
const char *FloatToString(
  Environment *theEnv,
  double number)
  {
   char floatString[40];
   int i;
   char x;
   CLIPSLexeme *thePtr;

   gensprintf(floatString,"%.15g",number);

   for (i = 0; (x = floatString[i]) != '\0'; i++)
     {
      if ((x == '.') || (x == 'e'))
        {
         thePtr = CreateString(theEnv,floatString);
         return thePtr->contents;
        }
     }

   genstrcat(floatString,".0");

   thePtr = CreateString(theEnv,floatString);
   return thePtr->contents;
  }

/*******************************************************************/
/* LongIntegerToString: Converts long integer to KB string format. */
/*******************************************************************/
const char *LongIntegerToString(
  Environment *theEnv,
  long long number)
  {
   char buffer[50];
   CLIPSLexeme *thePtr;

   gensprintf(buffer,"%lld",number);

   thePtr = CreateString(theEnv,buffer);
   return thePtr->contents;
  }

/******************************************************************/
/* DataObjectToString: Converts a UDFValue to KB string format. */
/******************************************************************/
const char *DataObjectToString(
  Environment *theEnv,
  UDFValue *theDO)
  {
   CLIPSLexeme *thePtr;
   const char *theString;
   char *newString;
   const char *prefix, *postfix;
   size_t length;
   CLIPSExternalAddress *theAddress;
   StringBuilder *theSB;
   
   char buffer[30];

   switch (theDO->header->type)
     {
      case MULTIFIELD_TYPE:
         prefix = "(";
         theString = ImplodeMultifield(theEnv,theDO)->contents;
         postfix = ")";
         break;

      case STRING_TYPE:
         prefix = "\"";
         theString = theDO->lexemeValue->contents;
         postfix = "\"";
         break;

      case INSTANCE_NAME_TYPE:
         prefix = "[";
         theString = theDO->lexemeValue->contents;
         postfix = "]";
         break;

      case SYMBOL_TYPE:
         return theDO->lexemeValue->contents;

      case FLOAT_TYPE:
         return(FloatToString(theEnv,theDO->floatValue->contents));

      case INTEGER_TYPE:
         return(LongIntegerToString(theEnv,theDO->integerValue->contents));

      case VOID_TYPE:
         return("");

#if OBJECT_SYSTEM
      case INSTANCE_ADDRESS_TYPE:
         if (theDO->instanceValue == &InstanceData(theEnv)->DummyInstance)
           { return("<Dummy Instance>"); }

         if (theDO->instanceValue->garbage)
           {
            prefix = "<Stale Instance-";
            theString = theDO->instanceValue->name->contents;
            postfix = ">";
           }
         else
           {
            prefix = "<Instance-";
            theString = GetFullInstanceName(theEnv,theDO->instanceValue)->contents;
            postfix = ">";
           }

        break;
#endif

      case EXTERNAL_ADDRESS_TYPE:
        theAddress = theDO->externalAddressValue;
        
        theSB = CreateStringBuilder(theEnv,30);

        OpenStringBuilderDestination(theEnv,"DOTS",theSB);

        if ((EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type] != NULL) &&
            (EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type]->longPrintFunction != NULL))
          { (*EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type]->longPrintFunction)(theEnv,"DOTS",theAddress); }
        else
          {
           WriteString(theEnv,"DOTS","<Pointer-");

           gensprintf(buffer,"%d-",theAddress->type);
           WriteString(theEnv,"DOTS",buffer);

           gensprintf(buffer,"%p",theAddress->contents);
           WriteString(theEnv,"DOTS",buffer);
           WriteString(theEnv,"DOTS",">");
          }

        thePtr = CreateString(theEnv,theSB->contents);
        SBDispose(theSB);

        CloseStringBuilderDestination(theEnv,"DOTS");
        return thePtr->contents;

#if DEFTEMPLATE_CONSTRUCT
      case FACT_ADDRESS_TYPE:
         if (theDO->factValue == &FactData(theEnv)->DummyFact)
           { return("<Dummy Fact>"); }

         gensprintf(buffer,"<Fact-%lld>",theDO->factValue->factIndex);
         thePtr = CreateString(theEnv,buffer);
         return thePtr->contents;
#endif

      default:
         return("UNK");
     }

   length = strlen(prefix) + strlen(theString) + strlen(postfix) + 1;
   newString = (char *) genalloc(theEnv,length);
   newString[0] = '\0';
   genstrcat(newString,prefix);
   genstrcat(newString,theString);
   genstrcat(newString,postfix);
   thePtr = CreateString(theEnv,newString);
   genfree(theEnv,newString,length);
   return thePtr->contents;
  }

/************************************************************/
/* SalienceInformationError: Error message for errors which */
/*   occur during the evaluation of a salience value.       */
/************************************************************/
void SalienceInformationError(
  Environment *theEnv,
  const char *constructType,
  const char *constructName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",8,true);
   WriteString(theEnv,STDERR,"This error occurred while evaluating the salience");
   if (constructName != NULL)
     {
      WriteString(theEnv,STDERR," for ");
      WriteString(theEnv,STDERR,constructType);
      WriteString(theEnv,STDERR," '");
      WriteString(theEnv,STDERR,constructName);
      WriteString(theEnv,STDERR,"'");
     }
   WriteString(theEnv,STDERR,".\n");
  }

/**********************************************************/
/* SalienceRangeError: Error message that is printed when */
/*   a salience value does not fall between the minimum   */
/*   and maximum salience values.                         */
/**********************************************************/
void SalienceRangeError(
  Environment *theEnv,
  int min,
  int max)
  {
   PrintErrorID(theEnv,"PRNTUTIL",9,true);
   WriteString(theEnv,STDERR,"Salience value out of range ");
   WriteInteger(theEnv,STDERR,min);
   WriteString(theEnv,STDERR," to ");
   WriteInteger(theEnv,STDERR,max);
   WriteString(theEnv,STDERR,".\n");
  }

/***************************************************************/
/* SalienceNonIntegerError: Error message that is printed when */
/*   a rule's salience does not evaluate to an integer.        */
/***************************************************************/
void SalienceNonIntegerError(
  Environment *theEnv)
  {
   PrintErrorID(theEnv,"PRNTUTIL",10,true);
   WriteString(theEnv,STDERR,"Salience value must be an integer value.\n");
  }

/****************************************************/
/* FactRetractedErrorMessage: Generic error message */
/*  when a fact has been retracted.                 */
/****************************************************/
void FactRetractedErrorMessage(
  Environment *theEnv,
  Fact *theFact)
  {
   char tempBuffer[20];
   
   PrintErrorID(theEnv,"PRNTUTIL",11,false);
   WriteString(theEnv,STDERR,"The fact ");
   gensprintf(tempBuffer,"f-%lld",theFact->factIndex);
   WriteString(theEnv,STDERR,tempBuffer);
   WriteString(theEnv,STDERR," has been retracted.\n");
  }

/****************************************************/
/* FactVarSlotErrorMessage1: Generic error message  */
/*   when a var/slot reference accesses a fact that */
/*   has been retracted.                            */
/****************************************************/
void FactVarSlotErrorMessage1(
  Environment *theEnv,
  Fact *theFact,
  const char *varSlot)
  {
   char tempBuffer[20];
   
   PrintErrorID(theEnv,"PRNTUTIL",12,false);
   
   WriteString(theEnv,STDERR,"The variable/slot reference ?");
   WriteString(theEnv,STDERR,varSlot);
   WriteString(theEnv,STDERR," cannot be resolved because the referenced fact ");
   gensprintf(tempBuffer,"f-%lld",theFact->factIndex);
   WriteString(theEnv,STDERR,tempBuffer);
   WriteString(theEnv,STDERR," has been retracted.\n");
  }

/****************************************************/
/* FactVarSlotErrorMessage2: Generic error message  */
/*   when a var/slot reference accesses an invalid  */
/*   slot.                                          */
/****************************************************/
void FactVarSlotErrorMessage2(
  Environment *theEnv,
  Fact *theFact,
  const char *varSlot)
  {
   char tempBuffer[20];
   
   PrintErrorID(theEnv,"PRNTUTIL",13,false);
   
   WriteString(theEnv,STDERR,"The variable/slot reference ?");
   WriteString(theEnv,STDERR,varSlot);
   WriteString(theEnv,STDERR," is invalid because the referenced fact ");
   gensprintf(tempBuffer,"f-%lld",theFact->factIndex);
   WriteString(theEnv,STDERR,tempBuffer);
   WriteString(theEnv,STDERR," does not contain the specified slot.\n");
  }

/******************************************************/
/* InvalidVarSlotErrorMessage: Generic error message  */
/*   when a var/slot reference accesses an invalid    */
/*   slot.                                            */
/******************************************************/
void InvalidVarSlotErrorMessage(
  Environment *theEnv,
  const char *varSlot)
  {
   PrintErrorID(theEnv,"PRNTUTIL",14,false);
   
   WriteString(theEnv,STDERR,"The variable/slot reference ?");
   WriteString(theEnv,STDERR,varSlot);
   WriteString(theEnv,STDERR," is invalid because slot names must be symbols.\n");
  }

/*******************************************************/
/* InstanceVarSlotErrorMessage1: Generic error message */
/*   when a var/slot reference accesses an instance    */
/*   that has been deleted.                            */
/*******************************************************/
void InstanceVarSlotErrorMessage1(
  Environment *theEnv,
  Instance *theInstance,
  const char *varSlot)
  {
   PrintErrorID(theEnv,"PRNTUTIL",15,false);
   
   WriteString(theEnv,STDERR,"The variable/slot reference ?");
   WriteString(theEnv,STDERR,varSlot);
   WriteString(theEnv,STDERR," cannot be resolved because the referenced instance [");
   WriteString(theEnv,STDERR,theInstance->name->contents);
   WriteString(theEnv,STDERR,"] has been deleted.\n");
  }
  
/************************************************/
/* InstanceVarSlotErrorMessage2: Generic error  */
/*   message when a var/slot reference accesses */
/*   an invalid slot.                           */
/************************************************/
void InstanceVarSlotErrorMessage2(
  Environment *theEnv,
  Instance *theInstance,
  const char *varSlot)
  {
   PrintErrorID(theEnv,"PRNTUTIL",16,false);
   
   WriteString(theEnv,STDERR,"The variable/slot reference ?");
   WriteString(theEnv,STDERR,varSlot);
   WriteString(theEnv,STDERR," is invalid because the referenced instance [");
   WriteString(theEnv,STDERR,theInstance->name->contents);
   WriteString(theEnv,STDERR,"] does not contain the specified slot.\n");
  }

/***************************************************/
/* SlotExistError: Prints out an appropriate error */
/*   message when a slot cannot be found for a     */
/*   function. Input to the function is the slot   */
/*   name and the function name.                   */
/***************************************************/
void SlotExistError(
  Environment *theEnv,
  const char *sname,
  const char *func)
  {
   PrintErrorID(theEnv,"INSFUN",3,false);
   WriteString(theEnv,STDERR,"No such slot '");
   WriteString(theEnv,STDERR,sname);
   WriteString(theEnv,STDERR,"' in function '");
   WriteString(theEnv,STDERR,func);
   WriteString(theEnv,STDERR,"'.\n");
   SetEvaluationError(theEnv,true);
  }
