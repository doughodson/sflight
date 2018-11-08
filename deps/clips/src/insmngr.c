   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/17/18             */
   /*                                                     */
   /*          INSTANCE PRIMITIVE SUPPORT MODULE          */
   /*******************************************************/

/*************************************************************/
/* Purpose:  Creation and Deletion of Instances Routines     */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Removed LOGICAL_DEPENDENCIES compilation flag. */
/*                                                           */
/*            Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Used gensprintf instead of sprintf.            */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Newly created instances can no longer use      */
/*            a preexisting instance name of another class   */
/*            [INSMNGR16].                                   */
/*                                                           */
/*      6.31: Marked deleted instances so that partial       */
/*            matches will not be propagated when the match  */
/*            is based on both the existence and             */
/*            non-existence of an instance.                  */
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
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#if DEFRULE_CONSTRUCT
#include "network.h"
#include "drive.h"
#include "objrtmch.h"
#include "lgcldpnd.h"
#endif

#include "classcom.h"
#include "classfun.h"
#include "cstrnchk.h"
#include "engine.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "insfun.h"
#include "memalloc.h"
#include "miscfun.h"
#include "modulutl.h"
#include "msgcom.h"
#include "msgfun.h"
#include "prccode.h"
#include "prntutil.h"
#include "router.h"
#include "sysdep.h"
#include "utility.h"

#include "insmngr.h"

#include "inscom.h"
#include "watch.h"

/* =========================================
   *****************************************
                   CONSTANTS
   =========================================
   ***************************************** */
#define MAKE_TRACE   "==>"
#define UNMAKE_TRACE "<=="

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static Instance               *NewInstance(Environment *);
   static Instance               *InstanceLocationInfo(Environment *,Defclass *,CLIPSLexeme *,Instance **,
                                                       unsigned *);
   static void                    InstallInstance(Environment *,Instance *,bool);
   static void                    BuildDefaultSlots(Environment *,bool);
   static bool                    CoreInitializeInstance(Environment *,Instance *,Expression *);
   static bool                    CoreInitializeInstanceCV(Environment *,Instance *,CLIPSValue *);
   static bool                    InsertSlotOverrides(Environment *,Instance *,Expression *);
   static bool                    InsertSlotOverridesCV(Environment *,Instance *,CLIPSValue *);
   static void                    EvaluateClassDefaults(Environment *,Instance *);
   static bool                    IMModifySlots(Environment *,Instance *,CLIPSValue *);

#if DEBUGGING_FUNCTIONS
   static void                    PrintInstanceWatch(Environment *,const char *,Instance *);
#endif

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************
  NAME         : InitializeInstanceCommand
  DESCRIPTION  : Initializes an instance of a class
  INPUTS       : The address of the result value
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance intialized
  NOTES        : H/L Syntax:
                 (active-initialize-instance <instance-name>
                    <slot-override>*)
 ***********************************************************/
void InitializeInstanceCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   Instance *ins;

   returnValue->lexemeValue = FalseSymbol(theEnv);
   ins = CheckInstance(context);
   if (ins == NULL)
     return;
   if (CoreInitializeInstance(theEnv,ins,GetFirstArgument()->nextArg) == true)
     { returnValue->value = ins->name; }
  }

/****************************************************************
  NAME         : MakeInstanceCommand
  DESCRIPTION  : Creates and initializes an instance of a class
  INPUTS       : The address of the result value
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance intialized
  NOTES        : H/L Syntax:
                 (active-make-instance <instance-name> of <class>
                    <slot-override>*)
  CHANGES      : It's now possible to create an instance of a
                 class that's not in scope if the module name
                 is specified.
 ****************************************************************/
void MakeInstanceCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   CLIPSLexeme *iname;
   Instance *ins;
   UDFValue temp;
   Defclass *cls;

   returnValue->lexemeValue = FalseSymbol(theEnv);
   EvaluateExpression(theEnv,GetFirstArgument(),&temp);
   
   if (temp.header->type == INSTANCE_NAME_TYPE)
     { iname = temp.lexemeValue; }
   else if (temp.header->type == SYMBOL_TYPE)
     { iname = CreateInstanceName(theEnv,temp.lexemeValue->contents); }
   else
     {
      PrintErrorID(theEnv,"INSMNGR",1,false);
      WriteString(theEnv,STDERR,"Expected a valid name for new instance.\n");
      SetEvaluationError(theEnv,true);
      InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
      return;
     }

   if (GetFirstArgument()->nextArg->type == DEFCLASS_PTR)
     cls = (Defclass *) GetFirstArgument()->nextArg->value;
   else
     {
      EvaluateExpression(theEnv,GetFirstArgument()->nextArg,&temp);
      if (temp.header->type != SYMBOL_TYPE)
        {
         PrintErrorID(theEnv,"INSMNGR",2,false);
         WriteString(theEnv,STDERR,"Expected a valid class name for new instance.\n");
         SetEvaluationError(theEnv,true);
         InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
         return;
        }

      cls = LookupDefclassByMdlOrScope(theEnv,temp.lexemeValue->contents); // Module or scope is now allowed

      if (cls == NULL)
        {
         ClassExistError(theEnv,ExpressionFunctionCallName(EvaluationData(theEnv)->CurrentExpression)->contents,
                         temp.lexemeValue->contents);
         SetEvaluationError(theEnv,true);
         InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
         return;
        }
     }

   ins = BuildInstance(theEnv,iname,cls,true);
   if (ins == NULL) return;

   if (CoreInitializeInstance(theEnv,ins,GetFirstArgument()->nextArg->nextArg) == true)
     { returnValue->value = GetFullInstanceName(theEnv,ins); }
   else
     QuashInstance(theEnv,ins);
  }

/***************************************************
  NAME         : GetFullInstanceName
  DESCRIPTION  : If this function is called while
                 the current module is other than
                 the one in which the instance
                 resides, then the module name is
                 prepended to the instance name.
                 Otherwise - the base name only is
                 returned.
  INPUTS       : The instance
  RETURNS      : The instance name symbol (with
                 module name and :: prepended)
  SIDE EFFECTS : Temporary buffer allocated possibly
                 and new symbol created
  NOTES        : Used to differentiate between
                 instances of the same name in
                 different modules.
                 Instances are now global in scope so
                 each instance name must belong to a
                 single instance. It's no longer
                 necessary to return the full instance
                 name.
 ***************************************************/
CLIPSLexeme *GetFullInstanceName(
  Environment *theEnv,
  Instance *ins)
  {
   if (ins == &InstanceData(theEnv)->DummyInstance)
     { return CreateInstanceName(theEnv,"Dummy Instance"); }

   return ins->name;
  }

/***************************************************
  NAME         : BuildInstance
  DESCRIPTION  : Creates an uninitialized instance
  INPUTS       : 1) Name of the instance
                 2) Class pointer
                 3) Flag indicating whether init
                    message will be called for
                    this instance or not
  RETURNS      : The address of the new instance,
                   NULL on errors (or when a
                   a logical basis in a rule was
                   deleted int the same RHS in
                   which the instance creation
                   occurred)
  SIDE EFFECTS : Old definition (if any) is deleted
  NOTES        : None
 ***************************************************/
Instance *BuildInstance(
  Environment *theEnv,
  CLIPSLexeme *iname,
  Defclass *cls,
  bool initMessage)
  {
   Instance *ins,*iprv;
   unsigned hashTableIndex;
   unsigned modulePosition;
   CLIPSLexeme *moduleName;
   UDFValue temp;

   if (iname->header.type == SYMBOL_TYPE)
     { iname = CreateInstanceName(theEnv,iname->contents); }

#if DEFRULE_CONSTRUCT
   if (EngineData(theEnv)->JoinOperationInProgress && cls->reactive)
     {
      PrintErrorID(theEnv,"INSMNGR",10,false);
      WriteString(theEnv,STDERR,"Cannot create instances of reactive classes while ");
      WriteString(theEnv,STDERR,"pattern-matching is in process.\n");
      SetEvaluationError(theEnv,true);
      InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
      return NULL;
     }
#endif
   if (cls->abstract)
     {
      PrintErrorID(theEnv,"INSMNGR",3,false);
      WriteString(theEnv,STDERR,"Cannot create instances of abstract class '");
      WriteString(theEnv,STDERR,DefclassName(cls));
      WriteString(theEnv,STDERR,"'.\n");
      SetEvaluationError(theEnv,true);
      InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
      return NULL;
     }
   modulePosition = FindModuleSeparator(iname->contents);
   if (modulePosition)
     {
      moduleName = ExtractModuleName(theEnv,modulePosition,iname->contents);
      if ((moduleName == NULL) ||
          (moduleName != cls->header.whichModule->theModule->header.name))
        {
         PrintErrorID(theEnv,"INSMNGR",11,true);
         WriteString(theEnv,STDERR,"Invalid module specifier in new instance name.\n");
         SetEvaluationError(theEnv,true);
         InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
         return NULL;
        }
      iname = ExtractConstructName(theEnv,modulePosition,iname->contents,INSTANCE_NAME_TYPE);
     }
   ins = InstanceLocationInfo(theEnv,cls,iname,&iprv,&hashTableIndex);

   if (ins != NULL)
     {
      if (ins->cls != cls)
        {
         PrintErrorID(theEnv,"INSMNGR",16,false);
         WriteString(theEnv,STDERR,"The instance name [");
         WriteString(theEnv,STDERR,iname->contents);
         WriteString(theEnv,STDERR,"] is in use by an instance of class '");
         WriteString(theEnv,STDERR,ins->cls->header.name->contents);
         WriteString(theEnv,STDERR,"'.\n");
         SetEvaluationError(theEnv,true);
         InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
         return NULL;
        }

      if (ins->installed == 0)
        {
         PrintErrorID(theEnv,"INSMNGR",4,false);
         WriteString(theEnv,STDERR,"The instance [");
         WriteString(theEnv,STDERR,iname->contents);
         WriteString(theEnv,STDERR,"] has a slot-value which depends on the instance definition.\n");
         SetEvaluationError(theEnv,true);
         InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
         return NULL;
        }
      ins->busy++;
      IncrementLexemeCount(iname);
      if (ins->garbage == 0)
        {
         if (InstanceData(theEnv)->MkInsMsgPass)
           DirectMessage(theEnv,MessageHandlerData(theEnv)->DELETE_SYMBOL,ins,NULL,NULL);
         else
           QuashInstance(theEnv,ins);
        }
      ins->busy--;
      ReleaseLexeme(theEnv,iname);
      if (ins->garbage == 0)
        {
         PrintErrorID(theEnv,"INSMNGR",5,false);
         WriteString(theEnv,STDERR,"Unable to delete old instance [");
         WriteString(theEnv,STDERR,iname->contents);
         WriteString(theEnv,STDERR,"].\n");
         SetEvaluationError(theEnv,true);
         InstanceData(theEnv)->makeInstanceError = MIE_COULD_NOT_CREATE_ERROR;
         return NULL;
        }
     }

   /* =============================================================
      Create the base instance from the defaults of the inheritance
      precedence list
      ============================================================= */
   InstanceData(theEnv)->CurrentInstance = NewInstance(theEnv);

#if DEFRULE_CONSTRUCT
   /* ==============================================
      Add this new instance as a dependent to
      any currently active basis - if the partial
      match was deleted, abort the instance creation
      ============================================== */
   if (AddLogicalDependencies(theEnv,(struct patternEntity *) InstanceData(theEnv)->CurrentInstance,false)
        == false)
     {
      rtn_struct(theEnv,instance,InstanceData(theEnv)->CurrentInstance);
      InstanceData(theEnv)->CurrentInstance = NULL;
      return NULL;
     }
#endif

   InstanceData(theEnv)->CurrentInstance->name = iname;
   InstanceData(theEnv)->CurrentInstance->cls = cls;
   BuildDefaultSlots(theEnv,initMessage);

   /* ============================================================
      Put the instance in the instance hash table and put it on its
        class's instance list
      ============================================================ */
   InstanceData(theEnv)->CurrentInstance->hashTableIndex = hashTableIndex;
   if (iprv == NULL)
     {
      InstanceData(theEnv)->CurrentInstance->nxtHash = InstanceData(theEnv)->InstanceTable[hashTableIndex];
      if (InstanceData(theEnv)->InstanceTable[hashTableIndex] != NULL)
        InstanceData(theEnv)->InstanceTable[hashTableIndex]->prvHash = InstanceData(theEnv)->CurrentInstance;
      InstanceData(theEnv)->InstanceTable[hashTableIndex] = InstanceData(theEnv)->CurrentInstance;
     }
   else
     {
      InstanceData(theEnv)->CurrentInstance->nxtHash = iprv->nxtHash;
      if (iprv->nxtHash != NULL)
        iprv->nxtHash->prvHash = InstanceData(theEnv)->CurrentInstance;
      iprv->nxtHash = InstanceData(theEnv)->CurrentInstance;
      InstanceData(theEnv)->CurrentInstance->prvHash = iprv;
     }

   /* ======================================
      Put instance in global and class lists
      ====================================== */
   if (InstanceData(theEnv)->CurrentInstance->cls->instanceList == NULL)
     InstanceData(theEnv)->CurrentInstance->cls->instanceList = InstanceData(theEnv)->CurrentInstance;
   else
     InstanceData(theEnv)->CurrentInstance->cls->instanceListBottom->nxtClass = InstanceData(theEnv)->CurrentInstance;
   InstanceData(theEnv)->CurrentInstance->prvClass = InstanceData(theEnv)->CurrentInstance->cls->instanceListBottom;
   InstanceData(theEnv)->CurrentInstance->cls->instanceListBottom = InstanceData(theEnv)->CurrentInstance;

   if (InstanceData(theEnv)->InstanceList == NULL)
     InstanceData(theEnv)->InstanceList = InstanceData(theEnv)->CurrentInstance;
   else
     InstanceData(theEnv)->InstanceListBottom->nxtList = InstanceData(theEnv)->CurrentInstance;
   InstanceData(theEnv)->CurrentInstance->prvList = InstanceData(theEnv)->InstanceListBottom;
   InstanceData(theEnv)->InstanceListBottom = InstanceData(theEnv)->CurrentInstance;
   InstanceData(theEnv)->ChangesToInstances = true;

   /* ==============================================================================
      Install the instance's name and slot-value symbols (prevent them from becoming
      ephemeral) - the class name and slot names are accounted for by the class
      ============================================================================== */
   InstallInstance(theEnv,InstanceData(theEnv)->CurrentInstance,true);

   ins = InstanceData(theEnv)->CurrentInstance;
   InstanceData(theEnv)->CurrentInstance = NULL;

   if (InstanceData(theEnv)->MkInsMsgPass)
     { DirectMessage(theEnv,MessageHandlerData(theEnv)->CREATE_SYMBOL,ins,&temp,NULL); }

#if DEFRULE_CONSTRUCT
   if (ins->cls->reactive)
     ObjectNetworkAction(theEnv,OBJECT_ASSERT,ins,-1);
#endif

   if (EvaluationData(theEnv)->EvaluationError)
     { InstanceData(theEnv)->makeInstanceError = MIE_RULE_NETWORK_ERROR; }
   else
     { InstanceData(theEnv)->makeInstanceError = MIE_NO_ERROR; }

   return ins;
  }

/*****************************************************************************
  NAME         : InitSlotsCommand
  DESCRIPTION  : Calls Kernel Expression Evaluator EvaluateExpression
                   for each expression-value of an instance expression

                 Evaluates default slots only - slots that were specified
                 by overrides (sp->override == 1) are ignored)
  INPUTS       : 1) Instance address
  RETURNS      : Nothing useful
  SIDE EFFECTS : Each UDFValue slot in the instance's slot array is replaced
                   by the evaluation (by EvaluateExpression) of the expression
                   in the slot list.  The old expression-values
                   are deleted.
  NOTES        : H/L Syntax: (init-slots <instance>)
 *****************************************************************************/
void InitSlotsCommand(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   EvaluationData(theEnv)->EvaluationError = false;

   if (CheckCurrentMessage(theEnv,"init-slots",true) == false)
     {
      returnValue->lexemeValue = FalseSymbol(theEnv);
      return;
     }

   EvaluateClassDefaults(theEnv,GetActiveInstance(theEnv));

   if (! EvaluationData(theEnv)->EvaluationError)
     { returnValue->instanceValue = GetActiveInstance(theEnv); }
   else
     { returnValue->lexemeValue = FalseSymbol(theEnv); }
  }

/******************************************************
  NAME         : QuashInstance
  DESCRIPTION  : Deletes an instance if it is not in
                   use, otherwise sticks it on the
                   garbage list
  INPUTS       : The instance
  RETURNS      : 1 if successful, 0 otherwise
  SIDE EFFECTS : Instance deleted or added to garbage
  NOTES        : Even though the instance is removed
                   from the class list, hash table and
                   instance list, its links remain
                   unchanged so that outside loops
                   can still determine where the next
                   node in the list is (assuming the
                   instance was garbage collected).
 ******************************************************/
UnmakeInstanceError QuashInstance(
  Environment *theEnv,
  Instance *ins)
  {
   int iflag;
   IGARBAGE *gptr;

#if DEFRULE_CONSTRUCT
   if (EngineData(theEnv)->JoinOperationInProgress && ins->cls->reactive)
     {
      PrintErrorID(theEnv,"INSMNGR",12,false);
      WriteString(theEnv,STDERR,"Cannot delete instances of reactive classes while ");
      WriteString(theEnv,STDERR,"pattern-matching is in process.\n");
      SetEvaluationError(theEnv,true);
      InstanceData(theEnv)->unmakeInstanceError = UIE_COULD_NOT_DELETE_ERROR;
      return UIE_COULD_NOT_DELETE_ERROR;
     }
#endif

   if (ins->garbage == 1)
     {
      InstanceData(theEnv)->unmakeInstanceError = UIE_COULD_NOT_DELETE_ERROR;
      return UIE_DELETED_ERROR;
     }
 
   if (ins->installed == 0)
     {
      PrintErrorID(theEnv,"INSMNGR",6,false);
      WriteString(theEnv,STDERR,"Cannot delete instance [");
      WriteString(theEnv,STDERR,ins->name->contents);
      WriteString(theEnv,STDERR,"] during initialization.\n");
      SetEvaluationError(theEnv,true);
      InstanceData(theEnv)->unmakeInstanceError = UIE_COULD_NOT_DELETE_ERROR;
      return UIE_COULD_NOT_DELETE_ERROR;
     }
     
#if DEBUGGING_FUNCTIONS
   if (ins->cls->traceInstances)
     PrintInstanceWatch(theEnv,UNMAKE_TRACE,ins);
#endif

#if DEFRULE_CONSTRUCT
   RemoveEntityDependencies(theEnv,(struct patternEntity *) ins);

   if (ins->cls->reactive)
     {
      ins->garbage = 1;
      ObjectNetworkAction(theEnv,OBJECT_RETRACT,ins,-1);
      ins->garbage = 0;
     }
#endif

   if (ins->prvHash != NULL)
     ins->prvHash->nxtHash = ins->nxtHash;
   else
     InstanceData(theEnv)->InstanceTable[ins->hashTableIndex] = ins->nxtHash;
   if (ins->nxtHash != NULL)
     ins->nxtHash->prvHash = ins->prvHash;

   if (ins->prvClass != NULL)
     ins->prvClass->nxtClass = ins->nxtClass;
   else
     ins->cls->instanceList = ins->nxtClass;
   if (ins->nxtClass != NULL)
     ins->nxtClass->prvClass = ins->prvClass;
   else
     ins->cls->instanceListBottom = ins->prvClass;

   if (ins->prvList != NULL)
     ins->prvList->nxtList = ins->nxtList;
   else
     InstanceData(theEnv)->InstanceList = ins->nxtList;
   if (ins->nxtList != NULL)
     ins->nxtList->prvList = ins->prvList;
   else
     InstanceData(theEnv)->InstanceListBottom = ins->prvList;

   iflag = ins->installed;
   InstallInstance(theEnv,ins,false);

   /* ==============================================
      If the instance is the basis for an executing
      rule, don't bother deleting its slots yet, for
      they may still be needed by pattern variables
      ============================================== */
#if DEFRULE_CONSTRUCT
   if ((iflag == 1)
       && (ins->patternHeader.busyCount == 0))
#else
   if (iflag == 1)
#endif
     RemoveInstanceData(theEnv,ins);

   if ((ins->busy == 0) &&
       (InstanceData(theEnv)->MaintainGarbageInstances == false)
#if DEFRULE_CONSTRUCT
        && (ins->patternHeader.busyCount == 0)
#endif
       )
     {
      ReleaseLexeme(theEnv,ins->name);
      rtn_struct(theEnv,instance,ins);
     }
   else
     {
      gptr = get_struct(theEnv,igarbage);
      ins->garbage = 1;
      gptr->ins = ins;
      gptr->nxt = InstanceData(theEnv)->InstanceGarbageList;
      InstanceData(theEnv)->InstanceGarbageList = gptr;
      UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
     }
     
   InstanceData(theEnv)->ChangesToInstances = true;

   if (EvaluationData(theEnv)->EvaluationError)
     {
      InstanceData(theEnv)->unmakeInstanceError = UIE_RULE_NETWORK_ERROR;
      return UIE_RULE_NETWORK_ERROR;
     }
     
   InstanceData(theEnv)->unmakeInstanceError = UIE_NO_ERROR;
   return UIE_NO_ERROR;
  }


#if DEFRULE_CONSTRUCT

/****************************************************
  NAME         : InactiveInitializeInstance
  DESCRIPTION  : Initializes an instance of a class
                 Pattern-matching is automatically
                 delayed until the instance is
                 completely initialized
  INPUTS       : The address of the result value
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance intialized
  NOTES        : H/L Syntax:
                 (initialize-instance <instance-name>
                    <slot-override>*)
 ****************************************************/
void InactiveInitializeInstance(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   bool ov;

   ov = SetDelayObjectPatternMatching(theEnv,true);
   InitializeInstanceCommand(theEnv,context,returnValue);
   SetDelayObjectPatternMatching(theEnv,ov);
  }

/**************************************************************
  NAME         : InactiveMakeInstance
  DESCRIPTION  : Creates and initializes an instance of a class
                 Pattern-matching is automatically
                 delayed until the instance is
                 completely initialized
  INPUTS       : The address of the result value
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance intialized
  NOTES        : H/L Syntax:
                 (make-instance <instance-name> of <class>
                    <slot-override>*)
 **************************************************************/
void InactiveMakeInstance(
  Environment *theEnv,
  UDFContext *context,
  UDFValue *returnValue)
  {
   bool ov;

   ov = SetDelayObjectPatternMatching(theEnv,true);
   MakeInstanceCommand(theEnv,context,returnValue);
   SetDelayObjectPatternMatching(theEnv,ov);
  }

#endif

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/********************************************************
  NAME         : NewInstance
  DESCRIPTION  : Allocates and initializes a new instance
  INPUTS       : None
  RETURNS      : The address of the new instance
  SIDE EFFECTS : None
  NOTES        : None
 ********************************************************/
static Instance *NewInstance(
  Environment *theEnv)
  {
   Instance *instance;

   instance = get_struct(theEnv,instance);
#if DEFRULE_CONSTRUCT
   instance->patternHeader.theInfo = &InstanceData(theEnv)->InstanceInfo;

   instance->patternHeader.dependents = NULL;
   instance->patternHeader.busyCount = 0;
   instance->patternHeader.timeTag = 0L;

   instance->partialMatchList = NULL;
   instance->basisSlots = NULL;
   instance->reteSynchronized = false;
#endif
   instance->patternHeader.header.type = INSTANCE_ADDRESS_TYPE;
   instance->busy = 0;
   instance->installed = 0;
   instance->garbage = 0;
   instance->initSlotsCalled = 0;
   instance->initializeInProgress = 0;
   instance->name = NULL;
   instance->hashTableIndex = 0;
   instance->cls = NULL;
   instance->slots = NULL;
   instance->slotAddresses = NULL;
   instance->prvClass = NULL;
   instance->nxtClass = NULL;
   instance->prvHash = NULL;
   instance->nxtHash = NULL;
   instance->prvList = NULL;
   instance->nxtList = NULL;
   return(instance);
  }

/*****************************************************************
  NAME         : InstanceLocationInfo
  DESCRIPTION  : Determines where a specified instance belongs
                   in the instance hash table
  INPUTS       : 1) The class of the new instance
                 2) The symbol for the name of the instance
                 3) Caller's buffer for previous node address
                 4) Caller's buffer for hash value
  RETURNS      : The address of the found instance, NULL otherwise
  SIDE EFFECTS : None
  NOTES        : Instance names only have to be unique within
                 a module.
                 Change: instance names must be unique regardless
                 of module.
 *****************************************************************/
static Instance *InstanceLocationInfo(
  Environment *theEnv,
  Defclass *cls,
  CLIPSLexeme *iname,
  Instance **prv,
  unsigned *hashTableIndex)
  {
   Instance *ins;

   *hashTableIndex = HashInstance(iname);
   ins = InstanceData(theEnv)->InstanceTable[*hashTableIndex];

   /* ========================================
      Make sure all instances of the same name
      are grouped together regardless of what
      module their classes are in
      ======================================== */
   *prv = NULL;
   while (ins != NULL)
     {
      if (ins->name == iname)
        { return(ins); }
      *prv = ins;
      ins = ins->nxtHash;
     }

   /*
   while ((ins != NULL) ? (ins->name != iname) : false)
     {
      *prv = ins;
      ins = ins->nxtHash;
     }
   while ((ins != NULL) ? (ins->name == iname) : false)
     {
      if (ins->cls->header.whichModule->theModule ==
          cls->header.whichModule->theModule)
        return(ins);
      *prv = ins;
      ins = ins->nxtHash;
     }
   */
   return NULL;
  }

/********************************************************
  NAME         : InstallInstance
  DESCRIPTION  : Prevent name and slot value symbols
                   from being ephemeral (all others
                   taken care of by class defn)
  INPUTS       : 1) The address of the instance
                 2) A flag indicating whether to
                    install or deinstall
  RETURNS      : Nothing useful
  SIDE EFFECTS : Symbol counts incremented or decremented
  NOTES        : Slot symbol installations are handled
                   by PutSlotValue
 ********************************************************/
static void InstallInstance(
  Environment *theEnv,
  Instance *ins,
  bool set)
  {
   if (set == true)
     {
      if (ins->installed)
        return;
#if DEBUGGING_FUNCTIONS
      if (ins->cls->traceInstances)
        PrintInstanceWatch(theEnv,MAKE_TRACE,ins);
#endif
      ins->installed = 1;
      IncrementLexemeCount(ins->name);
      IncrementDefclassBusyCount(theEnv,ins->cls);
      InstanceData(theEnv)->GlobalNumberOfInstances++;
     }
   else
     {
      if (! ins->installed)
        return;
      ins->installed = 0;
      InstanceData(theEnv)->GlobalNumberOfInstances--;

      /* =======================================
         Class counts is decremented by
         RemoveInstanceData() when slot data is
         truly deleted - and name count is
         deleted by CleanupInstances() or
         QuashInstance() when instance is
         truly deleted
         ======================================= */
     }
  }

/****************************************************************
  NAME         : BuildDefaultSlots
  DESCRIPTION  : The current instance's address is
                   in the global variable CurrentInstance.
                   This builds the slots and the default values
                   from the direct class of the instance and its
                   inheritances.
  INPUTS       : Flag indicating whether init message will be
                 called for this instance or not
  RETURNS      : Nothing useful
  SIDE EFFECTS : Allocates the slot array for
                   the current instance
  NOTES        : The current instance's address is
                 stored in a global variable
 ****************************************************************/
static void BuildDefaultSlots(
  Environment *theEnv,
  bool initMessage)
  {
   unsigned i,j;
   unsigned scnt;
   unsigned lscnt;
   InstanceSlot *dst = NULL,**adst;
   SlotDescriptor **src;

   scnt = InstanceData(theEnv)->CurrentInstance->cls->instanceSlotCount;
   lscnt = InstanceData(theEnv)->CurrentInstance->cls->localInstanceSlotCount;
   if (scnt > 0)
     {
      InstanceData(theEnv)->CurrentInstance->slotAddresses = adst =
         (InstanceSlot **) gm2(theEnv,(sizeof(InstanceSlot *) * scnt));
      if (lscnt != 0)
        InstanceData(theEnv)->CurrentInstance->slots = dst =
           (InstanceSlot *) gm2(theEnv,(sizeof(InstanceSlot) * lscnt));
      src = InstanceData(theEnv)->CurrentInstance->cls->instanceTemplate;

      /* ==================================================
         A map of slot addresses is created - shared slots
         point at values in the class, and local slots
         point at values in the instance

         Also - slots are always given an initial value
         (since slots cannot be unbound). If there is
         already an instance of a class with a shared slot,
         that value is left alone
         ================================================== */
      for (i = 0 , j = 0 ; i < scnt ; i++)
        {
         if (src[i]->shared)
           {
            src[i]->sharedCount++;
            adst[i] = &(src[i]->sharedValue);
           }
         else
           {
            dst[j].desc = src[i];
            dst[j].value = NULL;
            adst[i] = &dst[j++];
           }
         if (adst[i]->value == NULL)
           {
            adst[i]->valueRequired = initMessage;
            if (adst[i]->desc->multiple)
              {
               adst[i]->type = MULTIFIELD_TYPE;
               adst[i]->value = CreateUnmanagedMultifield(theEnv,0L);
               RetainMultifield(theEnv,adst[i]->multifieldValue);
              }
            else
              {
               adst[i]->type = SYMBOL_TYPE;
               adst[i]->value = CreateSymbol(theEnv,"nil");
               AtomInstall(theEnv,adst[i]->type,adst[i]->value);
              }
           }
         else
           adst[i]->valueRequired = false;
         adst[i]->override = false;
        }
     }
  }

/*******************************************************************
  NAME         : CoreInitializeInstance
  DESCRIPTION  : Performs the core work for initializing an instance
  INPUTS       : 1) The instance address
                 2) Slot override expressions
  RETURNS      : True if all OK, false otherwise
  SIDE EFFECTS : EvaluationError set on errors - slots evaluated
  NOTES        : None
 *******************************************************************/
static bool CoreInitializeInstance(
  Environment *theEnv,
  Instance *ins,
  Expression *ovrexp)
  {
   UDFValue temp;

   if (ins->installed == 0)
     {
      PrintErrorID(theEnv,"INSMNGR",7,false);
      WriteString(theEnv,STDERR,"Instance [");
      WriteString(theEnv,STDERR,ins->name->contents);
      WriteString(theEnv,STDERR,"] is already being initialized.\n");
      SetEvaluationError(theEnv,true);
      return false;
     }

   /* =======================================================
      Replace all default-slot values with any slot-overrides
      ======================================================= */
   ins->busy++;
   ins->installed = 0;

   /* =================================================================
      If the slots are initialized properly - the initializeInProgress
      flag will be turned off.
      ================================================================= */
   ins->initializeInProgress = 1;
   ins->initSlotsCalled = 0;

   if (InsertSlotOverrides(theEnv,ins,ovrexp) == false)
      {
       ins->installed = 1;
       ins->busy--;
       return false;
      }

   /* =================================================================
      Now that all the slot expressions are established - replace them
      with their evaluation
      ================================================================= */

   if (InstanceData(theEnv)->MkInsMsgPass)
     DirectMessage(theEnv,MessageHandlerData(theEnv)->INIT_SYMBOL,ins,&temp,NULL);
   else
     EvaluateClassDefaults(theEnv,ins);

   ins->busy--;
   ins->installed = 1;
   if (EvaluationData(theEnv)->EvaluationError)
     {
      PrintErrorID(theEnv,"INSMNGR",8,false);
      WriteString(theEnv,STDERR,"An error occurred during the initialization of instance [");
      WriteString(theEnv,STDERR,ins->name->contents);
      WriteString(theEnv,STDERR,"].\n");
      return false;
     }

   ins->initializeInProgress = 0;
   return (ins->initSlotsCalled == 0) ? false : true;
  }

/*******************************************************************
  NAME         : CoreInitializeInstanceCV
  DESCRIPTION  : Performs the core work for initializing an instance
  INPUTS       : 1) The instance address
                 2) Slot override CLIPSValues
  RETURNS      : True if all OK, false otherwise
  SIDE EFFECTS : EvaluationError set on errors - slots evaluated
  NOTES        : None
 *******************************************************************/
static bool CoreInitializeInstanceCV(
  Environment *theEnv,
  Instance *ins,
  CLIPSValue *overrides)
  {
   UDFValue temp;

   if (ins->installed == 0)
     {
      PrintErrorID(theEnv,"INSMNGR",7,false);
      WriteString(theEnv,STDERR,"Instance ");
      WriteString(theEnv,STDERR,ins->name->contents);
      WriteString(theEnv,STDERR," is already being initialized.\n");
      SetEvaluationError(theEnv,true);
      return false;
     }
     
   /*==========================================================*/
   /* Replace all default-slot values with any slot-overrides. */
   /*==========================================================*/
   
   ins->busy++;
   ins->installed = 0;
   
   /*===============================================*/
   /* If the slots are initialized properly - the   */
   /* initializeInProgress flag will be turned off. */
   /*===============================================*/
   
   ins->initializeInProgress = 1;
   ins->initSlotsCalled = 0;

   if (InsertSlotOverridesCV(theEnv,ins,overrides) == false)
      {
       ins->installed = 1;
       ins->busy--;
       return false;
      }

   /*====================================================*/
   /* Now that all the slot expressions are established, */
   /* replace them  with their evaluation.               */
   /*====================================================*/

   if (InstanceData(theEnv)->MkInsMsgPass)
     { DirectMessage(theEnv,MessageHandlerData(theEnv)->INIT_SYMBOL,ins,&temp,NULL); }
   else
     { EvaluateClassDefaults(theEnv,ins); }

   ins->busy--;
   ins->installed = 1;
   if (EvaluationData(theEnv)->EvaluationError)
     {
      PrintErrorID(theEnv,"INSMNGR",8,false);
      WriteString(theEnv,STDERR,"An error occurred during the initialization of instance [");
      WriteString(theEnv,STDERR,ins->name->contents);
      WriteString(theEnv,STDERR,"].\n");
      return false;
     }
     
   ins->initializeInProgress = 0;
   return (ins->initSlotsCalled == 0) ? false : true;
  }

/**********************************************************
  NAME         : InsertSlotOverrides
  DESCRIPTION  : Replaces value-expression for a slot
  INPUTS       : 1) The instance address
                 2) The address of the beginning of the
                    list of slot-expressions
  RETURNS      : True if all okay, false otherwise
  SIDE EFFECTS : Old slot expression deallocated
  NOTES        : Assumes symbols not yet installed
                 EVALUATES the slot-name expression but
                    simply copies the slot value-expression
 **********************************************************/
static bool InsertSlotOverrides(
  Environment *theEnv,
  Instance *ins,
  Expression *slot_exp)
  {
   InstanceSlot *slot;
   UDFValue temp, junk;

   EvaluationData(theEnv)->EvaluationError = false;
   while (slot_exp != NULL)
     {
      if ((EvaluateExpression(theEnv,slot_exp,&temp) == true) ? true :
          (temp.header->type != SYMBOL_TYPE))
        {
         PrintErrorID(theEnv,"INSMNGR",9,false);
         WriteString(theEnv,STDERR,"Expected a valid slot name for slot-override.\n");
         SetEvaluationError(theEnv,true);
         return false;
        }
      slot = FindInstanceSlot(theEnv,ins,temp.lexemeValue);
      if (slot == NULL)
        {
         PrintErrorID(theEnv,"INSMNGR",13,false);
         WriteString(theEnv,STDERR,"Slot '");
         WriteString(theEnv,STDERR,temp.lexemeValue->contents);
         WriteString(theEnv,STDERR,"' does not exist in instance [");
         WriteString(theEnv,STDERR,ins->name->contents);
         WriteString(theEnv,STDERR,"].\n");
         SetEvaluationError(theEnv,true);
         return false;
        }

      if (InstanceData(theEnv)->MkInsMsgPass)
        { DirectMessage(theEnv,slot->desc->overrideMessage,
                       ins,NULL,slot_exp->nextArg->argList); }
      else if (slot_exp->nextArg->argList)
        {
         if (EvaluateAndStoreInDataObject(theEnv,slot->desc->multiple,
                               slot_exp->nextArg->argList,&temp,true))
             PutSlotValue(theEnv,ins,slot,&temp,&junk,"function make-instance");
        }
      else
        {
         temp.begin = 0;
         temp.range = 0;
         temp.value = ProceduralPrimitiveData(theEnv)->NoParamValue;
         PutSlotValue(theEnv,ins,slot,&temp,&junk,"function make-instance");
        }

      if (EvaluationData(theEnv)->EvaluationError)
        return false;
      slot->override = true;
      slot_exp = slot_exp->nextArg->nextArg;
     }
   return true;
  }

/**********************************************************
  NAME         : InsertSlotOverridesCV
  DESCRIPTION  : Replaces value-expression for a slot
  INPUTS       : 1) The instance address
                 2) The address of the beginning of the
                    list of slot-expressions
  RETURNS      : True if all okay, false otherwise
  SIDE EFFECTS : Old slot expression deallocated
  NOTES        : Assumes symbols not yet installed
                 EVALUATES the slot-name expression but
                    simply copies the slot value-expression
 **********************************************************/
static bool InsertSlotOverridesCV(
  Environment *theEnv,
  Instance *ins,
  CLIPSValue *overrides)
  {
   unsigned int i;
   InstanceSlot *slot;
   UDFValue temp, junk;

   EvaluationData(theEnv)->EvaluationError = false;

   for (i = 0; i < ins->cls->slotCount; i++)
     {
      if (overrides[i].value == VoidConstant(theEnv)) continue;
      
      slot = ins->slotAddresses[i];
      CLIPSToUDFValue(&overrides[i],&temp);
      PutSlotValue(theEnv,ins,slot,&temp,&junk,"InstanceBuilder call");
      
      if (EvaluationData(theEnv)->EvaluationError)
        { return false; }
        
      slot->override = true;
     }
     
   return true;
  }

/*****************************************************************************
  NAME         : EvaluateClassDefaults
  DESCRIPTION  : Evaluates default slots only - slots that were specified
                 by overrides (sp->override == 1) are ignored)
  INPUTS       : 1) Instance address
  RETURNS      : Nothing useful
  SIDE EFFECTS : Each UDFValue slot in the instance's slot array is replaced
                   by the evaluation (by EvaluateExpression) of the expression
                   in the slot list.  The old expression-values
                   are deleted.
  NOTES        : None
 *****************************************************************************/
static void EvaluateClassDefaults(
  Environment *theEnv,
  Instance *ins)
  {
   InstanceSlot *slot;
   UDFValue temp,junk;
   unsigned long i;

   if (ins->initializeInProgress == 0)
     {
      PrintErrorID(theEnv,"INSMNGR",15,false);
      SetEvaluationError(theEnv,true);
      WriteString(theEnv,STDERR,"init-slots not valid in this context.\n");
      return;
     }
   for (i = 0 ; i < ins->cls->instanceSlotCount ; i++)
     {
      slot = ins->slotAddresses[i];

      /* ===========================================================
         Slot-overrides are just a short-hand for put-slots, so they
         should be done with messages.  Defaults are from the class
         definition and can be placed directly.
         =========================================================== */
      if (!slot->override)
        {
         if (slot->desc->dynamicDefault)
           {
            if (EvaluateAndStoreInDataObject(theEnv,slot->desc->multiple,
                                             (Expression *) slot->desc->defaultValue,
                                             &temp,true))
              PutSlotValue(theEnv,ins,slot,&temp,&junk,"function init-slots");
           }
         else if (((slot->desc->shared == 0) || (slot->desc->sharedCount == 1)) &&
                  (slot->desc->noDefault == 0))
           DirectPutSlotValue(theEnv,ins,slot,(UDFValue *) slot->desc->defaultValue,&junk);
         else if (slot->valueRequired)
           {
            PrintErrorID(theEnv,"INSMNGR",14,false);
            WriteString(theEnv,STDERR,"Override required for slot '");
            WriteString(theEnv,STDERR,slot->desc->slotName->name->contents);
            WriteString(theEnv,STDERR,"' in instance [");
            WriteString(theEnv,STDERR,ins->name->contents);
            WriteString(theEnv,STDERR,"].\n");
            SetEvaluationError(theEnv,true);
           }
         slot->valueRequired = false;
         if (ins->garbage == 1)
           {
            WriteString(theEnv,STDERR,ins->name->contents);
            WriteString(theEnv,STDERR," instance deleted by slot-override evaluation.\n");
            SetEvaluationError(theEnv,true);
           }
         if (EvaluationData(theEnv)->EvaluationError)
            return;
        }
      slot->override = false;
     }
   ins->initSlotsCalled = 1;
  }

#if DEBUGGING_FUNCTIONS

/***************************************************
  NAME         : PrintInstanceWatch
  DESCRIPTION  : Prints out a trace message for the
                 creation/deletion of an instance
  INPUTS       : 1) The trace string indicating if
                    this is a creation or deletion
                 2) The instance
  RETURNS      : Nothing usful
  SIDE EFFECTS : Watch message printed
  NOTES        : None
 ***************************************************/
static void PrintInstanceWatch(
  Environment *theEnv,
  const char *traceString,
  Instance *theInstance)
  {
   if (ConstructData(theEnv)->ClearReadyInProgress ||
       ConstructData(theEnv)->ClearInProgress)
     { return; }

   WriteString(theEnv,STDOUT,traceString);
   WriteString(theEnv,STDOUT," instance ");
   PrintInstanceNameAndClass(theEnv,STDOUT,theInstance,true);
  }

#endif

/**************************/
/* CreateInstanceBuilder: */
/**************************/
InstanceBuilder *CreateInstanceBuilder(
  Environment *theEnv,
  const char *defclassName)
  {
   InstanceBuilder *theIB;
   Defclass *theDefclass;
   unsigned int i;
   
   if (theEnv == NULL) return NULL;
      
   if (defclassName != NULL)
     {
      theDefclass = FindDefclass(theEnv,defclassName);
      if (theDefclass == NULL)
        {
         InstanceData(theEnv)->instanceBuilderError = IBE_DEFCLASS_NOT_FOUND_ERROR;
         return NULL;
        }
     }
   else
     { theDefclass = NULL; }
     
   theIB = get_struct(theEnv,instanceBuilder);
   theIB->ibEnv = theEnv;
   theIB->ibDefclass = theDefclass;
      
   if ((theDefclass == NULL) || (theDefclass->slotCount == 0))
     { theIB->ibValueArray = NULL; }
   else
     {
      theIB->ibValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * theDefclass->slotCount);
      
      for (i = 0; i < theDefclass->slotCount; i++)
        { theIB->ibValueArray[i].voidValue = VoidConstant(theEnv); }
     }

   InstanceData(theEnv)->instanceBuilderError = IBE_NO_ERROR;

   return theIB;
  }

/*************************/
/* IBPutSlotCLIPSInteger */
/*************************/
PutSlotError IBPutSlotCLIPSInteger(
  InstanceBuilder *theIB,
  const char *slotName,
  CLIPSInteger *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.integerValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/********************/
/* IBPutSlotInteger */
/********************/
PutSlotError IBPutSlotInteger(
  InstanceBuilder *theIB,
  const char *slotName,
  long long longLongValue)
  {
   CLIPSValue theValue;
   
   if (theIB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.integerValue = CreateInteger(theIB->ibEnv,longLongValue);
   return IBPutSlot(theIB,slotName,&theValue);
  }

/************************/
/* IBPutSlotCLIPSLexeme */
/************************/
PutSlotError IBPutSlotCLIPSLexeme(
  InstanceBuilder *theIB,
  const char *slotName,
  CLIPSLexeme *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.lexemeValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/*******************/
/* IBPutSlotSymbol */
/*******************/
PutSlotError IBPutSlotSymbol(
  InstanceBuilder *theIB,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theIB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateSymbol(theIB->ibEnv,stringValue);
   return IBPutSlot(theIB,slotName,&theValue);
  }

/*******************/
/* IBPutSlotString */
/*******************/
PutSlotError IBPutSlotString(
  InstanceBuilder *theIB,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theIB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateString(theIB->ibEnv,stringValue);
   return IBPutSlot(theIB,slotName,&theValue);
  }

/*************************/
/* IBPutSlotInstanceName */
/*************************/
PutSlotError IBPutSlotInstanceName(
  InstanceBuilder *theIB,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theIB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

   theValue.lexemeValue = CreateInstanceName(theIB->ibEnv,stringValue);
   return IBPutSlot(theIB,slotName,&theValue);
  }

/***********************/
/* IBPutSlotCLIPSFloat */
/***********************/
PutSlotError IBPutSlotCLIPSFloat(
  InstanceBuilder *theIB,
  const char *slotName,
  CLIPSFloat *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.floatValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/******************/
/* IBPutSlotFloat */
/******************/
PutSlotError IBPutSlotFloat(
  InstanceBuilder *theIB,
  const char *slotName,
  double doubleValue)
  {
   CLIPSValue theValue;
   
    if (theIB == NULL)
     { return PSE_NULL_POINTER_ERROR; }

  theValue.floatValue = CreateFloat(theIB->ibEnv,doubleValue);
   return IBPutSlot(theIB,slotName,&theValue);
  }

/*****************/
/* IBPutSlotFact */
/*****************/
PutSlotError IBPutSlotFact(
  InstanceBuilder *theIB,
  const char *slotName,
  Fact *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.factValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/*********************/
/* IBPutSlotInstance */
/*********************/
PutSlotError IBPutSlotInstance(
  InstanceBuilder *theIB,
  const char *slotName,
  Instance *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.instanceValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/****************************/
/* IBPutSlotExternalAddress */
/****************************/
PutSlotError IBPutSlotExternalAddress(
  InstanceBuilder *theIB,
  const char *slotName,
  CLIPSExternalAddress *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.externalAddressValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/***********************/
/* IBPutSlotMultifield */
/***********************/
PutSlotError IBPutSlotMultifield(
  InstanceBuilder *theIB,
  const char *slotName,
  Multifield *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.multifieldValue = slotValue;
   return IBPutSlot(theIB,slotName,&theValue);
  }

/**************/
/* IBPutSlot: */
/**************/
PutSlotError IBPutSlot(
  InstanceBuilder *theIB,
  const char *slotName,
  CLIPSValue *slotValue)
  {
   Environment *theEnv;
   int whichSlot;
   CLIPSValue oldValue;
   unsigned int i;
   SlotDescriptor *theSlot;
   ConstraintViolationType cvType;
   
   /*==========================*/
   /* Check for NULL pointers. */
   /*==========================*/
   
   if ((theIB == NULL) || (slotName == NULL) || (slotValue == NULL))
     { return PSE_NULL_POINTER_ERROR; }
     
   if ((theIB->ibDefclass == NULL) || (slotValue->value == NULL))
     { return PSE_NULL_POINTER_ERROR; }
   
   theEnv = theIB->ibEnv;
     
   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   whichSlot = FindInstanceTemplateSlot(theEnv,theIB->ibDefclass,CreateSymbol(theIB->ibEnv,slotName));
   if (whichSlot == -1)
     { return PSE_SLOT_NOT_FOUND_ERROR; }
   theSlot = theIB->ibDefclass->instanceTemplate[whichSlot];
   
   /*=============================================*/
   /* Make sure a single field value is not being */
   /* stored in a multifield slot or vice versa.  */
   /*=============================================*/

   if (((theSlot->multiple == 0) && (slotValue->header->type == MULTIFIELD_TYPE)) ||
       ((theSlot->multiple == 1) && (slotValue->header->type != MULTIFIELD_TYPE)))
     { return PSE_CARDINALITY_ERROR; }
     
   /*=================================*/
   /* Check constraints for the slot. */
   /*=================================*/

   if (theSlot->constraint != NULL)
     {
      if ((cvType = ConstraintCheckValue(theEnv,slotValue->header->type,slotValue->value,theSlot->constraint)) != NO_VIOLATION)
        {
         switch(cvType)
           {
            case NO_VIOLATION:
            case FUNCTION_RETURN_TYPE_VIOLATION:
              SystemError(theEnv,"INSMNGR",1);
              ExitRouter(theEnv,EXIT_FAILURE);
              break;
              
            case TYPE_VIOLATION:
              return PSE_TYPE_ERROR;
              
            case RANGE_VIOLATION:
              return PSE_RANGE_ERROR;
              
            case ALLOWED_VALUES_VIOLATION:
              return PSE_ALLOWED_VALUES_ERROR;
              
            case CARDINALITY_VIOLATION:
              return PSE_CARDINALITY_ERROR;
            
            case ALLOWED_CLASSES_VIOLATION:
              return PSE_ALLOWED_CLASSES_ERROR;
           }
        }
     }

   /*===================================*/
   /* Create the value array if needed. */
   /*===================================*/
     
   if (theIB->ibValueArray == NULL)
     {
      theIB->ibValueArray = (CLIPSValue *) gm2(theIB->ibEnv,sizeof(CLIPSValue) * theIB->ibDefclass->slotCount);
      for (i = 0; i < theIB->ibDefclass->slotCount; i++)
        { theIB->ibValueArray[i].voidValue = theIB->ibEnv->VoidConstant; }
     }

   /*=====================*/
   /* Set the slot value. */
   /*=====================*/
   
   oldValue.value = theIB->ibValueArray[whichSlot].value;
   
   if (oldValue.header->type == MULTIFIELD_TYPE)
     {
      if (MultifieldsEqual(oldValue.multifieldValue,slotValue->multifieldValue))
        { return PSE_NO_ERROR; }
     }
   else
     {
      if (oldValue.value == slotValue->value)
        { return PSE_NO_ERROR; }
     }
   
   Release(theEnv,oldValue.header);
   
   if (oldValue.header->type == MULTIFIELD_TYPE)
     { ReturnMultifield(theEnv,oldValue.multifieldValue); }

   if (slotValue->header->type == MULTIFIELD_TYPE)
     { theIB->ibValueArray[whichSlot].multifieldValue = CopyMultifield(theEnv,slotValue->multifieldValue); }
   else
     { theIB->ibValueArray[whichSlot].value = slotValue->value; }
      
   Retain(theEnv,theIB->ibValueArray[whichSlot].header);
   
   return PSE_NO_ERROR;
  }

/***********/
/* IBMake: */
/***********/
Instance *IBMake(
  InstanceBuilder *theIB,
  const char *instanceName)
  {
   Environment *theEnv;
   Instance *theInstance;
   CLIPSLexeme *instanceLexeme;
   UDFValue rv;
   unsigned int i;
   bool ov;
   
   if (theIB == NULL) return NULL;
   theEnv = theIB->ibEnv;
   
   if (theIB->ibDefclass == NULL)
     {
      InstanceData(theEnv)->instanceBuilderError = IBE_NULL_POINTER_ERROR;
      return NULL;
     }
     
   if (instanceName == NULL)
     {
      GensymStar(theEnv,&rv);
      instanceLexeme = CreateInstanceName(theEnv,rv.lexemeValue->contents);
     }
   else
     { instanceLexeme = CreateInstanceName(theEnv,instanceName); }
   
#if DEFRULE_CONSTRUCT
   ov = SetDelayObjectPatternMatching(theEnv,true);
#endif

   theInstance = BuildInstance(theEnv,instanceLexeme,theIB->ibDefclass,true);
   
   if (theInstance == NULL)
     {
      if (InstanceData(theEnv)->makeInstanceError == MIE_COULD_NOT_CREATE_ERROR)
        { InstanceData(theEnv)->instanceBuilderError = IBE_COULD_NOT_CREATE_ERROR; }
      else if (InstanceData(theEnv)->makeInstanceError == MIE_RULE_NETWORK_ERROR)
        { InstanceData(theEnv)->instanceBuilderError = IBE_RULE_NETWORK_ERROR; }
      else
        {
         SystemError(theEnv,"INSMNGR",3);
         ExitRouter(theEnv,EXIT_FAILURE);
        }
        
#if DEFRULE_CONSTRUCT
      SetDelayObjectPatternMatching(theEnv,ov);
#endif

      return NULL;
     }
   
   if (CoreInitializeInstanceCV(theIB->ibEnv,theInstance,theIB->ibValueArray) == false)
     {
      InstanceData(theEnv)->instanceBuilderError = IBE_COULD_NOT_CREATE_ERROR;
      QuashInstance(theIB->ibEnv,theInstance);
#if DEFRULE_CONSTRUCT
      SetDelayObjectPatternMatching(theEnv,ov);
#endif
      return NULL;
     }
   
#if DEFRULE_CONSTRUCT
   SetDelayObjectPatternMatching(theEnv,ov);
#endif

   for (i = 0; i < theIB->ibDefclass->slotCount; i++)
     {
      if (theIB->ibValueArray[i].voidValue != VoidConstant(theEnv))
        {
         Release(theEnv,theIB->ibValueArray[i].header);

         if (theIB->ibValueArray[i].header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theEnv,theIB->ibValueArray[i].multifieldValue); }

         theIB->ibValueArray[i].voidValue = VoidConstant(theEnv);
        }
     }
     
   InstanceData(theEnv)->instanceBuilderError = IBE_NO_ERROR;
   
   return theInstance;
  }

/**************/
/* IBDispose: */
/**************/
void IBDispose(
  InstanceBuilder *theIB)
  {
   Environment *theEnv;
   
   if (theIB == NULL) return;

   theEnv = theIB->ibEnv;
   
   IBAbort(theIB);
   
   if (theIB->ibValueArray != NULL)
     { rm(theEnv,theIB->ibValueArray,sizeof(CLIPSValue) * theIB->ibDefclass->slotCount); }
   
   rtn_struct(theEnv,instanceBuilder,theIB);
  }

/************/
/* IBAbort: */
/************/
void IBAbort(
  InstanceBuilder *theIB)
  {
   Environment *theEnv = theIB->ibEnv;
   unsigned int i;
   
   if (theIB == NULL) return;
   
   if (theIB->ibDefclass == NULL) return;
   
   theEnv = theIB->ibEnv;

   for (i = 0; i < theIB->ibDefclass->slotCount; i++)
     {
      Release(theEnv,theIB->ibValueArray[i].header);
      
      if (theIB->ibValueArray[i].header->type == MULTIFIELD_TYPE)
        { ReturnMultifield(theEnv,theIB->ibValueArray[i].multifieldValue); }
        
      theIB->ibValueArray[i].voidValue = VoidConstant(theEnv);
     }
  }
  
/*****************/
/* IBSetDefclass */
/*****************/
InstanceBuilderError IBSetDefclass(
  InstanceBuilder *theIB,
  const char *defclassName)
  {
   Defclass *theDefclass;
   Environment *theEnv;
   unsigned int i;
   
   if (theIB == NULL)
     { return IBE_NULL_POINTER_ERROR; }
      
   theEnv = theIB->ibEnv;

   IBAbort(theIB);
   
   if (defclassName != NULL)
     {
      theDefclass = FindDefclass(theIB->ibEnv,defclassName);
   
      if (theDefclass == NULL)
        {
         InstanceData(theEnv)->instanceBuilderError = IBE_DEFCLASS_NOT_FOUND_ERROR;
         return IBE_DEFCLASS_NOT_FOUND_ERROR;
        }
     }
   else
     { theDefclass = NULL; }

   if (theIB->ibValueArray != NULL)
     { rm(theEnv,theIB->ibValueArray,sizeof(CLIPSValue) * theIB->ibDefclass->slotCount); }

   theIB->ibDefclass = theDefclass;
   
   if ((theDefclass == NULL) || (theDefclass->slotCount == 0))
     { theIB->ibValueArray = NULL; }
   else
     {
      theIB->ibValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * theDefclass->slotCount);

      for (i = 0; i < theDefclass->slotCount; i++)
        { theIB->ibValueArray[i].voidValue = VoidConstant(theEnv); }
     }

   InstanceData(theEnv)->instanceBuilderError = IBE_NO_ERROR;
   return IBE_NO_ERROR;
  }

/************/
/* IBError: */
/************/
InstanceBuilderError IBError(
  Environment *theEnv)
  {
   return InstanceData(theEnv)->instanceBuilderError;
  }

/***************************/
/* CreateInstanceModifier: */
/***************************/
InstanceModifier *CreateInstanceModifier(
  Environment *theEnv,
  Instance *oldInstance)
  {
   InstanceModifier *theIM;
   unsigned int i;
   
   if (theEnv == NULL) return NULL;
   
   if (oldInstance != NULL)
     {
      if (oldInstance->garbage)
        {
         InstanceData(theEnv)->instanceModifierError = IME_DELETED_ERROR;
         return NULL;
        }
        
      RetainInstance(oldInstance);
     }

   theIM = get_struct(theEnv,instanceModifier);
   theIM->imEnv = theEnv;
   theIM->imOldInstance = oldInstance;

   if ((oldInstance == NULL) || (oldInstance->cls->slotCount == 0))
     {
      theIM->imValueArray = NULL;
      theIM->changeMap = NULL;
     }
   else
     {
      theIM->imValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * oldInstance->cls->slotCount);
      
      for (i = 0; i < oldInstance->cls->slotCount; i++)
        { theIM->imValueArray[i].voidValue = VoidConstant(theEnv); }

      theIM->changeMap = (char *) gm2(theEnv,CountToBitMapSize(oldInstance->cls->slotCount));
      ClearBitString((void *) theIM->changeMap,CountToBitMapSize(oldInstance->cls->slotCount));
     }

   InstanceData(theEnv)->instanceModifierError = IME_NO_ERROR;
   return theIM;
  }

/*************************/
/* IMPutSlotCLIPSInteger */
/*************************/
PutSlotError IMPutSlotCLIPSInteger(
  InstanceModifier *theFM,
  const char *slotName,
  CLIPSInteger *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.integerValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/********************/
/* IMPutSlotInteger */
/********************/
PutSlotError IMPutSlotInteger(
  InstanceModifier *theIM,
  const char *slotName,
  long long longLongValue)
  {
   CLIPSValue theValue;
   
   if (theIM == NULL)
     { return PSE_NULL_POINTER_ERROR; }
     
   theValue.integerValue = CreateInteger(theIM->imEnv,longLongValue);
   return IMPutSlot(theIM,slotName,&theValue);
  }

/************************/
/* IMPutSlotCLIPSLexeme */
/************************/
PutSlotError IMPutSlotCLIPSLexeme(
  InstanceModifier *theFM,
  const char *slotName,
  CLIPSLexeme *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.lexemeValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/*******************/
/* IMPutSlotSymbol */
/*******************/
PutSlotError IMPutSlotSymbol(
  InstanceModifier *theIM,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theIM == NULL)
     { return PSE_NULL_POINTER_ERROR; }
     
   theValue.lexemeValue = CreateSymbol(theIM->imEnv,stringValue);
   return IMPutSlot(theIM,slotName,&theValue);
  }

/*******************/
/* IMPutSlotString */
/*******************/
PutSlotError IMPutSlotString(
  InstanceModifier *theIM,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theIM == NULL)
     { return PSE_NULL_POINTER_ERROR; }
     
   theValue.lexemeValue = CreateString(theIM->imEnv,stringValue);
   return IMPutSlot(theIM,slotName,&theValue);
  }

/*************************/
/* IMPutSlotInstanceName */
/*************************/
PutSlotError IMPutSlotInstanceName(
  InstanceModifier *theIM,
  const char *slotName,
  const char *stringValue)
  {
   CLIPSValue theValue;
   
   if (theIM == NULL)
     { return PSE_NULL_POINTER_ERROR; }
     
   theValue.lexemeValue = CreateInstanceName(theIM->imEnv,stringValue);
   return IMPutSlot(theIM,slotName,&theValue);
  }

/***********************/
/* IMPutSlotCLIPSFloat */
/***********************/
PutSlotError IMPutSlotCLIPSFloat(
  InstanceModifier *theFM,
  const char *slotName,
  CLIPSFloat *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.floatValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/******************/
/* IMPutSlotFloat */
/******************/
PutSlotError IMPutSlotFloat(
  InstanceModifier *theIM,
  const char *slotName,
  double doubleValue)
  {
   CLIPSValue theValue;
   
   if (theIM == NULL)
     { return PSE_NULL_POINTER_ERROR; }
     
   theValue.floatValue = CreateFloat(theIM->imEnv,doubleValue);
   return IMPutSlot(theIM,slotName,&theValue);
  }

/*****************/
/* IMPutSlotFact */
/*****************/
PutSlotError IMPutSlotFact(
  InstanceModifier *theFM,
  const char *slotName,
  Fact *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.factValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/*********************/
/* IMPutSlotInstance */
/*********************/
PutSlotError IMPutSlotInstance(
  InstanceModifier *theFM,
  const char *slotName,
  Instance *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.instanceValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/****************************/
/* IMPutSlotExternalAddress */
/****************************/
PutSlotError IMPutSlotExternalAddress(
  InstanceModifier *theFM,
  const char *slotName,
  CLIPSExternalAddress *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.externalAddressValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/***********************/
/* IMPutSlotMultifield */
/***********************/
PutSlotError IMPutSlotMultifield(
  InstanceModifier *theFM,
  const char *slotName,
  Multifield *slotValue)
  {
   CLIPSValue theValue;
   
   theValue.multifieldValue = slotValue;
   return IMPutSlot(theFM,slotName,&theValue);
  }

/**************/
/* IMPutSlot: */
/**************/
PutSlotError IMPutSlot(
  InstanceModifier *theIM,
  const char *slotName,
  CLIPSValue *slotValue)
  {
   Environment *theEnv;
   int whichSlot;
   CLIPSValue oldValue;
   CLIPSValue oldInstanceValue;
   unsigned int i;
   SlotDescriptor *theSlot;
   ConstraintViolationType cvType;
   
   /*==========================*/
   /* Check for NULL pointers. */
   /*==========================*/

   if ((theIM == NULL) || (slotName == NULL) || (slotValue == NULL))
     { return PSE_NULL_POINTER_ERROR; }
     
   if ((theIM->imOldInstance == NULL) || (slotValue->value == NULL))
     { return PSE_NULL_POINTER_ERROR; }
   
   theEnv = theIM->imEnv;

   /*======================================*/
   /* Deleted instances can't be modified. */
   /*======================================*/
   
   if (theIM->imOldInstance->garbage)
     { return PSE_INVALID_TARGET_ERROR; }

   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   whichSlot = FindInstanceTemplateSlot(theEnv,theIM->imOldInstance->cls,CreateSymbol(theIM->imEnv,slotName));
   if (whichSlot == -1)
     { return PSE_SLOT_NOT_FOUND_ERROR; }
   theSlot = theIM->imOldInstance->cls->instanceTemplate[whichSlot];

   /*=============================================*/
   /* Make sure a single field value is not being */
   /* stored in a multifield slot or vice versa.  */
   /*=============================================*/

   if (((theSlot->multiple == 0) && (slotValue->header->type == MULTIFIELD_TYPE)) ||
       ((theSlot->multiple == 1) && (slotValue->header->type != MULTIFIELD_TYPE)))
     { return PSE_CARDINALITY_ERROR; }
     
   /*=================================*/
   /* Check constraints for the slot. */
   /*=================================*/

   if (theSlot->constraint != NULL)
     {
      if ((cvType = ConstraintCheckValue(theEnv,slotValue->header->type,slotValue->value,theSlot->constraint)) != NO_VIOLATION)
        {
         switch(cvType)
           {
            case NO_VIOLATION:
            case FUNCTION_RETURN_TYPE_VIOLATION:
              SystemError(theEnv,"INSMNGR",2);
              ExitRouter(theEnv,EXIT_FAILURE);
              break;
              
            case TYPE_VIOLATION:
              return PSE_TYPE_ERROR;
              
            case RANGE_VIOLATION:
              return PSE_RANGE_ERROR;
              
            case ALLOWED_VALUES_VIOLATION:
              return PSE_ALLOWED_VALUES_ERROR;
              
            case CARDINALITY_VIOLATION:
              return PSE_CARDINALITY_ERROR;
            
            case ALLOWED_CLASSES_VIOLATION:
              return PSE_ALLOWED_CLASSES_ERROR;
           }
        }
     }

   /*===========================*/
   /* Set up the change arrays. */
   /*===========================*/

   if (theIM->imValueArray == NULL)
     {
      theIM->imValueArray = (CLIPSValue *) gm2(theIM->imEnv,sizeof(CLIPSValue) * theIM->imOldInstance->cls->slotCount);
      for (i = 0; i < theIM->imOldInstance->cls->slotCount; i++)
        { theIM->imValueArray[i].voidValue = theIM->imEnv->VoidConstant; }
     }

   if (theIM->changeMap == NULL)
     {
      theIM->changeMap = (char *) gm2(theIM->imEnv,CountToBitMapSize(theIM->imOldInstance->cls->slotCount));
      ClearBitString((void *) theIM->changeMap,CountToBitMapSize(theIM->imOldInstance->cls->slotCount));
     }
     
   /*=====================*/
   /* Set the slot value. */
   /*=====================*/

   oldValue.value = theIM->imValueArray[whichSlot].value;
   oldInstanceValue.value = theIM->imOldInstance->slotAddresses[whichSlot]->value;

   if (oldInstanceValue.header->type == MULTIFIELD_TYPE)
     {
      if (MultifieldsEqual(oldInstanceValue.multifieldValue,slotValue->multifieldValue))
        {
         Release(theIM->imEnv,oldValue.header);
         if (oldValue.header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theIM->imEnv,oldValue.multifieldValue); }
         theIM->imValueArray[whichSlot].voidValue = theIM->imEnv->VoidConstant;
         ClearBitMap(theIM->changeMap,whichSlot);
         return PSE_NO_ERROR;
        }

      if (MultifieldsEqual(oldValue.multifieldValue,slotValue->multifieldValue))
        { return PSE_NO_ERROR; }
     }
   else
     {
      if (slotValue->value == oldInstanceValue.value)
        {
         Release(theIM->imEnv,oldValue.header);
         theIM->imValueArray[whichSlot].voidValue = theIM->imEnv->VoidConstant;
         ClearBitMap(theIM->changeMap,whichSlot);
         return PSE_NO_ERROR;
        }
        
      if (oldValue.value == slotValue->value)
        { return PSE_NO_ERROR; }
     }

   SetBitMap(theIM->changeMap,whichSlot);

   Release(theIM->imEnv,oldValue.header);

   if (oldValue.header->type == MULTIFIELD_TYPE)
     { ReturnMultifield(theIM->imEnv,oldValue.multifieldValue); }
      
   if (slotValue->header->type == MULTIFIELD_TYPE)
     { theIM->imValueArray[whichSlot].multifieldValue = CopyMultifield(theIM->imEnv,slotValue->multifieldValue); }
   else
     { theIM->imValueArray[whichSlot].value = slotValue->value; }

   Retain(theIM->imEnv,theIM->imValueArray[whichSlot].header);

   return PSE_NO_ERROR;
  }

/*************/
/* IMModify: */
/*************/
Instance *IMModify(
  InstanceModifier *theIM)
  {
   Environment *theEnv;
#if DEFRULE_CONSTRUCT
   bool ov;
#endif

   if (theIM == NULL)
     { return NULL; }
     
   theEnv = theIM->imEnv;
   
   if (theIM->imOldInstance == NULL)
     {
      InstanceData(theEnv)->instanceModifierError = IME_NULL_POINTER_ERROR;
      return NULL;
     }

   if (theIM->imOldInstance->garbage)
     {
      InstanceData(theEnv)->instanceModifierError = IME_DELETED_ERROR;
      return NULL;
     }

   if (theIM->changeMap == NULL)
     { return theIM->imOldInstance; }
     
   if (! BitStringHasBitsSet(theIM->changeMap,CountToBitMapSize(theIM->imOldInstance->cls->slotCount)))
     { return theIM->imOldInstance; }

#if DEFRULE_CONSTRUCT
   ov = SetDelayObjectPatternMatching(theIM->imEnv,true);
#endif

   IMModifySlots(theIM->imEnv,theIM->imOldInstance,theIM->imValueArray);

   if ((InstanceData(theEnv)->makeInstanceError == MIE_RULE_NETWORK_ERROR) ||
       (InstanceData(theEnv)->unmakeInstanceError == UIE_RULE_NETWORK_ERROR))
     { InstanceData(theEnv)->instanceModifierError = IME_RULE_NETWORK_ERROR; }
   else if ((InstanceData(theEnv)->unmakeInstanceError == UIE_COULD_NOT_DELETE_ERROR) ||
            (InstanceData(theEnv)->makeInstanceError == MIE_COULD_NOT_CREATE_ERROR))
     { InstanceData(theEnv)->instanceModifierError = IME_COULD_NOT_MODIFY_ERROR; }
   else
     { InstanceData(theEnv)->instanceModifierError = IME_NO_ERROR; }

#if DEFRULE_CONSTRUCT
   SetDelayObjectPatternMatching(theIM->imEnv,ov);
#endif

   IMAbort(theIM);
   
   return theIM->imOldInstance;
  }

/*****************/
/* IMModifySlots */
/*****************/
static bool IMModifySlots(
  Environment *theEnv,
  Instance *theInstance,
  CLIPSValue *overrides)
  {
   UDFValue temp, junk;
   InstanceSlot *insSlot;
   unsigned int i;

   for (i = 0; i < theInstance->cls->slotCount; i++)
     {
      if (overrides[i].value == VoidConstant(theEnv))
        { continue; }
        
      insSlot = theInstance->slotAddresses[i];
      
      if (insSlot->desc->multiple && (overrides[i].header->type != MULTIFIELD_TYPE))
        {
         temp.value = CreateMultifield(theEnv,1L);
         temp.begin = 0;
         temp.range = 1;
         temp.multifieldValue->contents[0].value = overrides[i].value;
        }
      else
        { CLIPSToUDFValue(&overrides[i],&temp); }
        
      if (PutSlotValue(theEnv,theInstance,insSlot,&temp,&junk,"InstanceModifier call") != PSE_NO_ERROR)
        { return false; }
     }
     
   return true;
  }

/**************/
/* IMDispose: */
/**************/
void IMDispose(
  InstanceModifier *theIM)
  {
   Environment *theEnv = theIM->imEnv;
   unsigned int i;

   /*========================*/
   /* Clear the value array. */
   /*========================*/
   
   if (theIM->imOldInstance != NULL)
     {
      for (i = 0; i < theIM->imOldInstance->cls->slotCount; i++)
        {
         Release(theEnv,theIM->imValueArray[i].header);

         if (theIM->imValueArray[i].header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theEnv,theIM->imValueArray[i].multifieldValue); }
        }
     }

   /*=====================================*/
   /* Return the value and change arrays. */
   /*=====================================*/
   
   if (theIM->imValueArray != NULL)
     { rm(theEnv,theIM->imValueArray,sizeof(CLIPSValue) * theIM->imOldInstance->cls->slotCount); }
      
   if (theIM->changeMap != NULL)
     { rm(theEnv,(void *) theIM->changeMap,CountToBitMapSize(theIM->imOldInstance->cls->slotCount)); }

   /*========================================*/
   /* Return the InstanceModifier structure. */
   /*========================================*/
   
   ReleaseInstance(theIM->imOldInstance);
   
   rtn_struct(theEnv,instanceModifier,theIM);
  }

/************/
/* IMAbort: */
/************/
void IMAbort(
  InstanceModifier *theIM)
  {
   GCBlock gcb;
   Environment *theEnv;
   unsigned int i;
   
   if (theIM == NULL) return;
   
   if (theIM->imOldInstance == NULL) return;
   
   theEnv = theIM->imEnv;
   
   GCBlockStart(theEnv,&gcb);

   for (i = 0; i < theIM->imOldInstance->cls->slotCount; i++)
     {
      Release(theEnv,theIM->imValueArray[i].header);

      if (theIM->imValueArray[i].header->type == MULTIFIELD_TYPE)
        { ReturnMultifield(theEnv,theIM->imValueArray[i].multifieldValue); }
        
      theIM->imValueArray[i].voidValue = theIM->imEnv->VoidConstant;
     }
        
   if (theIM->changeMap != NULL)
     { ClearBitString((void *) theIM->changeMap,CountToBitMapSize(theIM->imOldInstance->cls->slotCount)); }
     
   GCBlockEnd(theEnv,&gcb);
  }

/******************/
/* IMSetInstance: */
/******************/
InstanceModifierError IMSetInstance(
  InstanceModifier *theIM,
  Instance *oldInstance)
  {
   Environment *theEnv;
   unsigned int currentSlotCount, newSlotCount;
   unsigned int i;
      
   if (theIM == NULL)
     { return IME_NULL_POINTER_ERROR; }
     
   theEnv = theIM->imEnv;

   /*=============================================*/
   /* Modifiers can only be created for instances */
   /* that have not been deleted.                 */
   /*=============================================*/
   
   if (oldInstance != NULL)
     {
      if (oldInstance->garbage)
        {
         InstanceData(theEnv)->instanceModifierError = IME_DELETED_ERROR;
         return IME_DELETED_ERROR;
        }
     }

   /*========================*/
   /* Clear the value array. */
   /*========================*/
   
   if (theIM->imValueArray != NULL)
     {
      for (i = 0; i < theIM->imOldInstance->cls->slotCount; i++)
        {
         Release(theEnv,theIM->imValueArray[i].header);

         if (theIM->imValueArray[i].header->type == MULTIFIELD_TYPE)
           { ReturnMultifield(theEnv,theIM->imValueArray[i].multifieldValue); }
        }
     }

   /*==================================================*/
   /* Resize the value and change arrays if necessary. */
   /*==================================================*/
   
   if (theIM->imOldInstance == NULL)
     { currentSlotCount = 0; }
   else
     { currentSlotCount = theIM->imOldInstance->cls->slotCount; }

   if (oldInstance == NULL)
     { newSlotCount = 0; }
   else
     { newSlotCount = oldInstance->cls->slotCount; }

   if (newSlotCount != currentSlotCount)
     {
      if (theIM->imValueArray != NULL)
        { rm(theEnv,theIM->imValueArray,sizeof(CLIPSValue) * currentSlotCount); }
      
      if (theIM->changeMap != NULL)
        { rm(theEnv,(void *) theIM->changeMap,currentSlotCount); }
      
      if (oldInstance->cls->slotCount == 0)
        {
         theIM->imValueArray = NULL;
         theIM->changeMap = NULL;
        }
      else
        {
         theIM->imValueArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * newSlotCount);
         theIM->changeMap = (char *) gm2(theEnv,CountToBitMapSize(newSlotCount));
        }
     }
   
   /*=================================*/
   /* Update the fact being modified. */
   /*=================================*/
   
   RetainInstance(oldInstance);
   ReleaseInstance(theIM->imOldInstance);
   theIM->imOldInstance = oldInstance;
   
   /*=========================================*/
   /* Initialize the value and change arrays. */
   /*=========================================*/
   
   for (i = 0; i < theIM->imOldInstance->cls->slotCount; i++)
     { theIM->imValueArray[i].voidValue = theIM->imEnv->VoidConstant; }
   
   if (newSlotCount != 0)
     { ClearBitString((void *) theIM->changeMap,CountToBitMapSize(theIM->imOldInstance->cls->slotCount)); }

   /*================================================================*/
   /* Return true to indicate the modifier was successfully created. */
   /*================================================================*/
   
   InstanceData(theEnv)->instanceModifierError = IME_NO_ERROR;
   return IME_NO_ERROR;
  }

/************/
/* IMError: */
/************/
InstanceModifierError IMError(
  Environment *theEnv)
  {
   return InstanceData(theEnv)->instanceModifierError;
  }

#endif



