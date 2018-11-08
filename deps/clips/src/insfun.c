   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  11/07/17             */
   /*                                                     */
   /*               INSTANCE FUNCTIONS MODULE             */
   /*******************************************************/

/*************************************************************/
/* Purpose:  Internal instance manipulation routines         */
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
/*            Changed name of variable log to logName        */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*            Changed name of variable exp to theExp         */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Link error occurs for the SlotExistError       */
/*            function when OBJECT_SYSTEM is set to 0 in     */
/*            setup.h. DR0865                                */
/*                                                           */
/*            Converted INSTANCE_PATTERN_MATCHING to         */
/*            DEFRULE_CONSTRUCT.                             */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Moved EvaluateAndStoreInDataObject to          */
/*            evaluatn.c                                     */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Support for long long integers.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Fixed slot override default ?NONE bug.         */
/*                                                           */
/*            Instances of the form [<name>] are now         */
/*            searched for in all modules.                   */
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
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */

#include <stdlib.h>

#include "setup.h"

#if OBJECT_SYSTEM

#include "argacces.h"
#include "classcom.h"
#include "classfun.h"
#include "cstrnchk.h"
#if DEFRULE_CONSTRUCT
#include "drive.h"
#include "objrtmch.h"
#endif
#include "engine.h"
#include "envrnmnt.h"
#include "inscom.h"
#include "insmngr.h"
#include "memalloc.h"
#include "modulutl.h"
#include "msgcom.h"
#include "msgfun.h"
#include "prccode.h"
#include "prntutil.h"
#include "router.h"
#include "utility.h"

#include "insfun.h"

/* =========================================
   *****************************************
                   CONSTANTS
   =========================================
   ***************************************** */
#define BIG_PRIME    11329

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static Instance               *FindImportedInstance(Environment *,Defmodule *,Defmodule *,Instance *);

#if DEFRULE_CONSTRUCT
   static void                    NetworkModifyForSharedSlot(Environment *,int,Defclass *,SlotDescriptor *);
#endif

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************/
/* IncrementInstanceCallback: Increments the       */
/*   number of references to a specified instance. */
/***************************************************/
void IncrementInstanceCallback(
  Environment *theEnv,
  Instance *theInstance)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
   if (theInstance == NULL) return;

   theInstance->busy++;
  }

/***************************************************/
/* DecrementInstanceCallback: Decrements the       */
/*   number of references to a specified instance. */
/***************************************************/
void DecrementInstanceCallback(
  Environment *theEnv,
  Instance *theInstance)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
   if (theInstance == NULL) return;

   theInstance->busy--;
  }

/***************************************************
  NAME         : RetainInstance
  DESCRIPTION  : Increments instance busy count -
                   prevents it from being deleted
  INPUTS       : The address of the instance
  RETURNS      : Nothing useful
  SIDE EFFECTS : Count set
  NOTES        : None
 ***************************************************/
void RetainInstance(
  Instance *theInstance)
  {
   if (theInstance == NULL) return;
   
   theInstance->busy++;
  }

/***************************************************
  NAME         : ReleaseInstance
  DESCRIPTION  : Decrements instance busy count -
                   might allow it to be deleted
  INPUTS       : The address of the instance
  RETURNS      : Nothing useful
  SIDE EFFECTS : Count set
  NOTES        : None
 ***************************************************/
void ReleaseInstance(
  Instance *theInstance)
  {
   if (theInstance == NULL) return;

   theInstance->busy--;
  }

/***************************************************
  NAME         : InitializeInstanceTable
  DESCRIPTION  : Initializes instance hash table
                  to all NULL addresses
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Hash table initialized
  NOTES        : None
 ***************************************************/
void InitializeInstanceTable(
  Environment *theEnv)
  {
   int i;

   InstanceData(theEnv)->InstanceTable = (Instance **)
                    gm2(theEnv,sizeof(Instance *) * INSTANCE_TABLE_HASH_SIZE);
   for (i = 0 ; i < INSTANCE_TABLE_HASH_SIZE ; i++)
     InstanceData(theEnv)->InstanceTable[i] = NULL;
  }

/*******************************************************
  NAME         : CleanupInstances
  DESCRIPTION  : Iterates through instance garbage
                   list looking for nodes that
                   have become unused - and purges
                   them
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Non-busy instance garbage nodes deleted
  NOTES        : None
 *******************************************************/
void CleanupInstances(
  Environment *theEnv,
  void *context)
  {
   IGARBAGE *gprv,*gtmp,*dump;

   if (InstanceData(theEnv)->MaintainGarbageInstances)
     return;
   gprv = NULL;
   gtmp = InstanceData(theEnv)->InstanceGarbageList;
   while (gtmp != NULL)
     {
#if DEFRULE_CONSTRUCT
      if ((gtmp->ins->busy == 0)
          && (gtmp->ins->patternHeader.busyCount == 0))
#else
      if (gtmp->ins->busy == 0)
#endif
        {
         ReleaseLexeme(theEnv,gtmp->ins->name);
         rtn_struct(theEnv,instance,gtmp->ins);
         if (gprv == NULL)
           InstanceData(theEnv)->InstanceGarbageList = gtmp->nxt;
         else
           gprv->nxt = gtmp->nxt;
         dump = gtmp;
         gtmp = gtmp->nxt;
         rtn_struct(theEnv,igarbage,dump);
        }
      else
        {
         gprv = gtmp;
         gtmp = gtmp->nxt;
        }
     }
  }

/*******************************************************
  NAME         : HashInstance
  DESCRIPTION  : Generates a hash index for a given
                 instance name
  INPUTS       : The address of the instance name SYMBOL_HN
  RETURNS      : The hash index value
  SIDE EFFECTS : None
  NOTES        : Counts on the fact that the symbol
                 has already been hashed into the
                 symbol table - uses that hash value
                 multiplied by a prime for a new hash
 *******************************************************/
unsigned HashInstance(
  CLIPSLexeme *cname)
  {
   unsigned long tally;

   tally = ((unsigned long) cname->bucket) * BIG_PRIME;
   return (unsigned) (tally % INSTANCE_TABLE_HASH_SIZE);
  }

/***************************************************
  NAME         : DestroyAllInstances
  DESCRIPTION  : Deallocates all instances,
                  reinitializes hash table and
                  resets class instance pointers
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : All instances deallocated
  NOTES        : None
 ***************************************************/
void DestroyAllInstances(
  Environment *theEnv,
  void *context)
  {
   Instance *iptr;
   bool svmaintain;

   SaveCurrentModule(theEnv);
   svmaintain = InstanceData(theEnv)->MaintainGarbageInstances;
   InstanceData(theEnv)->MaintainGarbageInstances = true;
   iptr = InstanceData(theEnv)->InstanceList;
   while (iptr != NULL)
     {
      SetCurrentModule(theEnv,iptr->cls->header.whichModule->theModule);
      DirectMessage(theEnv,MessageHandlerData(theEnv)->DELETE_SYMBOL,iptr,NULL,NULL);
      iptr = iptr->nxtList;
      while ((iptr != NULL) ? iptr->garbage : false)
        iptr = iptr->nxtList;
     }
   InstanceData(theEnv)->MaintainGarbageInstances = svmaintain;
   RestoreCurrentModule(theEnv);
  }

/******************************************************
  NAME         : RemoveInstanceData
  DESCRIPTION  : Deallocates all the data objects
                 in instance slots and then dealloactes
                 the slots themeselves
  INPUTS       : The instance
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance slots removed
  NOTES        : An instance made with CopyInstanceData
                 will have shared values removed
                 in all cases because they are not
                 "real" instances.
                 Instance class busy count decremented
 ******************************************************/
void RemoveInstanceData(
  Environment *theEnv,
  Instance *ins)
  {
   long i;
   InstanceSlot *sp;

   DecrementDefclassBusyCount(theEnv,ins->cls);
   for (i = 0 ; i < ins->cls->instanceSlotCount ; i++)
     {
      sp = ins->slotAddresses[i];
      if ((sp == &sp->desc->sharedValue) ?
          (--sp->desc->sharedCount == 0) : true)
        {
         if (sp->desc->multiple)
           {
            ReleaseMultifield(theEnv,sp->multifieldValue);
            AddToMultifieldList(theEnv,sp->multifieldValue);
           }
         else
           AtomDeinstall(theEnv,sp->type,sp->value);
         sp->value = NULL;
        }
     }
   if (ins->cls->instanceSlotCount != 0)
     {
      rm(theEnv,ins->slotAddresses,
         (ins->cls->instanceSlotCount * sizeof(InstanceSlot *)));
      if (ins->cls->localInstanceSlotCount != 0)
        rm(theEnv,ins->slots,
           (ins->cls->localInstanceSlotCount * sizeof(InstanceSlot)));
     }
   ins->slots = NULL;
   ins->slotAddresses = NULL;
  }

/***************************************************************************
  NAME         : FindInstanceBySymbol
  DESCRIPTION  : Looks up a specified instance in the instance hash table
  INPUTS       : The symbol for the name of the instance
  RETURNS      : The address of the found instance, NULL otherwise
  SIDE EFFECTS : None
  NOTES        : An instance is searched for by name first in the
                 current module - then in imported modules according
                 to the order given in the current module's definition.
                 Instances of the form [<name>] are now searched for in
                 all modules.
 ***************************************************************************/
Instance *FindInstanceBySymbol(
  Environment *theEnv,
  CLIPSLexeme *moduleAndInstanceName)
  {
   unsigned modulePosition;
   bool searchImports;
   CLIPSLexeme *moduleName, *instanceName;
   Defmodule *currentModule,*theModule;

   currentModule = GetCurrentModule(theEnv);

   /* =======================================
      Instance names of the form [<name>] are
      searched for only in the current module
      ======================================= */
   modulePosition = FindModuleSeparator(moduleAndInstanceName->contents);
   if (modulePosition == 0)
     {
      Instance *ins;
      if (moduleAndInstanceName->header.type == SYMBOL_TYPE)
        { moduleAndInstanceName = CreateInstanceName(theEnv,moduleAndInstanceName->contents); }

      ins = InstanceData(theEnv)->InstanceTable[HashInstance(moduleAndInstanceName)];
      while (ins != NULL)
        {
         if (ins->name == moduleAndInstanceName)
           { return ins; }
         ins = ins->nxtHash;
        }
      return NULL;
     }

   /* =========================================
      Instance names of the form [::<name>] are
      searched for in the current module and
      imported modules in the definition order
      ========================================= */
   else if (modulePosition == 1)
     {
      theModule = currentModule;
      instanceName = ExtractConstructName(theEnv,modulePosition,moduleAndInstanceName->contents,INSTANCE_NAME_TYPE);
      searchImports = true;
     }

   /* =============================================
      Instance names of the form [<module>::<name>]
      are searched for in the specified module
      ============================================= */
   else
     {
      moduleName = ExtractModuleName(theEnv,modulePosition,moduleAndInstanceName->contents);
      theModule = FindDefmodule(theEnv,moduleName->contents);
      instanceName = ExtractConstructName(theEnv,modulePosition,moduleAndInstanceName->contents,INSTANCE_NAME_TYPE);
      if (theModule == NULL)
        return NULL;
      searchImports = false;
     }
   return(FindInstanceInModule(theEnv,instanceName,theModule,currentModule,searchImports));
  }

/***************************************************
  NAME         : FindInstanceInModule
  DESCRIPTION  : Finds an instance of the given name
                 in the given module in scope of
                 the given current module
                 (will also search imported modules
                  if specified)
  INPUTS       : 1) The instance name (no module)
                 2) The module to search
                 3) The currently active module
                 4) A flag indicating whether
                    to search imported modules of
                    given module as well
  RETURNS      : The instance (NULL if none found)
  SIDE EFFECTS : None
  NOTES        : The class no longer needs to be in
                 scope of the current module if the
                 instance's module name has been specified.
 ***************************************************/
Instance *FindInstanceInModule(
  Environment *theEnv,
  CLIPSLexeme *instanceName,
  Defmodule *theModule,
  Defmodule *currentModule,
  bool searchImports)
  {
   Instance *startInstance,*ins;

   /* ===============================
      Find the first instance of the
      correct name in the hash chain
      =============================== */
   startInstance = InstanceData(theEnv)->InstanceTable[HashInstance(instanceName)];
   while (startInstance != NULL)
     {
      if (startInstance->name == instanceName)
        break;
      startInstance = startInstance->nxtHash;
     }

   if (startInstance == NULL)
     return NULL;

   /* ===========================================
      Look for the instance in the specified
      module - if the class of the found instance
      is in scope of the current module, we have
      found the instance
      =========================================== */
   for (ins = startInstance ;
        (ins != NULL) ? (ins->name == startInstance->name) : false ;
        ins = ins->nxtHash)
     //if ((ins->cls->header.whichModule->theModule == theModule) &&
     //     DefclassInScope(theEnv,ins->cls,currentModule))
     if (ins->cls->header.whichModule->theModule == theModule)
       return(ins);

   /* ================================
      For ::<name> formats, we need to
      search imported modules too
      ================================ */
   if (searchImports == false)
     return NULL;
   MarkModulesAsUnvisited(theEnv);
   return(FindImportedInstance(theEnv,theModule,currentModule,startInstance));
  }

/********************************************************************
  NAME         : FindInstanceSlot
  DESCRIPTION  : Finds an instance slot by name
  INPUTS       : 1) The address of the instance
                 2) The symbolic name of the slot
  RETURNS      : The address of the slot, NULL if not found
  SIDE EFFECTS : None
  NOTES        : None
 ********************************************************************/
InstanceSlot *FindInstanceSlot(
  Environment *theEnv,
  Instance *ins,
  CLIPSLexeme *sname)
  {
   int i;

   i = FindInstanceTemplateSlot(theEnv,ins->cls,sname);
   return (i != -1) ? ins->slotAddresses[i] : NULL;
  }

/********************************************************************
  NAME         : FindInstanceTemplateSlot
  DESCRIPTION  : Performs a search on an class's instance
                   template slot array to find a slot by name
  INPUTS       : 1) The address of the class
                 2) The symbolic name of the slot
  RETURNS      : The index of the slot, -1 if not found
  SIDE EFFECTS : None
  NOTES        : The slot's unique id is used as index into
                 the slot map array.
 ********************************************************************/
int FindInstanceTemplateSlot(
  Environment *theEnv,
  Defclass *cls,
  CLIPSLexeme *sname)
  {
   unsigned short sid;

   sid = FindSlotNameID(theEnv,sname);
   if (sid == SLOT_NAME_NOT_FOUND)
     return -1;
     
   if (sid > cls->maxSlotNameID)
     return -1;
     
   return((int) cls->slotNameMap[sid] - 1);
  }

/*******************************************************
  NAME         : PutSlotValue
  DESCRIPTION  : Evaluates new slot-expression and
                   stores it as a multifield
                   variable for the slot.
  INPUTS       : 1) The address of the instance
                    (NULL if no trace-messages desired)
                 2) The address of the slot
                 3) The address of the value
                 4) UDFValue pointer to store the
                    set value
                 5) The command doing the put-
  RETURNS      : False on errors, or true
  SIDE EFFECTS : Old value deleted and new one allocated
                 Old value symbols deinstalled
                 New value symbols installed
  NOTES        : None
 *******************************************************/
PutSlotError PutSlotValue(
  Environment *theEnv,
  Instance *ins,
  InstanceSlot *sp,
  UDFValue *val,
  UDFValue *setVal,
  const char *theCommand)
  {
   PutSlotError rv;
   if ((rv = ValidSlotValue(theEnv,val,sp->desc,ins,theCommand)) != PSE_NO_ERROR)
     {
      setVal->value = FalseSymbol(theEnv);
      return rv;
     }
   return DirectPutSlotValue(theEnv,ins,sp,val,setVal);
  }

/*******************************************************
  NAME         : DirectPutSlotValue
  DESCRIPTION  : Evaluates new slot-expression and
                   stores it as a multifield
                   variable for the slot.
  INPUTS       : 1) The address of the instance
                    (NULL if no trace-messages desired)
                 2) The address of the slot
                 3) The address of the value
                 4) UDFValue pointer to store the
                    set value
  RETURNS      : False on errors, or true
  SIDE EFFECTS : Old value deleted and new one allocated
                 Old value symbols deinstalled
                 New value symbols installed
  NOTES        : None
 *******************************************************/
PutSlotError DirectPutSlotValue(
  Environment *theEnv,
  Instance *ins,
  InstanceSlot *sp,
  UDFValue *val,
  UDFValue *setVal)
  {
   size_t i,j; /* 6.04 Bug Fix */
#if DEFRULE_CONSTRUCT
   int sharedTraversalID;
   InstanceSlot *bsp,**spaddr;
#endif
   UDFValue tmpVal;

   setVal->value = FalseSymbol(theEnv);
   if (val == NULL)
     {
      SystemError(theEnv,"INSFUN",1);
      ExitRouter(theEnv,EXIT_FAILURE);
     }
   else if (val->value == ProceduralPrimitiveData(theEnv)->NoParamValue)
     {
      if (sp->desc->dynamicDefault)
        {
         val = &tmpVal;
         if (!EvaluateAndStoreInDataObject(theEnv,sp->desc->multiple,
                                           (Expression *) sp->desc->defaultValue,val,true))
           return PSE_EVALUATION_ERROR;
        }
      else if (sp->desc->defaultValue != NULL)
        { val = (UDFValue *) sp->desc->defaultValue; }
      else
        {
         PrintErrorID(theEnv,"INSMNGR",14,false);
         WriteString(theEnv,STDERR,"Override required for slot '");
         WriteString(theEnv,STDERR,sp->desc->slotName->name->contents);
         WriteString(theEnv,STDERR,"' in instance [");
         WriteString(theEnv,STDERR,ins->name->contents);
         WriteString(theEnv,STDERR,"].\n");
         SetEvaluationError(theEnv,true);
         return PSE_EVALUATION_ERROR;
        }
     }
#if DEFRULE_CONSTRUCT
   if (EngineData(theEnv)->JoinOperationInProgress && sp->desc->reactive &&
       (ins->cls->reactive || sp->desc->shared))
     {
      PrintErrorID(theEnv,"INSFUN",5,false);
      WriteString(theEnv,STDERR,"Cannot modify reactive instance slots while ");
      WriteString(theEnv,STDERR,"pattern-matching is in process.\n");
      SetEvaluationError(theEnv,true);
      return PSE_RULE_NETWORK_ERROR;
     }

   /* =============================================
      If we are about to change a slot of an object
      which is a basis for a firing rule, we need
      to make sure that slot is copied first
      ============================================= */
   if (ins->basisSlots != NULL)
     {
      spaddr = &ins->slotAddresses[ins->cls->slotNameMap[sp->desc->slotName->id] - 1];
      bsp = ins->basisSlots + (spaddr - ins->slotAddresses);
      if (bsp->value == NULL)
        {
         bsp->type = sp->type;
         bsp->value = sp->value;
         if (sp->desc->multiple)
           RetainMultifield(theEnv,bsp->multifieldValue);
         else
           AtomInstall(theEnv,bsp->type,bsp->value);
        }
     }

#endif
   if (sp->desc->multiple == 0)
     {
      AtomDeinstall(theEnv,sp->type,sp->value);

      /* ======================================
         Assumed that multfield already checked
         to be of cardinality 1
         ====================================== */
      if (val->header->type == MULTIFIELD_TYPE)
        {
         sp->type = val->multifieldValue->contents[val->begin].header->type;
         sp->value = val->multifieldValue->contents[val->begin].value;
        }
      else
        {
         sp->type = val->header->type;
         sp->value = val->value;
        }
      AtomInstall(theEnv,sp->type,sp->value);
      setVal->value = sp->value;
     }
   else
     {
      ReleaseMultifield(theEnv,sp->multifieldValue);
      AddToMultifieldList(theEnv,sp->multifieldValue);
      sp->type = MULTIFIELD_TYPE;
      if (val->header->type == MULTIFIELD_TYPE)
        {
         sp->value = CreateUnmanagedMultifield(theEnv,(unsigned long) val->range);
         for (i = 0 , j = val->begin ; i < val->range ; i++ , j++)
           {
            sp->multifieldValue->contents[i].value = val->multifieldValue->contents[j].value;
           }
        }
      else
        {
         sp->multifieldValue = CreateUnmanagedMultifield(theEnv,1L);
         sp->multifieldValue->contents[0].value = val->value;
        }
      RetainMultifield(theEnv,sp->multifieldValue);
      setVal->value = sp->value;
      setVal->begin = 0;
      setVal->range = sp->multifieldValue->length;
     }
   /* ==================================================
      6.05 Bug fix - any slot set directly or indirectly
      by a slot override or other side-effect during an
      instance initialization should not have its
      default value set
      ================================================== */

   sp->override = ins->initializeInProgress;

#if DEBUGGING_FUNCTIONS
   if (ins->cls->traceSlots &&
       (! ConstructData(theEnv)->ClearReadyInProgress) &&
       (! ConstructData(theEnv)->ClearInProgress))
     {
      if (sp->desc->shared)
        WriteString(theEnv,STDOUT,"::= shared slot ");
      else
        WriteString(theEnv,STDOUT,"::= local slot ");
      WriteString(theEnv,STDOUT,sp->desc->slotName->name->contents);
      WriteString(theEnv,STDOUT," in instance ");
      WriteString(theEnv,STDOUT,ins->name->contents);
      WriteString(theEnv,STDOUT," <- ");
      if (sp->type != MULTIFIELD_TYPE)
        PrintAtom(theEnv,STDOUT,sp->type,sp->value);
      else
        PrintMultifieldDriver(theEnv,STDOUT,sp->multifieldValue,0,
                              sp->multifieldValue->length,true);
      WriteString(theEnv,STDOUT,"\n");
     }
#endif
   InstanceData(theEnv)->ChangesToInstances = true;

#if DEFRULE_CONSTRUCT
   if (ins->cls->reactive && sp->desc->reactive)
     {
      /* ============================================
         If we have changed a shared slot, we need to
         perform a Rete update for every instance
         which contains this slot
         ============================================ */
      if (sp->desc->shared)
        {
         sharedTraversalID = GetTraversalID(theEnv);
         if (sharedTraversalID != -1)
           {
            NetworkModifyForSharedSlot(theEnv,sharedTraversalID,sp->desc->cls,sp->desc);
            ReleaseTraversalID(theEnv);
           }
         else
           {
            PrintErrorID(theEnv,"INSFUN",6,false);
            WriteString(theEnv,STDERR,"Unable to pattern-match on shared slot '");
            WriteString(theEnv,STDERR,sp->desc->slotName->name->contents);
            WriteString(theEnv,STDERR,"' in class '");
            WriteString(theEnv,STDERR,DefclassName(sp->desc->cls));
            WriteString(theEnv,STDERR,"'.\n");
            return PSE_RULE_NETWORK_ERROR;
           }
        }
      else
        ObjectNetworkAction(theEnv,OBJECT_MODIFY,ins,(int) sp->desc->slotName->id);
     }
#endif

   return PSE_NO_ERROR;
  }

/*******************************************************************
  NAME         : ValidSlotValue
  DESCRIPTION  : Determines if a value is appropriate
                   for a slot-value
  INPUTS       : 1) The value buffer
                 2) Slot descriptor
                 3) Instance for which slot is being checked
                    (can be NULL)
                 4) Buffer holding printout of the offending command
                    (if NULL assumes message-handler is executing
                     and calls PrintHandler for CurrentCore instead)
  RETURNS      : True if value is OK, false otherwise
  SIDE EFFECTS : Sets EvaluationError if slot is not OK
  NOTES        : Examines all fields of a multi-field
 *******************************************************************/
PutSlotError ValidSlotValue(
  Environment *theEnv,
  UDFValue *val,
  SlotDescriptor *sd,
  Instance *ins,
  const char *theCommand)
  {
   ConstraintViolationType violationCode;

   /* ===================================
      Special NoParamValue means to reset
      slot to default value
      =================================== */
   if (val->value == ProceduralPrimitiveData(theEnv)->NoParamValue)
     return PSE_NO_ERROR;
   if ((sd->multiple == 0) && (val->header->type == MULTIFIELD_TYPE) &&
                              (val->range != 1))
     {
      PrintErrorID(theEnv,"INSFUN",7,false);
      WriteString(theEnv,STDERR,"The value ");
      WriteUDFValue(theEnv,STDERR,val);
      WriteString(theEnv,STDERR," is illegal for single-field ");
      PrintSlot(theEnv,STDERR,sd,ins,theCommand);
      WriteString(theEnv,STDERR,".\n");
      SetEvaluationError(theEnv,true);
      return PSE_CARDINALITY_ERROR;
     }
   if (val->header->type == VOID_TYPE)
     {
      PrintErrorID(theEnv,"INSFUN",8,false);
      WriteString(theEnv,STDERR,"Void function illegal value for ");
      PrintSlot(theEnv,STDERR,sd,ins,theCommand);
      WriteString(theEnv,STDERR,".\n");
      SetEvaluationError(theEnv,true);
      return PSE_CARDINALITY_ERROR;
     }
   if (GetDynamicConstraintChecking(theEnv))
     {
      violationCode = ConstraintCheckDataObject(theEnv,val,sd->constraint);
      if (violationCode != NO_VIOLATION)
        {
         PrintErrorID(theEnv,"CSTRNCHK",1,false);
         WriteString(theEnv,STDERR,"The value ");
         if ((val->header->type == MULTIFIELD_TYPE) && (sd->multiple == 0))
           PrintAtom(theEnv,STDERR,val->multifieldValue->contents[val->begin].header->type,
                                   val->multifieldValue->contents[val->begin].value);
         else
           WriteUDFValue(theEnv,STDERR,val);
         WriteString(theEnv,STDERR," for ");
         PrintSlot(theEnv,STDERR,sd,ins,theCommand);
         ConstraintViolationErrorMessage(theEnv,NULL,NULL,0,0,NULL,0,
                                         violationCode,sd->constraint,false);
         SetEvaluationError(theEnv,true);

         switch(violationCode)
           {
            case NO_VIOLATION:
              SystemError(theEnv,"FACTMNGR",2);
              ExitRouter(theEnv,EXIT_FAILURE);
              break;
        
            case FUNCTION_RETURN_TYPE_VIOLATION:
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
   return PSE_NO_ERROR;
  }

/********************************************************
  NAME         : CheckInstance
  DESCRIPTION  : Checks to see if the first argument to
                 a function is a valid instance
  INPUTS       : Name of the calling function
  RETURNS      : The address of the instance
  SIDE EFFECTS : EvaluationError set and messages printed
                 on errors
  NOTES        : Used by Initialize and ModifyInstance
 ********************************************************/
Instance *CheckInstance(
  UDFContext *context)
  {
   Instance *ins;
   UDFValue temp;
   Environment *theEnv = context->environment;

   UDFFirstArgument(context,ANY_TYPE_BITS,&temp);
   if (temp.header->type == INSTANCE_ADDRESS_TYPE)
     {
      ins = temp.instanceValue;
      if (ins->garbage == 1)
        {
         StaleInstanceAddress(theEnv,UDFContextFunctionName(context),0);
         SetEvaluationError(theEnv,true);
         return NULL;
        }
     }
   else if (temp.header->type == INSTANCE_NAME_TYPE)
     {
      ins = FindInstanceBySymbol(theEnv,temp.lexemeValue);
      if (ins == NULL)
        {
         NoInstanceError(theEnv,temp.lexemeValue->contents,UDFContextFunctionName(context));
         return NULL;
        }
     }
   else if (temp.header->type == SYMBOL_TYPE)
     {
      temp.value = CreateInstanceName(theEnv,temp.lexemeValue->contents);
      ins = FindInstanceBySymbol(theEnv,temp.lexemeValue);
      if (ins == NULL)
        {
         NoInstanceError(theEnv,temp.lexemeValue->contents,UDFContextFunctionName(context));
         return NULL;
        }
     }
   else
     {
      PrintErrorID(theEnv,"INSFUN",1,false);
      WriteString(theEnv,STDERR,"Expected a valid instance in function '");
      WriteString(theEnv,STDERR,UDFContextFunctionName(context));
      WriteString(theEnv,STDERR,"'.\n");
      SetEvaluationError(theEnv,true);
      return NULL;
     }
   return(ins);
  }

/***************************************************
  NAME         : NoInstanceError
  DESCRIPTION  : Prints out an appropriate error
                  message when an instance cannot be
                  found for a function
  INPUTS       : 1) The instance name
                 2) The function name
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
void NoInstanceError(
  Environment *theEnv,
  const char *iname,
  const char *func)
  {
   PrintErrorID(theEnv,"INSFUN",2,false);
   WriteString(theEnv,STDERR,"No such instance [");
   WriteString(theEnv,STDERR,iname);
   WriteString(theEnv,STDERR,"] in function '");
   WriteString(theEnv,STDERR,func);
   WriteString(theEnv,STDERR,"'.\n");
   SetEvaluationError(theEnv,true);
  }

/***************************************************
  NAME         : StaleInstanceAddress
  DESCRIPTION  : Prints out an appropriate error
                  message when an instance address
                  is no longer valid
  INPUTS       : The function name
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
void StaleInstanceAddress(
  Environment *theEnv,
  const char *func,
  int whichArg)
  {
   PrintErrorID(theEnv,"INSFUN",4,false);
   WriteString(theEnv,STDERR,"Invalid instance-address in function '");
   WriteString(theEnv,STDERR,func);
   WriteString(theEnv,STDERR,"'");
   if (whichArg > 0)
     {
      WriteString(theEnv,STDERR,", argument #");
      WriteInteger(theEnv,STDERR,whichArg);
     }
   WriteString(theEnv,STDERR,".\n");
  }

/**********************************************************************
  NAME         : GetInstancesChanged
  DESCRIPTION  : Returns whether instances have changed
                   (any were added/deleted or slot values were changed)
                   since last time flag was set to false
  INPUTS       : None
  RETURNS      : The instances-changed flag
  SIDE EFFECTS : None
  NOTES        : Used by interfaces to update instance windows
 **********************************************************************/
bool GetInstancesChanged(
  Environment *theEnv)
  {
   return InstanceData(theEnv)->ChangesToInstances;
  }

/*******************************************************
  NAME         : SetInstancesChanged
  DESCRIPTION  : Sets instances-changed flag (see above)
  INPUTS       : The value (true or false)
  RETURNS      : Nothing useful
  SIDE EFFECTS : The flag is set
  NOTES        : None
 *******************************************************/
void SetInstancesChanged(
  Environment *theEnv,
  bool changed)
  {
   InstanceData(theEnv)->ChangesToInstances = changed;
  }

/*******************************************************************
  NAME         : PrintSlot
  DESCRIPTION  : Displays the name and origin of a slot
  INPUTS       : 1) The logical output name
                 2) The slot descriptor
                 3) The instance source (can be NULL)
                 4) Buffer holding printout of the offending command
                    (if NULL assumes message-handler is executing
                     and calls PrintHandler for CurrentCore instead)
  RETURNS      : Nothing useful
  SIDE EFFECTS : Message printed
  NOTES        : None
 *******************************************************************/
void PrintSlot(
  Environment *theEnv,
  const char *logName,
  SlotDescriptor *sd,
  Instance *ins,
  const char *theCommand)
  {
   WriteString(theEnv,logName,"slot '");
   WriteString(theEnv,logName,sd->slotName->name->contents);
   WriteString(theEnv,logName,"'");
   if (ins != NULL)
     {
      WriteString(theEnv,logName," of instance [");
      WriteString(theEnv,logName,ins->name->contents);
      WriteString(theEnv,logName,"]");
     }
   else if (sd->cls != NULL)
     {
      WriteString(theEnv,logName," of class '");
      WriteString(theEnv,logName,DefclassName(sd->cls));
      WriteString(theEnv,logName," of class '");
     }
   WriteString(theEnv,logName," found in ");
   if (theCommand != NULL)
     WriteString(theEnv,logName,theCommand);
   else
     PrintHandler(theEnv,logName,MessageHandlerData(theEnv)->CurrentCore->hnd,true,false);
  }

/*****************************************************
  NAME         : PrintInstanceNameAndClass
  DESCRIPTION  : Displays an instance's name and class
  INPUTS       : 1) Logical name of output
                 2) The instance
                 3) Flag indicating whether to
                    print carriage-return at end
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instnace name and class printed
  NOTES        : None
 *****************************************************/
void PrintInstanceNameAndClass(
  Environment *theEnv,
  const char *logicalName,
  Instance *theInstance,
  bool linefeedFlag)
  {
   WriteString(theEnv,logicalName,"[");
   WriteString(theEnv,logicalName,InstanceName(theInstance));
   WriteString(theEnv,logicalName,"] of ");
   PrintClassName(theEnv,logicalName,theInstance->cls,false,linefeedFlag);
  }

/***************************************************
  NAME         : PrintInstanceName
  DESCRIPTION  : Used by the rule system commands
                 such as (matches) and (agenda)
                 to print out the name of an instance
  INPUTS       : 1) The logical output name
                 2) A pointer to the instance
  RETURNS      : Nothing useful
  SIDE EFFECTS : Name of instance printed
  NOTES        : None
 ***************************************************/
void PrintInstanceName(
  Environment *theEnv,
  const char *logName,
  Instance *theInstance)
  {
   if (theInstance->garbage)
     {
      WriteString(theEnv,logName,"<stale instance [");
      WriteString(theEnv,logName,theInstance->name->contents);
      WriteString(theEnv,logName,"]>");
     }
   else
     {
      WriteString(theEnv,logName,"[");
      WriteString(theEnv,logName,GetFullInstanceName(theEnv,theInstance)->contents);
      WriteString(theEnv,logName,"]");
     }
  }

/***************************************************
  NAME         : PrintInstanceLongForm
  DESCRIPTION  : Used by kernel to print
                 instance addresses
  INPUTS       : 1) The logical output name
                 2) A pointer to the instance
  RETURNS      : Nothing useful
  SIDE EFFECTS : Address of instance printed
  NOTES        : None
 ***************************************************/
void PrintInstanceLongForm(
  Environment *theEnv,
  const char *logName,
  Instance *theInstance)
  {
   if (PrintUtilityData(theEnv)->InstanceAddressesToNames)
     {
      if (theInstance == &InstanceData(theEnv)->DummyInstance)
        WriteString(theEnv,logName,"\"<Dummy Instance>\"");
      else
        {
         WriteString(theEnv,logName,"[");
         WriteString(theEnv,logName,GetFullInstanceName(theEnv,theInstance)->contents);
         WriteString(theEnv,logName,"]");
        }
     }
   else
     {
      if (PrintUtilityData(theEnv)->AddressesToStrings)
        WriteString(theEnv,logName,"\"");
      if (theInstance == &InstanceData(theEnv)->DummyInstance)
        WriteString(theEnv,logName,"<Dummy Instance>");
      else if (theInstance->garbage)
        {
         WriteString(theEnv,logName,"<Stale Instance-");
         WriteString(theEnv,logName,theInstance->name->contents);
         WriteString(theEnv,logName,">");
        }
      else
        {
         WriteString(theEnv,logName,"<Instance-");
         WriteString(theEnv,logName,GetFullInstanceName(theEnv,theInstance)->contents);
         WriteString(theEnv,logName,">");
        }
      if (PrintUtilityData(theEnv)->AddressesToStrings)
        WriteString(theEnv,logName,"\"");
     }
  }

#if DEFRULE_CONSTRUCT

/***************************************************
  NAME         : DecrementObjectBasisCount
  DESCRIPTION  : Decrements the basis count of an
                 object indicating that it is in
                 use by the partial match of the
                 currently executing rule
  INPUTS       : The instance address
  RETURNS      : Nothing useful
  SIDE EFFECTS : Basis count decremented and
                 basis copy (possibly) deleted
  NOTES        : When the count goes to zero, the
                 basis copy of the object (if any)
                 is deleted.
 ***************************************************/
void DecrementObjectBasisCount(
  Environment *theEnv,
  Instance *theInstance)
  {
   long i;

   theInstance->patternHeader.busyCount--;
   if (theInstance->patternHeader.busyCount == 0)
     {
      if (theInstance->garbage)
        RemoveInstanceData(theEnv,theInstance);
      if (theInstance->cls->instanceSlotCount != 0)
        {
         for (i = 0 ; i < theInstance->cls->instanceSlotCount ; i++)
           if (theInstance->basisSlots[i].value != NULL)
             {
              if (theInstance->basisSlots[i].desc->multiple)
                ReleaseMultifield(theEnv,theInstance->basisSlots[i].multifieldValue);
              else
                AtomDeinstall(theEnv,theInstance->basisSlots[i].type,
                              theInstance->basisSlots[i].value);
             }
         rm(theEnv,theInstance->basisSlots,
            (theInstance->cls->instanceSlotCount * sizeof(InstanceSlot)));
         theInstance->basisSlots = NULL;
        }
     }
  }

/***************************************************
  NAME         : IncrementObjectBasisCount
  DESCRIPTION  : Increments the basis count of an
                 object indicating that it is in
                 use by the partial match of the
                 currently executing rule

                 If this the count was zero,
                 allocate an array of extra
                 instance slots for use by
                 slot variables
  INPUTS       : The instance address
  RETURNS      : Nothing useful
  SIDE EFFECTS : Basis count incremented
  NOTES        : None
 ***************************************************/
void IncrementObjectBasisCount(
  Environment *theEnv,
  Instance *theInstance)
  {
   long i;

   if (theInstance->patternHeader.busyCount == 0)
     {
      if (theInstance->cls->instanceSlotCount != 0)
        {
         theInstance->basisSlots = (InstanceSlot *)
                            gm2(theEnv,(sizeof(InstanceSlot) * theInstance->cls->instanceSlotCount));
         for (i = 0 ; i < theInstance->cls->instanceSlotCount ; i++)
           {
            theInstance->basisSlots[i].desc = theInstance->slotAddresses[i]->desc;
            theInstance->basisSlots[i].value = NULL;
           }
        }
     }
   theInstance->patternHeader.busyCount++;
  }

/***************************************************
  NAME         : MatchObjectFunction
  DESCRIPTION  : Filters an instance through the
                 object pattern network
                 Used for incremental resets in
                 binary loads and run-time modules
  INPUTS       : The instance
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instance pattern-matched
  NOTES        : None
 ***************************************************/
void MatchObjectFunction(
  Environment *theEnv,
  Instance *theInstance)
  {
   ObjectNetworkAction(theEnv,OBJECT_ASSERT,theInstance,-1);
  }

/***************************************************
  NAME         : NetworkSynchronized
  DESCRIPTION  : Determines if state of instance is
                 consistent with last push through
                 pattern-matching network
  INPUTS       : The instance
  RETURNS      : True if instance has not
                 changed since last push through the
                 Rete network, false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
bool NetworkSynchronized(
  Environment *theEnv,
  Instance *theInstance)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   return theInstance->reteSynchronized;
  }

/***************************************************
  NAME         : InstanceIsDeleted
  DESCRIPTION  : Determines if an instance has been
                 deleted
  INPUTS       : The instance
  RETURNS      : True if instance has been deleted,
                 false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
bool InstanceIsDeleted(
  Environment *theEnv,
  Instance *theInstance)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   return theInstance->garbage;
  }
#endif

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/*****************************************************
  NAME         : FindImportedInstance
  DESCRIPTION  : Searches imported modules for an
                 instance of the correct name
                 The imports are searched recursively
                 in the order of the module definition
  INPUTS       : 1) The module for which to
                    search imported modules
                 2) The currently active module
                 3) The first instance of the
                    correct name (cannot be NULL)
  RETURNS      : An instance of the correct name
                 imported from another module which
                 is in scope of the current module
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************/
static Instance *FindImportedInstance(
  Environment *theEnv,
  Defmodule *theModule,
  Defmodule *currentModule,
  Instance *startInstance)
  {
   struct portItem *importList;
   Instance *ins;

   if (theModule->visitedFlag)
     return NULL;
   theModule->visitedFlag = true;
   importList = theModule->importList;
   while (importList != NULL)
     {
      theModule = FindDefmodule(theEnv,importList->moduleName->contents);
      for (ins = startInstance ;
           (ins != NULL) ? (ins->name == startInstance->name) : false ;
           ins = ins->nxtHash)
        if ((ins->cls->header.whichModule->theModule == theModule) &&
             DefclassInScope(theEnv,ins->cls,currentModule))
          return(ins);
      ins = FindImportedInstance(theEnv,theModule,currentModule,startInstance);
      if (ins != NULL)
        return(ins);
      importList = importList->next;
     }

   /* ========================================================
      Make sure instances of system classes are always visible
      ======================================================== */
   for (ins = startInstance ;
        (ins != NULL) ? (ins->name == startInstance->name) : false ;
        ins = ins->nxtHash)
     if (ins->cls->system)
       return(ins);

   return NULL;
  }

#if DEFRULE_CONSTRUCT

/*****************************************************
  NAME         : NetworkModifyForSharedSlot
  DESCRIPTION  : Performs a Rete network modify for
                 all instances which contain a
                 specific shared slot
  INPUTS       : 1) The traversal id to use when
                    recursively entering subclasses
                    to prevent duplicate examinations
                    of a class
                 2) The class
                 3) The descriptor for the shared slot
  RETURNS      : Nothing useful
  SIDE EFFECTS : Instances which contain the shared
                 slot are filtered through the
                 Rete network via a retract/assert
  NOTES        : Assumes traversal id has been
                 established
 *****************************************************/
static void NetworkModifyForSharedSlot(
  Environment *theEnv,
  int sharedTraversalID,
  Defclass *cls,
  SlotDescriptor *sd)
  {
   Instance *ins;
   unsigned long i;

   /* ================================================
      Make sure we haven't already examined this class
      ================================================ */
   if (TestTraversalID(cls->traversalRecord,sharedTraversalID))
     return;
   SetTraversalID(cls->traversalRecord,sharedTraversalID);

   /* ===========================================
      If the instances of this class contain the
      shared slot, send update events to the Rete
      network for all of its instances
      =========================================== */
   if ((sd->slotName->id > cls->maxSlotNameID) ? false :
       ((cls->slotNameMap[sd->slotName->id] == 0) ? false :
        (cls->instanceTemplate[cls->slotNameMap[sd->slotName->id] - 1] == sd)))
     {
      for (ins = cls->instanceList ; ins != NULL ; ins = ins->nxtClass)
        ObjectNetworkAction(theEnv,OBJECT_MODIFY,ins,(int) sd->slotName->id);
     }

   /* ==================================
      Check the subclasses of this class
      ================================== */
   for (i = 0 ; i < cls->directSubclasses.classCount ; i++)
     NetworkModifyForSharedSlot(theEnv,sharedTraversalID,cls->directSubclasses.classArray[i],sd);
  }

#endif /* DEFRULE_CONSTRUCT */

#endif /* OBJECT_SYSTEM */


