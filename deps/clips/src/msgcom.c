   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/02/18             */
   /*                                                     */
   /*                OBJECT MESSAGE COMMANDS              */
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
/*      6.23: Changed name of variable log to logName        */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Removed IMPERATIVE_MESSAGE_HANDLERS            */
/*                    compilation flag.                      */
/*                                                           */
/*            Corrected code to remove run-time program      */
/*            compiler warnings.                             */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Added DeallocateMessageHandlerData to          */
/*            deallocate message handler environment data.   */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
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
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Pretty print functions accept optional logical */
/*            name argument.                                 */
/*                                                           */
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include <string.h>

#include "argacces.h"
#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif
#include "classcom.h"
#include "classfun.h"
#include "classinf.h"
#if (! BLOAD_ONLY) && (! RUN_TIME)
#include "constrct.h"
#include "msgpsr.h"
#endif
#include "envrnmnt.h"
#if ! RUN_TIME
#include "extnfunc.h"
#endif
#include "insfun.h"
#include "insmoddp.h"
#include "msgfun.h"
#include "msgpass.h"
#include "memalloc.h"
#include "prccode.h"
#include "prntutil.h"
#include "router.h"
#if DEBUGGING_FUNCTIONS
#include "watch.h"
#endif

#include "msgcom.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if ! RUN_TIME
   static void                    CreateSystemHandlers(Environment *,void *);
#endif

#if (! BLOAD_ONLY) && (! RUN_TIME)
   static bool                    WildDeleteHandler(Environment *,Defclass *,CLIPSLexeme *,const char *);
#endif

#if DEBUGGING_FUNCTIONS
   static bool                    DefmessageHandlerWatchAccess(Environment *,int,bool,Expression *);
   static bool                    DefmessageHandlerWatchPrint(Environment *,const char *,int,Expression *);
   static bool                    DefmessageHandlerWatchSupport(Environment *,const char *,const char *,bool,
                                                                void (*)(Environment *,const char *,Defclass *,unsigned),
                                                                void (*)(Defclass *,unsigned,bool),
                                                                Expression *);
   static bool                    WatchClassHandlers(Environment *,Defclass *,const char *,int,const char *,bool,bool,
                                                     void (*)(Environment *,const char *,Defclass *,unsigned),
                                                     void (*)(Defclass *,unsigned,bool));
   static void                    PrintHandlerWatchFlag(Environment *,const char *,Defclass *,unsigned);
#endif

   static void                    DeallocateMessageHandlerData(Environment *);

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************
  NAME         : SetupMessageHandlers
  DESCRIPTION  : Sets up internal symbols and
                 fucntion definitions pertaining to
                 message-handlers.  Also creates
                 system handlers
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Functions and data structures
                 initialized
  NOTES        : Should be called before
                 SetupInstanceModDupCommands() in
                 INSMODDP.C
 ***************************************************/
void SetupMessageHandlers(
  Environment *theEnv)
  {
   EntityRecord handlerGetInfo = { "HANDLER_GET", HANDLER_GET,0,1,1,
                                        PrintHandlerSlotGetFunction,
                                        PrintHandlerSlotGetFunction,NULL,
                                        HandlerSlotGetFunction,
                                        NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL },

                 handlerPutInfo = { "HANDLER_PUT", HANDLER_PUT,0,1,1,
                                        PrintHandlerSlotPutFunction,
                                        PrintHandlerSlotPutFunction,NULL,
                                        HandlerSlotPutFunction,
                                        NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

   AllocateEnvironmentData(theEnv,MESSAGE_HANDLER_DATA,sizeof(struct messageHandlerData),DeallocateMessageHandlerData);
   memcpy(&MessageHandlerData(theEnv)->HandlerGetInfo,&handlerGetInfo,sizeof(struct entityRecord));
   memcpy(&MessageHandlerData(theEnv)->HandlerPutInfo,&handlerPutInfo,sizeof(struct entityRecord));

   MessageHandlerData(theEnv)->hndquals[0] = "around";
   MessageHandlerData(theEnv)->hndquals[1] = "before";
   MessageHandlerData(theEnv)->hndquals[2] = "primary";
   MessageHandlerData(theEnv)->hndquals[3] = "after";

   InstallPrimitive(theEnv,&MessageHandlerData(theEnv)->HandlerGetInfo,HANDLER_GET);
   InstallPrimitive(theEnv,&MessageHandlerData(theEnv)->HandlerPutInfo,HANDLER_PUT);

#if ! RUN_TIME
   MessageHandlerData(theEnv)->INIT_SYMBOL = CreateSymbol(theEnv,INIT_STRING);
   IncrementLexemeCount(MessageHandlerData(theEnv)->INIT_SYMBOL);

   MessageHandlerData(theEnv)->DELETE_SYMBOL = CreateSymbol(theEnv,DELETE_STRING);
   IncrementLexemeCount(MessageHandlerData(theEnv)->DELETE_SYMBOL);

   MessageHandlerData(theEnv)->CREATE_SYMBOL = CreateSymbol(theEnv,CREATE_STRING);
   IncrementLexemeCount(MessageHandlerData(theEnv)->CREATE_SYMBOL);

   AddClearFunction(theEnv,"defclass",CreateSystemHandlers,-100,NULL);

#if ! BLOAD_ONLY
   MessageHandlerData(theEnv)->SELF_SYMBOL = CreateSymbol(theEnv,SELF_STRING);
   IncrementLexemeCount(MessageHandlerData(theEnv)->SELF_SYMBOL);

   AddConstruct(theEnv,"defmessage-handler","defmessage-handlers",
                ParseDefmessageHandler,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
   AddUDF(theEnv,"undefmessage-handler","v",2,3,"y",UndefmessageHandlerCommand,"UndefmessageHandlerCommand",NULL);

#endif

   AddUDF(theEnv,"send","*",2,UNBOUNDED,"*;*;y",SendCommand,"SendCommand",NULL);

#if DEBUGGING_FUNCTIONS
   AddUDF(theEnv,"preview-send","v",2,2,"y",PreviewSendCommand,"PreviewSendCommand",NULL);

   AddUDF(theEnv,"ppdefmessage-handler","v",2,4,"y",PPDefmessageHandlerCommand,"PPDefmessageHandlerCommand",NULL);
   AddUDF(theEnv,"list-defmessage-handlers","v",0,2,"y",ListDefmessageHandlersCommand,"ListDefmessageHandlersCommand",NULL);
#endif

   AddUDF(theEnv,"next-handlerp","b",0,0,NULL,NextHandlerAvailableFunction,"NextHandlerAvailableFunction",NULL);
   FuncSeqOvlFlags(theEnv,"next-handlerp",true,false);
   AddUDF(theEnv,"call-next-handler","*",0,0,NULL,CallNextHandler,"CallNextHandler",NULL);
   FuncSeqOvlFlags(theEnv,"call-next-handler",true,false);
   AddUDF(theEnv,"override-next-handler","*",0,UNBOUNDED,NULL,CallNextHandler,"CallNextHandler",NULL);
   FuncSeqOvlFlags(theEnv,"override-next-handler",true,false);

   AddUDF(theEnv,"dynamic-get","*",1,1,"y",DynamicHandlerGetSlot,"DynamicHandlerGetSlot",NULL);
   AddUDF(theEnv,"dynamic-put","*",1,UNBOUNDED,"*;y",DynamicHandlerPutSlot,"DynamicHandlerPutSlot",NULL);
   AddUDF(theEnv,"get","*",1,1,"y",DynamicHandlerGetSlot,"DynamicHandlerGetSlot",NULL);
   AddUDF(theEnv,"put","*",1,UNBOUNDED,"*;y",DynamicHandlerPutSlot,"DynamicHandlerPutSlot",NULL);
#endif

#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"messages",0,&MessageHandlerData(theEnv)->WatchMessages,36,NULL,NULL);
   AddWatchItem(theEnv,"message-handlers",0,&MessageHandlerData(theEnv)->WatchHandlers,35,
                DefmessageHandlerWatchAccess,DefmessageHandlerWatchPrint);
#endif
  }

/*******************************************************/
/* DeallocateMessageHandlerData: Deallocates environment */
/*    data for the message handler functionality.        */
/******************************************************/
static void DeallocateMessageHandlerData(
  Environment *theEnv)
  {
   HANDLER_LINK *tmp, *mhead, *chead;

   mhead = MessageHandlerData(theEnv)->TopOfCore;
   while (mhead != NULL)
     {
      tmp = mhead;
      mhead = mhead->nxt;
      rtn_struct(theEnv,messageHandlerLink,tmp);
     }

   chead = MessageHandlerData(theEnv)->OldCore;
   while (chead != NULL)
     {
      mhead = chead;
      chead = chead->nxtInStack;

      while (mhead != NULL)
        {
         tmp = mhead;
         mhead = mhead->nxt;
         rtn_struct(theEnv,messageHandlerLink,tmp);
        }
     }
  }

/*****************************************************
  NAME         : DefmessageHandlerName
  DESCRIPTION  : Gets the name of a message-handler
  INPUTS       : 1) Pointer to a class
                 2) Array index of handler in class's
                    message-handler array (+1)
  RETURNS      : Name-string of message-handler
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************/
const char *DefmessageHandlerName(
  Defclass *theDefclass,
  unsigned theIndex)
  {
   return theDefclass->handlers[theIndex-1].header.name->contents;
  }

/*****************************************************
  NAME         : DefmessageHandlerType
  DESCRIPTION  : Gets the type of a message-handler
  INPUTS       : 1) Pointer to a class
                 2) Array index of handler in class's
                    message-handler array (+1)
  RETURNS      : Type-string of message-handler
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************/
const char *DefmessageHandlerType(
  Defclass *theDefclass,
  unsigned theIndex)
  {
   Environment *theEnv = theDefclass->header.env;
   
   return MessageHandlerData(theEnv)->hndquals[theDefclass->handlers[theIndex-1].type];
  }

/**************************************************************
  NAME         : GetNextDefmessageHandler
  DESCRIPTION  : Finds first or next handler for a class
  INPUTS       : 1) The address of the handler's class
                 2) The array index of the current handler (+1)
  RETURNS      : The array index (+1) of the next handler, or 0
                   if there is none
  SIDE EFFECTS : None
  NOTES        : If index == 0, the first handler array index
                 (i.e. 1) returned
 **************************************************************/
unsigned GetNextDefmessageHandler(
  Defclass *theDefclass,
  unsigned theIndex)
  {
   if (theIndex == 0)
     { return (theDefclass->handlers != NULL) ? 1 : 0; }

   if (theIndex == theDefclass->handlerCount)
     { return 0; }

   return theIndex+1;
  }

/*****************************************************
  NAME         : GetDefmessageHandlerPointer
  DESCRIPTION  : Returns a pointer to a handler
  INPUTS       : 1) Pointer to a class
                 2) Array index of handler in class's
                    message-handler array (+1)
  RETURNS      : Pointer to the handler.
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************/
DefmessageHandler *GetDefmessageHandlerPointer(
  Defclass *theDefclass,
  unsigned int theIndex)
  {
   return &theDefclass->handlers[theIndex-1];
  }

#if DEBUGGING_FUNCTIONS

/*********************************************************
  NAME         : DefmessageHandlerGetWatch
  DESCRIPTION  : Determines if trace messages for calls
                 to this handler will be generated or not
  INPUTS       : 1) A pointer to the class
                 2) The index of the handler
  RETURNS      : True if a trace is active,
                 false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 *********************************************************/
bool DefmessageHandlerGetWatch(
  Defclass *theDefclass,
  unsigned theIndex)
  {
   return theDefclass->handlers[theIndex-1].trace;
  }

/*********************************************************
  NAME         : DefmessageHandlerSetWatch
  DESCRIPTION  : Sets the trace to ON/OFF for the
                 calling of the handler
  INPUTS       : 1) True to set the trace on,
                    false to set it off
                 2) A pointer to the class
                 3) The index of the handler
  RETURNS      : Nothing useful
  SIDE EFFECTS : Watch flag for the handler set
  NOTES        : None
 *********************************************************/
void DefmessageHandlerSetWatch(
  Defclass *theClass,
  unsigned theIndex,
  bool newState)
  {
   theClass->handlers[theIndex-1].trace = newState;
  }

#endif

/***************************************************
  NAME         : FindDefmessageHandler
  DESCRIPTION  : Determines the index of a specfied
                  message-handler
  INPUTS       : 1) A pointer to the class
                 2) Name-string of the handler
                 3) Handler-type: "around","before",
                    "primary", or "after"
  RETURNS      : The index of the handler
                   (0 if not found)
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
unsigned FindDefmessageHandler(
  Defclass *theDefclass,
  const char *hname,
  const char *htypestr)
  {
   unsigned htype;
   CLIPSLexeme *hsym;
   int theIndex;
   Environment *theEnv = theDefclass->header.env;

   htype = HandlerType(theEnv,"handler-lookup",false,htypestr);
   if (htype == MERROR)
     { return 0; }

   hsym = FindSymbolHN(theEnv,hname,SYMBOL_BIT);
   if (hsym == NULL)
     { return 0; }

   theIndex = FindHandlerByIndex(theDefclass,hsym,htype);
   return (unsigned) (theIndex+1);
  }

/***************************************************
  NAME         : DefmessageHandlerIsDeletable
  DESCRIPTION  : Determines if a message-handler
                   can be deleted
  INPUTS       : 1) Address of the handler's class
                 2) Index of the handler
  RETURNS      : True if deletable, false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
bool DefmessageHandlerIsDeletable(
  Defclass *theDefclass,
  unsigned theIndex)
  {
   Environment *theEnv = theDefclass->header.env;

   if (! ConstructsDeletable(theEnv))
     { return false; }

   if (theDefclass->handlers[theIndex-1].system == 1)
     { return false; }

#if (! BLOAD_ONLY) && (! RUN_TIME)
   return (HandlersExecuting(theDefclass) == false) ? true : false;
#else
   return false;
#endif
  }

/******************************************************************************
  NAME         : UndefmessageHandlerCommand
  DESCRIPTION  : Deletes a handler from a class
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Handler deleted if possible
  NOTES        : H/L Syntax: (undefmessage-handler <class> <handler> [<type>])
 ******************************************************************************/
void UndefmessageHandlerCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
#if RUN_TIME || BLOAD_ONLY
   PrintErrorID(theEnv,"MSGCOM",3,false);
   WriteString(theEnv,STDERR,"Unable to delete message-handlers.\n");
#else
   CLIPSLexeme *mname;
   const char *tname;
   UDFValue theArg;
   Defclass *cls;

#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv))
     {
      PrintErrorID(theEnv,"MSGCOM",3,false);
      WriteString(theEnv,STDERR,"Unable to delete message-handlers.\n");
      return;
     }
#endif
   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg)) return;

   cls = LookupDefclassByMdlOrScope(theEnv,theArg.lexemeValue->contents);
   if ((cls == NULL) ? (strcmp(theArg.lexemeValue->contents,"*") != 0) : false)
     {
      ClassExistError(theEnv,"undefmessage-handler",theArg.lexemeValue->contents);
      return;
     }
   if (! UDFNextArgument(context,SYMBOL_BIT,&theArg)) return;

   mname = theArg.lexemeValue;
   if (UDFHasNextArgument(context))
     {
      if (! UDFNextArgument(context,SYMBOL_BIT,&theArg)) return;

      tname = theArg.lexemeValue->contents;
      if (strcmp(tname,"*") == 0)
        tname = NULL;
     }
   else
     tname = MessageHandlerData(theEnv)->hndquals[MPRIMARY];
   WildDeleteHandler(theEnv,cls,mname,tname);
#endif
  }

/***********************************************************
  NAME         : UndefmessageHandler
  DESCRIPTION  : Deletes a handler from a class
  INPUTS       : 1) Class address    (Can be NULL)
                 2) Handler index (can be 0)
  RETURNS      : 1 if successful, 0 otherwise
  SIDE EFFECTS : Handler deleted if possible
  NOTES        : None
 ***********************************************************/
bool UndefmessageHandler(
  Defclass *theDefclass,
  unsigned mhi,
  Environment *allEnv)
  {
   Environment *theEnv;
#if (! RUN_TIME) &&  (! BLOAD_ONLY)
   bool success;
   GCBlock gcb;
#endif

   if (theDefclass == NULL)
     { theEnv = allEnv; }
   else
     { theEnv = theDefclass->header.env; }

#if RUN_TIME || BLOAD_ONLY
   PrintErrorID(theEnv,"MSGCOM",3,false);
   WriteString(theEnv,STDERR,"Unable to delete message-handlers.\n");
   return false;
#else
     
#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv))
     {
      PrintErrorID(theEnv,"MSGCOM",3,false);
      WriteString(theEnv,STDERR,"Unable to delete message-handlers.\n");
      return false;
     }
#endif
   GCBlockStart(theEnv,&gcb);
   if (theDefclass == NULL)
     {
      if (mhi != 0)
        {
         PrintErrorID(theEnv,"MSGCOM",1,false);
         WriteString(theEnv,STDERR,"Incomplete message-handler specification for deletion.\n");
         GCBlockEnd(theEnv,&gcb);
         return false;
        }
      success = WildDeleteHandler(theEnv,NULL,NULL,NULL);
      GCBlockEnd(theEnv,&gcb);
      return success;
     }

   if (mhi == 0)
     {
      success = WildDeleteHandler(theEnv,theDefclass,NULL,NULL);
      GCBlockEnd(theEnv,&gcb);
      return success;
     }

   if (HandlersExecuting(theDefclass))
     {
      HandlerDeleteError(theEnv,DefclassName(theDefclass));
      GCBlockEnd(theEnv,&gcb);
      return false;
     }

   theDefclass->handlers[mhi-1].mark = 1;
   DeallocateMarkedHandlers(theEnv,theDefclass);
   GCBlockEnd(theEnv,&gcb);
   return true;
#endif
  }

#if DEBUGGING_FUNCTIONS

/*******************************************************************************
  NAME         : PPDefmessageHandlerCommand
  DESCRIPTION  : Displays the pretty-print form (if any) for a handler
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (ppdefmessage-handler <class> <message> [<type>])
 *******************************************************************************/
void PPDefmessageHandlerCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   UDFValue theArg;
   CLIPSLexeme *csym, *msym;
   const char *tname;
   const char *logicalName;
   Defclass *cls = NULL;
   unsigned mtype;
   DefmessageHandler *hnd;

   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg))
     { return; }

   csym = FindSymbolHN(theEnv,theArg.lexemeValue->contents,SYMBOL_BIT);

   if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
     { return; }

   msym = FindSymbolHN(theEnv,theArg.lexemeValue->contents,SYMBOL_BIT);

   if (UDFHasNextArgument(context))
     {
      if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
        { return; }
      tname = theArg.lexemeValue->contents;
     }
   else
     tname = MessageHandlerData(theEnv)->hndquals[MPRIMARY];
     
   mtype = HandlerType(theEnv,"ppdefmessage-handler",true,tname);
   if (mtype == MERROR)
     {
      SetEvaluationError(theEnv,true);
      return;
     }
     
   if (UDFHasNextArgument(context))
     {
      logicalName = GetLogicalName(context,STDOUT);
      if (logicalName == NULL)
        {
         IllegalLogicalNameMessage(theEnv,"ppdefmessage-handler");
         SetHaltExecution(theEnv,true);
         SetEvaluationError(theEnv,true);
         return;
        }
     }
   else
     { logicalName = STDOUT; }

   if (csym != NULL)
     cls = LookupDefclassByMdlOrScope(theEnv,csym->contents);
   if (((cls == NULL) || (msym == NULL)) ? true :
       ((hnd = FindHandlerByAddress(cls,msym,mtype)) == NULL))
     {
      PrintErrorID(theEnv,"MSGCOM",2,false);
      WriteString(theEnv,STDERR,"Unable to find message-handler '");
      WriteString(theEnv,STDERR,msym->contents);
      WriteString(theEnv,STDERR,"' ");
      WriteString(theEnv,STDERR,tname);
      WriteString(theEnv,STDERR," for class '");
      WriteString(theEnv,STDERR,csym->contents);
      WriteString(theEnv,STDERR,"' in function 'ppdefmessage-handler'.\n");
      SetEvaluationError(theEnv,true);
      return;
     }
     
   if (strcmp(logicalName,"nil") == 0)
     {
      if (hnd->header.ppForm != NULL)
        { returnValue->lexemeValue = CreateString(theEnv,hnd->header.ppForm); }
      else
        { returnValue->lexemeValue = CreateString(theEnv,""); }
     }
   else
     {
      if (hnd->header.ppForm != NULL)
        WriteString(theEnv,logicalName,hnd->header.ppForm);
     }
  }

/*****************************************************************************
  NAME         : ListDefmessageHandlersCommand
  DESCRIPTION  : Depending on arguments, does lists handlers which
                   match restrictions
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (list-defmessage-handlers [<class> [inherit]]))
 *****************************************************************************/
void ListDefmessageHandlersCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   bool inhp;
   Defclass *clsptr;

   if (UDFArgumentCount(context) == 0)
     ListDefmessageHandlers(theEnv,NULL,STDOUT,false);
   else
     {
      clsptr = ClassInfoFnxArgs(context,"list-defmessage-handlers",&inhp);
      if (clsptr == NULL)
        return;
      ListDefmessageHandlers(theEnv,clsptr,STDOUT,inhp);
     }
  }

/********************************************************************
  NAME         : PreviewSendCommand
  DESCRIPTION  : Displays a list of the core for a message describing
                   shadows,etc.
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Temporary core created and destroyed
  NOTES        : H/L Syntax: (preview-send <class> <msg>)
 ********************************************************************/
void PreviewSendCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Defclass *cls;
   UDFValue theArg;

   /* =============================
      Get the class for the message
      ============================= */

   if (! UDFFirstArgument(context,SYMBOL_BIT,&theArg))
     { return; }

   cls = LookupDefclassByMdlOrScope(theEnv,theArg.lexemeValue->contents);

   if (cls == NULL)
     {
      ClassExistError(theEnv,"preview-send",theArg.lexemeValue->contents);
      return;
     }

   if (! UDFNextArgument(context,SYMBOL_BIT,&theArg))
     { return; }

   PreviewSend(cls,STDOUT,theArg.lexemeValue->contents);
  }

/********************************************************
  NAME         : DefmessageHandlerPPForm
  DESCRIPTION  : Gets a message-handler pretty print form
  INPUTS       : 1) Address of the handler's class
                 2) Index of the handler
  RETURNS      : True if printable, false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ********************************************************/
const char *DefmessageHandlerPPForm(
  Defclass *theDefclass,
  unsigned theIndex)
  {
   return theDefclass->handlers[theIndex-1].header.ppForm;
  }

/*******************************************************************
  NAME         : ListDefmessageHandlers
  DESCRIPTION  : Lists message-handlers for a class
  INPUTS       : 1) The logical name of the output
                 2) Class name (NULL to display all handlers)
                 3) A flag indicating whether to list inherited
                    handlers or not
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 *******************************************************************/
void ListDefmessageHandlers(
  Environment *theEnv,
  Defclass *theDefclass,
  const char *logName,
  bool inhp)
  {
   unsigned long cnt;
   PACKED_CLASS_LINKS plinks;

   if (theDefclass != NULL)
     {
      if (inhp)
        { cnt = DisplayHandlersInLinks(theEnv,logName,&theDefclass->allSuperclasses,0); }
      else
        {
         plinks.classCount = 1;
         plinks.classArray = &theDefclass;
         cnt = DisplayHandlersInLinks(theEnv,logName,&plinks,0);
        }
     }
   else
     {
      plinks.classCount = 1;
      cnt = 0L;
      for (theDefclass = GetNextDefclass(theEnv,NULL) ;
           theDefclass != NULL ;
           theDefclass = GetNextDefclass(theEnv,theDefclass))
        {
         plinks.classArray = &theDefclass;
         cnt += DisplayHandlersInLinks(theEnv,logName,&plinks,0);
        }
     }
   PrintTally(theEnv,logName,cnt,"message-handler","message-handlers");
  }

/********************************************************************
  NAME         : PreviewSend
  DESCRIPTION  : Displays a list of the core for a message describing
                   shadows,etc.
  INPUTS       : 1) Logical name of output
                 2) Class pointer
                 3) Message name-string
  RETURNS      : Nothing useful
  SIDE EFFECTS : Temporary core created and destroyed
  NOTES        : None
 ********************************************************************/
void PreviewSend(
  Defclass *theDefclass,
  const char *logicalName,
  const char *msgname)
  {
   HANDLER_LINK *core;
   CLIPSLexeme *msym;
   Environment *theEnv = theDefclass->header.env;

   msym = FindSymbolHN(theEnv,msgname,SYMBOL_BIT);
   if (msym == NULL)
     { return; }

   core = FindPreviewApplicableHandlers(theEnv,theDefclass,msym);
   if (core != NULL)
     {
      DisplayCore(theEnv,logicalName,core,0);
      DestroyHandlerLinks(theEnv,core);
     }
  }

/****************************************************
  NAME         : DisplayHandlersInLinks
  DESCRIPTION  : Recursively displays all handlers
                  for an array of classes
  INPUTS       : 1) The logical name of the output
                 2) The packed class links
                 3) The index to print from the links
  RETURNS      : The number of handlers printed
  SIDE EFFECTS : None
  NOTES        : Used by DescribeClass()
 ****************************************************/
unsigned long DisplayHandlersInLinks(
  Environment *theEnv,
  const char *logName,
  PACKED_CLASS_LINKS *plinks,
  unsigned int theIndex)
  {
   unsigned long i;
   unsigned long cnt;

   cnt = plinks->classArray[theIndex]->handlerCount;
   if ((theIndex + 1) < plinks->classCount)
     cnt += DisplayHandlersInLinks(theEnv,logName,plinks,theIndex + 1);
   for (i = 0 ; i < plinks->classArray[theIndex]->handlerCount ; i++)
     PrintHandler(theEnv,logName,&plinks->classArray[theIndex]->handlers[i],false,true);
   return cnt;
  }

#endif

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

#if ! RUN_TIME

/**********************************************************
  NAME         : CreateSystemHandlers
  DESCRIPTION  : Attachess the system message-handlers
                 after a (clear)
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : System handlers created
  NOTES        : Must be called after CreateSystemClasses()
 **********************************************************/
static void CreateSystemHandlers(
  Environment *theEnv,
  void *context)
  {
   NewSystemHandler(theEnv,USER_TYPE_NAME,INIT_STRING,"init-slots",0);
   NewSystemHandler(theEnv,USER_TYPE_NAME,DELETE_STRING,"delete-instance",0);
   NewSystemHandler(theEnv,USER_TYPE_NAME,CREATE_STRING,"(create-instance)",0);

#if DEBUGGING_FUNCTIONS
   NewSystemHandler(theEnv,USER_TYPE_NAME,PRINT_STRING,"ppinstance",0);
#endif

   NewSystemHandler(theEnv,USER_TYPE_NAME,DIRECT_MODIFY_STRING,"(direct-modify)",1);
   NewSystemHandler(theEnv,USER_TYPE_NAME,MSG_MODIFY_STRING,"(message-modify)",1);
   NewSystemHandler(theEnv,USER_TYPE_NAME,DIRECT_DUPLICATE_STRING,"(direct-duplicate)",2);
   NewSystemHandler(theEnv,USER_TYPE_NAME,MSG_DUPLICATE_STRING,"(message-duplicate)",2);
  }

#endif

#if (! BLOAD_ONLY) && (! RUN_TIME)

/************************************************************
  NAME         : WildDeleteHandler
  DESCRIPTION  : Deletes a handler from a class
  INPUTS       : 1) Class address (Can be NULL)
                 2) Message Handler Name (Can be NULL)
                 3) Type name ("primary", etc.)
  RETURNS      : 1 if successful, 0 otherwise
  SIDE EFFECTS : Handler deleted if possible
  NOTES        : None
 ************************************************************/
static bool WildDeleteHandler(
  Environment *theEnv,
  Defclass *cls,
  CLIPSLexeme *msym,
  const char *tname)
  {
   int mtype;

   if (msym == NULL)
     msym = CreateSymbol(theEnv,"*");
   if (tname != NULL)
     {
      mtype = (int) HandlerType(theEnv,"undefmessage-handler",true,tname);
      if (mtype == MERROR)
        return false;
     }
   else
     mtype = -1;
   if (cls == NULL)
     {
      bool success = true;

      for (cls = GetNextDefclass(theEnv,NULL) ;
           cls != NULL ;
           cls = GetNextDefclass(theEnv,cls))
        if (DeleteHandler(theEnv,cls,msym,mtype,false) == false)
          success = false;
      return(success);
     }
   return(DeleteHandler(theEnv,cls,msym,mtype,true));
  }

#endif

#if DEBUGGING_FUNCTIONS

/******************************************************************
  NAME         : DefmessageHandlerWatchAccess
  DESCRIPTION  : Parses a list of class names passed by
                 AddWatchItem() and sets the traces accordingly
  INPUTS       : 1) A code indicating which trace flag is to be set
                    0 - Watch instance creation/deletion
                    1 - Watch slot changes to instances
                 2) The value to which to set the trace flags
                 3) A list of expressions containing the names
                    of the classes for which to set traces
  RETURNS      : True if all OK, false otherwise
  SIDE EFFECTS : Watch flags set in specified classes
  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
static bool DefmessageHandlerWatchAccess(
  Environment *theEnv,
  int code,
  bool newState,
  Expression *argExprs)
  {
#if MAC_XCD
#pragma unused(code)
#endif
   if (newState)
     return(DefmessageHandlerWatchSupport(theEnv,"watch",NULL,newState,
                                        NULL,DefmessageHandlerSetWatch,argExprs));
   else
     return(DefmessageHandlerWatchSupport(theEnv,"unwatch",NULL,newState,
                                        NULL,DefmessageHandlerSetWatch,argExprs));
  }

/***********************************************************************
  NAME         : DefmessageHandlerWatchPrint
  DESCRIPTION  : Parses a list of class names passed by
                 AddWatchItem() and displays the traces accordingly
  INPUTS       : 1) The logical name of the output
                 2) A code indicating which trace flag is to be examined
                    0 - Watch instance creation/deletion
                    1 - Watch slot changes to instances
                 3) A list of expressions containing the names
                    of the classes for which to examine traces
  RETURNS      : True if all OK, false otherwise
  SIDE EFFECTS : Watch flags displayed for specified classes
  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
static bool DefmessageHandlerWatchPrint(
  Environment *theEnv,
  const char *logName,
  int code,
  Expression *argExprs)
  {
#if MAC_XCD
#pragma unused(code)
#endif
   return DefmessageHandlerWatchSupport(theEnv,"list-watch-items",logName,false,
                                        PrintHandlerWatchFlag,NULL,argExprs);
  }

/*******************************************************
  NAME         : DefmessageHandlerWatchSupport
  DESCRIPTION  : Sets or displays handlers specified
  INPUTS       : 1) The calling function name
                 2) The logical output name for displays
                    (can be NULL)
                 4) The new set state (can be -1)
                 5) The print function (can be NULL)
                 6) The trace function (can be NULL)
                 7) The handlers expression list
  RETURNS      : True if all OK,
                 false otherwise
  SIDE EFFECTS : Handler trace flags set or displayed
  NOTES        : None
 *******************************************************/
static bool DefmessageHandlerWatchSupport(
  Environment *theEnv,
  const char *funcName,
  const char *logName,
  bool newState,
  void (*printFunc)(Environment *,const char *,Defclass *,unsigned),
  void (*traceFunc)(Defclass *,unsigned,bool),
  Expression *argExprs)
  {
   Defmodule *theModule;
   Defclass *theClass;
   const char *theHandlerStr;
   int theType;
   unsigned int argIndex = 2;
   UDFValue tmpData;

   /* ===============================
      If no handlers are specified,
      show the trace for all handlers
      in all handlers
      =============================== */
   if (argExprs == NULL)
     {
      SaveCurrentModule(theEnv);
      theModule = GetNextDefmodule(theEnv,NULL);
      while (theModule != NULL)
        {
         SetCurrentModule(theEnv,theModule);
         if (traceFunc == NULL)
           {
            WriteString(theEnv,logName,DefmoduleName(theModule));
            WriteString(theEnv,logName,":\n");
           }
         theClass = GetNextDefclass(theEnv,NULL);
         while (theClass != NULL)
            {
             if (WatchClassHandlers(theEnv,theClass,NULL,-1,logName,newState,
                                    true,printFunc,traceFunc) == false)
                 return false;
             theClass = GetNextDefclass(theEnv,theClass);
            }
          theModule = GetNextDefmodule(theEnv,theModule);
         }
      RestoreCurrentModule(theEnv);
      return true;
     }

   /* ================================================
      Set or show the traces for the specified handler
      ================================================ */
   while (argExprs != NULL)
     {
      if (EvaluateExpression(theEnv,argExprs,&tmpData))
        return false;
      if (tmpData.header->type != SYMBOL_TYPE)
        {
         ExpectedTypeError1(theEnv,funcName,argIndex,"'class name'");
         return false;
        }
      theClass = LookupDefclassByMdlOrScope(theEnv,tmpData.lexemeValue->contents);
      if (theClass == NULL)
        {
         ExpectedTypeError1(theEnv,funcName,argIndex,"'class name'");
         return false;
        }
      if (GetNextArgument(argExprs) != NULL)
        {
         argExprs = GetNextArgument(argExprs);
         argIndex++;
         if (EvaluateExpression(theEnv,argExprs,&tmpData))
           return false;
         if (tmpData.header->type != SYMBOL_TYPE)
           {
            ExpectedTypeError1(theEnv,funcName,argIndex,"'handler name'");
            return false;
           }
         theHandlerStr = tmpData.lexemeValue->contents;
         if (GetNextArgument(argExprs) != NULL)
           {
            argExprs = GetNextArgument(argExprs);
            argIndex++;
            if (EvaluateExpression(theEnv,argExprs,&tmpData))
              return false;
            if (tmpData.header->type != SYMBOL_TYPE)
              {
               ExpectedTypeError1(theEnv,funcName,argIndex,"'handler type'");
               return false;
              }
            if ((theType = (int) HandlerType(theEnv,funcName,true,tmpData.lexemeValue->contents)) == MERROR)
              return false;
           }
         else
           theType = -1;
        }
      else
        {
         theHandlerStr = NULL;
         theType = -1;
        }
      if (WatchClassHandlers(theEnv,theClass,theHandlerStr,theType,logName,
                             newState,false,printFunc,traceFunc) == false)
        {
         ExpectedTypeError1(theEnv,funcName,argIndex,"handler");
         return false;
        }
      argIndex++;
      argExprs = GetNextArgument(argExprs);
     }
   return true;
  }

/*******************************************************
  NAME         : WatchClassHandlers
  DESCRIPTION  : Sets or displays handlers specified
  INPUTS       : 1) The class
                 2) The handler name (or NULL wildcard)
                 3) The handler type (or -1 wildcard)
                 4) The logical output name for displays
                    (can be NULL)
                 5) The new set state (can be -1)
                 6) The print function (can be NULL)
                 7) The trace function (can be NULL)
  RETURNS      : True if all OK,
                 false otherwise
  SIDE EFFECTS : Handler trace flags set or displayed
  NOTES        : None
 *******************************************************/
static bool WatchClassHandlers(
  Environment *theEnv,
  Defclass *theClass,
  const char *theHandlerStr,
  int theType,
  const char *logName,
  bool newState,
  bool indentp,
  void (*printFunc)(Environment *,const char *,Defclass *,unsigned),
  void (*traceFunc)(Defclass *,unsigned,bool))
  {
   unsigned theHandler;
   bool found = false;

   theHandler = GetNextDefmessageHandler(theClass,0);
   while (theHandler != 0)
     {
      if ((theType == -1) ? true :
          (theType == (int) theClass->handlers[theHandler-1].type))
        {
         if ((theHandlerStr == NULL) ? true :
             (strcmp(theHandlerStr,DefmessageHandlerName(theClass,theHandler)) == 0))
            {
             if (traceFunc != NULL)
               (*traceFunc)(theClass,theHandler,newState);
             else
               {
                if (indentp)
                  WriteString(theEnv,logName,"   ");
                (*printFunc)(theEnv,logName,theClass,theHandler);
               }
             found = true;
            }
        }
      theHandler = GetNextDefmessageHandler(theClass,theHandler);
     }
   if ((theHandlerStr != NULL) && (theType != -1) && (found == false))
     return false;
   return true;
  }

/***************************************************
  NAME         : PrintHandlerWatchFlag
  DESCRIPTION  : Displays trace value for handler
  INPUTS       : 1) The logical name of the output
                 2) The class
                 3) The handler index
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
static void PrintHandlerWatchFlag(
  Environment *theEnv,
  const char *logName,
  Defclass *theClass,
  unsigned theHandler)
  {
   WriteString(theEnv,logName,DefclassName(theClass));
   WriteString(theEnv,logName," ");
   WriteString(theEnv,logName,DefmessageHandlerName(theClass,theHandler));
   WriteString(theEnv,logName," ");
   WriteString(theEnv,logName,DefmessageHandlerType(theClass,theHandler));

   if (DefmessageHandlerGetWatch(theClass,theHandler))
     WriteString(theEnv,logName," = on\n");
   else
     WriteString(theEnv,logName," = off\n");
  }

#endif /* DEBUGGING_FUNCTIONS */

#endif /* OBJECT_SYSTEM */

