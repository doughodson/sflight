   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/15/18             */
   /*                                                     */
   /*           MESSAGE-HANDLER PARSER FUNCTIONS          */
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
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            Static constraint checking is always enabled.  */
/*                                                           */
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM && (! BLOAD_ONLY) && (! RUN_TIME)

#include <string.h>

#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif

#include "classcom.h"
#include "classfun.h"
#include "constrct.h"
#include "cstrcpsr.h"
#include "cstrnchk.h"
#include "envrnmnt.h"
#include "exprnpsr.h"
#include "insfun.h"
#include "memalloc.h"
#include "modulutl.h"
#include "msgcom.h"
#include "msgfun.h"
#include "pprint.h"
#include "prccode.h"
#include "prntutil.h"
#include "router.h"
#include "scanner.h"
#include "strngrtr.h"
#include "sysdep.h"

#include "msgpsr.h"

/* =========================================
   *****************************************
                   CONSTANTS
   =========================================
   ***************************************** */
#define SELF_LEN         4
#define SELF_SLOT_REF   ':'

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static bool                    IsParameterSlotReference(Environment *,const char *);
   static int                     SlotReferenceVar(Environment *,Expression *,void *);
   static int                     BindSlotReference(Environment *,Expression *,void *);
   static SlotDescriptor         *CheckSlotReference(Environment *,Defclass *,int,void *,bool,Expression *);
   static void                    GenHandlerSlotReference(Environment *,Expression *,unsigned short,SlotDescriptor *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************************
  NAME         : ParseDefmessageHandler
  DESCRIPTION  : Parses a message-handler for a class of objects
  INPUTS       : The logical name of the input source
  RETURNS      : False if successful parse, true otherwise
  SIDE EFFECTS : Handler allocated and inserted into class
  NOTES        : H/L Syntax:

                 (defmessage-handler <class> <name> [<type>] [<comment>]
                    (<params>)
                    <action>*)

                 <params> ::= <var>* | <var>* $?<name>
 ***********************************************************************/
bool ParseDefmessageHandler(
  Environment *theEnv,
  const char *readSource)
  {
   Defclass *cls;
   CLIPSLexeme *cname, *mname, *wildcard;
   unsigned mtype = MPRIMARY;
   unsigned short min, max;
   unsigned short lvars;
   bool error;
   Expression *hndParams,*actions;
   DefmessageHandler *hnd;

   SetPPBufferStatus(theEnv,true);
   FlushPPBuffer(theEnv);
   SetIndentDepth(theEnv,3);
   SavePPBuffer(theEnv,"(defmessage-handler ");

#if BLOAD || BLOAD_AND_BSAVE
   if ((Bloaded(theEnv)) && (! ConstructData(theEnv)->CheckSyntaxMode))
     {
      CannotLoadWithBloadMessage(theEnv,"defmessage-handler");
      return true;
     }
#endif
   cname = GetConstructNameAndComment(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken,"defmessage-handler",
                                      NULL,NULL,"~",true,false,true,false);
   if (cname == NULL)
     return true;
   cls = LookupDefclassByMdlOrScope(theEnv,cname->contents);
   if (cls == NULL)
     {
      PrintErrorID(theEnv,"MSGPSR",1,false);
      WriteString(theEnv,STDERR,"A class must be defined before its message-handlers.\n");
      return true;
     }
   if ((cls == DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME_TYPE]) ||
       (cls == DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS_TYPE]) ||
       (cls == DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME_TYPE]->directSuperclasses.classArray[0]))
     {
      PrintErrorID(theEnv,"MSGPSR",8,false);
      WriteString(theEnv,STDERR,"Message-handlers cannot be attached to the class '");
      WriteString(theEnv,STDERR,DefclassName(cls));
      WriteString(theEnv,STDERR,"'.\n");
      return true;
     }
   if (HandlersExecuting(cls))
     {
      PrintErrorID(theEnv,"MSGPSR",2,false);
      WriteString(theEnv,STDERR,"Cannot (re)define message-handlers during execution of ");
      WriteString(theEnv,STDERR,"other message-handlers for the same class.\n");
      return true;
     }
   if (DefclassData(theEnv)->ObjectParseToken.tknType != SYMBOL_TOKEN)
     {
      SyntaxErrorMessage(theEnv,"defmessage-handler");
      return true;
     }
   PPBackup(theEnv);
   PPBackup(theEnv);
   SavePPBuffer(theEnv," ");
   SavePPBuffer(theEnv,DefclassData(theEnv)->ObjectParseToken.printForm);
   SavePPBuffer(theEnv," ");
   mname = DefclassData(theEnv)->ObjectParseToken.lexemeValue;
   GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
   if (DefclassData(theEnv)->ObjectParseToken.tknType != LEFT_PARENTHESIS_TOKEN)
     {
      SavePPBuffer(theEnv," ");
      if (DefclassData(theEnv)->ObjectParseToken.tknType != STRING_TOKEN)
        {
         if (DefclassData(theEnv)->ObjectParseToken.tknType != SYMBOL_TOKEN)
           {
            SyntaxErrorMessage(theEnv,"defmessage-handler");
            return true;
           }
         mtype = HandlerType(theEnv,"defmessage-handler",false,DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents);
         if (mtype == MERROR)
           return true;

         GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
         if (DefclassData(theEnv)->ObjectParseToken.tknType == STRING_TOKEN)
           {
            SavePPBuffer(theEnv," ");
            GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
           }
        }
      else
        {
         SavePPBuffer(theEnv," ");
         GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
        }
     }
   PPBackup(theEnv);
   PPBackup(theEnv);
   PPCRAndIndent(theEnv);
   SavePPBuffer(theEnv,DefclassData(theEnv)->ObjectParseToken.printForm);

   hnd = FindHandlerByAddress(cls,mname,mtype);
   if (GetPrintWhileLoading(theEnv) && GetCompilationsWatch(theEnv))
     {
      WriteString(theEnv,STDOUT,"   Handler ");
      WriteString(theEnv,STDOUT,mname->contents);
      WriteString(theEnv,STDOUT," ");
      WriteString(theEnv,STDOUT,MessageHandlerData(theEnv)->hndquals[mtype]);
      if (hnd == NULL)
        WriteString(theEnv,STDOUT," defined.\n");
      else
        WriteString(theEnv,STDOUT," redefined.\n");
     }

   if ((hnd != NULL) ? hnd->system : false)
     {
      PrintErrorID(theEnv,"MSGPSR",3,false);
      WriteString(theEnv,STDERR,"System message-handlers may not be modified.\n");
      return true;
     }

   hndParams = GenConstant(theEnv,SYMBOL_TYPE,MessageHandlerData(theEnv)->SELF_SYMBOL);
   hndParams = ParseProcParameters(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken,hndParams,
                                    &wildcard,&min,&max,&error,IsParameterSlotReference);
   if (error)
     return true;
   PPCRAndIndent(theEnv);
   ExpressionData(theEnv)->ReturnContext = true;
   actions = ParseProcActions(theEnv,"message-handler",readSource,
                              &DefclassData(theEnv)->ObjectParseToken,hndParams,wildcard,
                              SlotReferenceVar,BindSlotReference,&lvars,
                              cls);
   if (actions == NULL)
     {
      ReturnExpression(theEnv,hndParams);
      return true;
     }
   if (DefclassData(theEnv)->ObjectParseToken.tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      SyntaxErrorMessage(theEnv,"defmessage-handler");
      ReturnExpression(theEnv,hndParams);
      ReturnPackedExpression(theEnv,actions);
      return true;
     }
   PPBackup(theEnv);
   PPBackup(theEnv);
   SavePPBuffer(theEnv,DefclassData(theEnv)->ObjectParseToken.printForm);
   SavePPBuffer(theEnv,"\n");

   /* ===================================================
      If we're only checking syntax, don't add the
      successfully parsed defmessage-handler to the KB.
      =================================================== */

   if (ConstructData(theEnv)->CheckSyntaxMode)
     {
      ReturnExpression(theEnv,hndParams);
      ReturnPackedExpression(theEnv,actions);
      return false;
     }

   if (hnd != NULL)
     {
      ExpressionDeinstall(theEnv,hnd->actions);
      ReturnPackedExpression(theEnv,hnd->actions);
      if (hnd->header.ppForm != NULL)
        rm(theEnv,(void *) hnd->header.ppForm,
           (sizeof(char) * (strlen(hnd->header.ppForm)+1)));
     }
   else
     {
      hnd = InsertHandlerHeader(theEnv,cls,mname,mtype);
      IncrementLexemeCount(hnd->header.name);
     }
   ReturnExpression(theEnv,hndParams);

   hnd->minParams = min;
   hnd->maxParams = max;
   hnd->localVarCount = lvars;
   hnd->actions = actions;
   ExpressionInstall(theEnv,hnd->actions);
#if DEBUGGING_FUNCTIONS

   /* ===================================================
      Old handler trace status is automatically preserved
      =================================================== */
   if (GetConserveMemory(theEnv) == false)
     hnd->header.ppForm = CopyPPBuffer(theEnv);
   else
#endif
     hnd->header.ppForm = NULL;
   return false;
  }

/*******************************************************************************
  NAME         : CreateGetAndPutHandlers
  DESCRIPTION  : Creates two message-handlers with
                  the following syntax for the slot:

                 (defmessage-handler <class> get-<slot-name> primary ()
                    ?self:<slot-name>)

                 For single-field slots:

                 (defmessage-handler <class> put-<slot-name> primary (?value)
                    (bind ?self:<slot-name> ?value))

                 For multifield slots:

                 (defmessage-handler <class> put-<slot-name> primary ($?value)
                    (bind ?self:<slot-name> ?value))

  INPUTS       : The class slot descriptor
  RETURNS      : Nothing useful
  SIDE EFFECTS : Message-handlers created
  NOTES        : A put handler is not created for read-only slots
 *******************************************************************************/
void CreateGetAndPutHandlers(
  Environment *theEnv,
  SlotDescriptor *sd)
  {
   const char *className,*slotName;
   size_t bufsz;
   char *buf;
   const char *handlerRouter = "*** Default Public Handlers ***";
   bool oldPWL,oldCM;
   const char *oldRouter;
   const char *oldString;
   long oldIndex;

   if ((sd->createReadAccessor == 0) && (sd->createWriteAccessor == 0))
     return;
   className = sd->cls->header.name->contents;
   slotName = sd->slotName->name->contents;

   bufsz = (sizeof(char) * (strlen(className) + (strlen(slotName) * 2) + 80));
   buf = (char *) gm2(theEnv,bufsz);

   oldPWL = GetPrintWhileLoading(theEnv);
   SetPrintWhileLoading(theEnv,false);
   oldCM = SetConserveMemory(theEnv,true);

   if (sd->createReadAccessor)
     {
      gensprintf(buf,"%s get-%s () ?self:%s)",className,slotName,slotName);

      oldRouter = RouterData(theEnv)->FastCharGetRouter;
      oldString = RouterData(theEnv)->FastCharGetString;
      oldIndex = RouterData(theEnv)->FastCharGetIndex;

      RouterData(theEnv)->FastCharGetRouter = handlerRouter;
      RouterData(theEnv)->FastCharGetIndex = 0;
      RouterData(theEnv)->FastCharGetString = buf;

      ParseDefmessageHandler(theEnv,handlerRouter);
      DestroyPPBuffer(theEnv);
      /*
      if (OpenStringSource(theEnv,handlerRouter,buf,0))
        {
         ParseDefmessageHandler(handlerRouter);
         DestroyPPBuffer();
         CloseStringSource(theEnv,handlerRouter);
        }
      */
      RouterData(theEnv)->FastCharGetRouter = oldRouter;
      RouterData(theEnv)->FastCharGetIndex = oldIndex;
      RouterData(theEnv)->FastCharGetString = oldString;
     }

   if (sd->createWriteAccessor)
     {
      gensprintf(buf,"%s put-%s ($?value) (bind ?self:%s ?value))",
                  className,slotName,slotName);

      oldRouter = RouterData(theEnv)->FastCharGetRouter;
      oldString = RouterData(theEnv)->FastCharGetString;
      oldIndex = RouterData(theEnv)->FastCharGetIndex;

      RouterData(theEnv)->FastCharGetRouter = handlerRouter;
      RouterData(theEnv)->FastCharGetIndex = 0;
      RouterData(theEnv)->FastCharGetString = buf;

      ParseDefmessageHandler(theEnv,handlerRouter);
      DestroyPPBuffer(theEnv);

/*
      if (OpenStringSource(theEnv,handlerRouter,buf,0))
        {
         ParseDefmessageHandler(handlerRouter);
         DestroyPPBuffer();
         CloseStringSource(theEnv,handlerRouter);
        }
*/
      RouterData(theEnv)->FastCharGetRouter = oldRouter;
      RouterData(theEnv)->FastCharGetIndex = oldIndex;
      RouterData(theEnv)->FastCharGetString = oldString;
     }

   SetPrintWhileLoading(theEnv,oldPWL);
   SetConserveMemory(theEnv,oldCM);

   rm(theEnv,buf,bufsz);
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/*****************************************************************
  NAME         : IsParameterSlotReference
  DESCRIPTION  : Determines if a message-handler parameter is of
                 the form ?self:<name>, which is not allowed since
                 this is slot reference syntax
  INPUTS       : The paramter name
  RETURNS      : True if the parameter is a slot reference,
                 false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************************/
static bool IsParameterSlotReference(
  Environment *theEnv,
  const char *pname)
  {
   if ((strncmp(pname,SELF_STRING,SELF_LEN) == 0) ?
                  (pname[SELF_LEN] == SELF_SLOT_REF) : false)
     {
      PrintErrorID(theEnv,"MSGPSR",4,false);
      WriteString(theEnv,STDERR,"Illegal slot reference in parameter list.\n");
      return true;
     }
   return false;
  }

/****************************************************************************
  NAME         : SlotReferenceVar
  DESCRIPTION  : Replaces direct slot references in handler body
                   with special function calls to reference active instance
                   at run-time
                 The slot in in the class bound at parse-time is always
                   referenced (early binding).
                 Slot references of the form ?self:<name> directly reference
                   ProcParamArray[0] (the message object - ?self) to
                   find the specified slot at run-time
  INPUTS       : 1) Variable expression
                 2) The class of the handler being parsed
  RETURNS      : 0 if not recognized, 1 if so, -1 on errors
  SIDE EFFECTS : Handler body SF_VARIABLE and MF_VARIABLE replaced with
                   direct slot access function
  NOTES        : Objects are allowed to directly access their own slots
                 without sending a message to themselves.  Since the object
                 is "within the boundary of its internals", this does not
                 violate the encapsulation principle of OOP.
 ****************************************************************************/
static int SlotReferenceVar(
  Environment *theEnv,
  Expression *varexp,
  void *userBuffer)
  {
   struct token itkn;
   bool oldpp;
   SlotDescriptor *sd;

   if ((varexp->type != SF_VARIABLE) && (varexp->type != MF_VARIABLE))
     { return 0; }
   if ((strncmp(varexp->lexemeValue->contents,SELF_STRING,SELF_LEN) == 0) ?
               (varexp->lexemeValue->contents[SELF_LEN] == SELF_SLOT_REF) : false)
     {
      OpenStringSource(theEnv,"hnd-var",varexp->lexemeValue->contents + SELF_LEN + 1,0);
      oldpp = GetPPBufferStatus(theEnv);
      SetPPBufferStatus(theEnv,false);
      GetToken(theEnv,"hnd-var",&itkn);
      SetPPBufferStatus(theEnv,oldpp);
      CloseStringSource(theEnv,"hnd-var");
      if (itkn.tknType != STOP_TOKEN)
        {
         sd = CheckSlotReference(theEnv,(Defclass *) userBuffer,TokenTypeToType(itkn.tknType),itkn.value,
                                 false,NULL);
         if (sd == NULL)
           { return -1; }
         GenHandlerSlotReference(theEnv,varexp,HANDLER_GET,sd);
         return 1;
        }
     }

   return 0;
  }

/****************************************************************************
  NAME         : BindSlotReference
  DESCRIPTION  : Replaces direct slot binds in handler body with special
                 function calls to reference active instance at run-time
                 The slot in in the class bound at parse-time is always
                 referenced (early binding).
                 Slot references of the form ?self:<name> directly reference
                   ProcParamArray[0] (the message object - ?self) to
                   find the specified slot at run-time
  INPUTS       : 1) Variable expression
                 2) The class for the message-handler being parsed
  RETURNS      : 0 if not recognized, 1 if so, -1 on errors
  SIDE EFFECTS : Handler body "bind" call replaced with  direct slot access
                   function
  NOTES        : Objects are allowed to directly access their own slots
                 without sending a message to themselves.  Since the object
                 is "within the boundary of its internals", this does not
                 violate the encapsulation principle of OOP.
 ****************************************************************************/
static int BindSlotReference(
  Environment *theEnv,
  Expression *bindExp,
  void *userBuffer)
  {
   const char *bindName;
   struct token itkn;
   bool oldpp;
   SlotDescriptor *sd;
   Expression *saveExp;

   bindName = bindExp->argList->lexemeValue->contents;
   if (strcmp(bindName,SELF_STRING) == 0)
     {
      PrintErrorID(theEnv,"MSGPSR",5,false);
      WriteString(theEnv,STDERR,"Active instance parameter cannot be changed.\n");
      return -1;
     }
   if ((strncmp(bindName,SELF_STRING,SELF_LEN) == 0) ?
               (bindName[SELF_LEN] == SELF_SLOT_REF) : false)
     {
      OpenStringSource(theEnv,"hnd-var",bindName + SELF_LEN + 1,0);
      oldpp = GetPPBufferStatus(theEnv);
      SetPPBufferStatus(theEnv,false);
      GetToken(theEnv,"hnd-var",&itkn);
      SetPPBufferStatus(theEnv,oldpp);
      CloseStringSource(theEnv,"hnd-var");
      if (itkn.tknType != STOP_TOKEN)
        {
         saveExp = bindExp->argList->nextArg;
         sd = CheckSlotReference(theEnv,(Defclass *) userBuffer,TokenTypeToType(itkn.tknType),itkn.value,
                                 true,saveExp);
         if (sd == NULL)
           { return -1; }
         GenHandlerSlotReference(theEnv,bindExp,HANDLER_PUT,sd);
         bindExp->argList->nextArg = NULL;
         ReturnExpression(theEnv,bindExp->argList);
         bindExp->argList = saveExp;
         return 1;
        }
     }
   return 0;
  }

/*********************************************************
  NAME         : CheckSlotReference
  DESCRIPTION  : Examines a ?self:<slot-name> reference
                 If the reference is a single-field or
                 global variable, checking and evaluation
                 is delayed until run-time.  If the
                 reference is a symbol, this routine
                 verifies that the slot is a legal
                 slot for the reference (i.e., it exists
                 in the class to which the message-handler
                 is being attached, it is visible and it
                 is writable for write reference)
  INPUTS       : 1) A buffer holding the class
                    of the handler being parsed
                 2) The type of the slot reference
                 3) The value of the slot reference
                 4) A flag indicating if this is a read
                    or write access
                 5) Value expression for write
  RETURNS      : Class slot on success, NULL on errors
  SIDE EFFECTS : Messages printed on errors.
  NOTES        : For static references, this function
                 insures that the slot is either
                 publicly visible or that the handler
                 is being attached to the same class in
                 which the private slot is defined.
 *********************************************************/
static SlotDescriptor *CheckSlotReference(
  Environment *theEnv,
  Defclass *theDefclass,
  int theType,
  void *theValue,
  bool writeFlag,
  Expression *writeExpression)
  {
   int slotIndex;
   SlotDescriptor *sd;
   ConstraintViolationType vCode;

   if (theType != SYMBOL_TYPE)
     {
      PrintErrorID(theEnv,"MSGPSR",7,false);
      WriteString(theEnv,STDERR,"Illegal value for ?self reference.\n");
      return NULL;
     }
   slotIndex = FindInstanceTemplateSlot(theEnv,theDefclass,(CLIPSLexeme *) theValue);
   if (slotIndex == -1)
     {
      PrintErrorID(theEnv,"MSGPSR",6,false);
      WriteString(theEnv,STDERR,"No such slot '");
      WriteString(theEnv,STDERR,((CLIPSLexeme *) theValue)->contents);
      WriteString(theEnv,STDERR,"' in class '");
      WriteString(theEnv,STDERR,DefclassName(theDefclass));
      WriteString(theEnv,STDERR,"' for ?self reference.\n");
      return NULL;
     }
   sd = theDefclass->instanceTemplate[slotIndex];
   if ((sd->publicVisibility == 0) && (sd->cls != theDefclass))
     {
      SlotVisibilityViolationError(theEnv,sd,theDefclass,true);
      return NULL;
     }
   if (! writeFlag)
     return(sd);

   /* =================================================
      If a slot is initialize-only, the WithinInit flag
      still needs to be checked at run-time, for the
      handler could be called out of the context of
      an init.
      ================================================= */
   if (sd->noWrite && (sd->initializeOnly == 0))
     {
      SlotAccessViolationError(theEnv,((CLIPSLexeme *) theValue)->contents,
                               NULL,theDefclass);
      return NULL;
     }

   vCode = ConstraintCheckExpressionChain(theEnv,writeExpression,sd->constraint);
   if (vCode != NO_VIOLATION)
     {
      PrintErrorID(theEnv,"CSTRNCHK",1,false);
      WriteString(theEnv,STDERR,"Expression for ");
      PrintSlot(theEnv,STDERR,sd,NULL,"direct slot write");
      ConstraintViolationErrorMessage(theEnv,NULL,NULL,0,0,NULL,0,
                                      vCode,sd->constraint,false);
      return NULL;
     }
   return(sd);
  }

/***************************************************
  NAME         : GenHandlerSlotReference
  DESCRIPTION  : Creates a bitmap of the class id
                 and slot index for the get or put
                 operation. The bitmap and operation
                 type are stored in the given
                 expression.
  INPUTS       : 1) The expression
                 2) The operation type
                 3) The class slot
  RETURNS      : Nothing useful
  SIDE EFFECTS : Bitmap created and expression
                 initialized
  NOTES        : None
 ***************************************************/
static void GenHandlerSlotReference(
  Environment *theEnv,
  Expression *theExp,
  unsigned short theType,
  SlotDescriptor *sd)
  {
   HANDLER_SLOT_REFERENCE handlerReference;

   ClearBitString(&handlerReference,sizeof(HANDLER_SLOT_REFERENCE));
   handlerReference.classID = sd->cls->id;
   handlerReference.slotID = sd->slotName->id;
   theExp->type = theType;
   theExp->value =  AddBitMap(theEnv,&handlerReference,
                              sizeof(HANDLER_SLOT_REFERENCE));
  }

#endif
