   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  05/16/18             */
   /*                                                     */
   /*                  CLASS PARSER MODULE                */
   /*******************************************************/

/**************************************************************/
/* Purpose: Parsing Routines for Defclass Construct           */
/*                                                            */
/* Principal Programmer(s):                                   */
/*      Brian L. Dantes                                       */
/*                                                            */
/* Contributing Programmer(s):                                */
/*                                                            */
/* Revision History:                                          */
/*                                                            */
/*      6.24: Added allowed-classes slot facet.               */
/*                                                            */
/*            Converted INSTANCE_PATTERN_MATCHING to          */
/*            DEFRULE_CONSTRUCT.                              */
/*                                                            */
/*            Renamed BOOLEAN macro type to intBool.          */
/*                                                            */
/*      6.30: Added support to allow CreateClassScopeMap to   */
/*            be used by other functions.                     */
/*                                                            */
/*            Changed integer type/precision.                 */
/*                                                            */
/*            GetConstructNameAndComment API change.          */
/*                                                            */
/*            Added const qualifiers to remove C++            */
/*            deprecation warnings.                           */
/*                                                            */
/*            Converted API macros to function calls.         */
/*                                                            */
/*            Changed find construct functionality so that    */
/*            imported modules are search when locating a     */
/*            named construct.                                */
/*                                                            */
/*      6.40: Pragma once and other inclusion changes.        */
/*                                                            */
/*            Added support for booleans with <stdbool.h>.    */
/*                                                            */
/*            Removed use of void pointers for specific       */
/*            data structures.                                */
/*                                                            */
/*            Eval support for run time and bload only.       */
/*                                                            */
/*            Removed use of single-slot in class             */
/*            definitions.                                    */
/*                                                            */
/**************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif

#include "classcom.h"
#include "classfun.h"
#include "clsltpsr.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "inherpsr.h"
#include "memalloc.h"
#include "modulpsr.h"
#include "modulutl.h"
#include "msgpsr.h"
#include "pprint.h"
#include "prntutil.h"
#include "router.h"
#include "scanner.h"

#include "classpsr.h"

/* =========================================
   *****************************************
                   CONSTANTS
   =========================================
   ***************************************** */
#define ROLE_RLN             "role"
#define ABSTRACT_RLN         "abstract"
#define CONCRETE_RLN         "concrete"

#define HANDLER_DECL         "message-handler"

#define SLOT_RLN             "slot"
#define MLT_SLOT_RLN         "multislot"

#define DIRECT               0
#define INHERIT              1

#if OBJECT_SYSTEM && (! BLOAD_ONLY) && (! RUN_TIME)

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static bool                    ValidClassName(Environment *,const char *,Defclass **);
   static bool                    ParseSimpleQualifier(Environment *,const char *,const char *,const char *,const char *,bool *,bool *);
   static bool                    ReadUntilClosingParen(Environment *,const char *,struct token *);
   static void                    AddClass(Environment *,Defclass *);
   static void                    BuildSubclassLinks(Environment *,Defclass *);
   static void                    FormInstanceTemplate(Environment *,Defclass *);
   static void                    FormSlotNameMap(Environment *,Defclass *);
   static TEMP_SLOT_LINK         *MergeSlots(Environment *,TEMP_SLOT_LINK *,Defclass *,unsigned short *,unsigned short);
   static void                    PackSlots(Environment *,Defclass *,TEMP_SLOT_LINK *);
   static void                    CreatePublicSlotMessageHandlers(Environment *,Defclass *);


/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************************************************
  NAME         : ParseDefclass
  DESCRIPTION  : (defclass ...) is a construct (as
                 opposed to a function), thus no variables
                 may be used.  This means classes may only
                 be STATICALLY defined (like rules).
  INPUTS       : The logical name of the router
                    for the parser input
  RETURNS      : False if successful parse, true otherwise
  SIDE EFFECTS : Inserts valid class definition into
                 Class Table.
  NOTES        : H/L Syntax :
                 (defclass <name> [<comment>]
                    (is-a <superclass-name>+)
                    <class-descriptor>*)

                 <class-descriptor> :== (slot <name> <slot-descriptor>*) |
                                        (role abstract|concrete) |
                                        (pattern-match reactive|non-reactive)

                                        These are for documentation only:
                                        (message-handler <name> [<type>])

                 <slot-descriptor>  :== (default <default-expression>) |
                                        (default-dynamic <default-expression>) |
                                        (storage shared|local) |
                                        (access read-only|read-write|initialize-only) |
                                        (propagation no-inherit|inherit) |
                                        (source composite|exclusive)
                                        (pattern-match reactive|non-reactive)
                                        (visibility public|private)
                                        (override-message <message-name>)
                                        (type ...) |
                                        (cardinality ...) |
                                        (allowed-symbols ...) |
                                        (allowed-strings ...) |
                                        (allowed-numbers ...) |
                                        (allowed-integers ...) |
                                        (allowed-floats ...) |
                                        (allowed-values ...) |
                                        (allowed-instance-names ...) |
                                        (allowed-classes ...) |
                                        (range ...)

               <default-expression> ::= ?NONE | ?VARIABLE | <expression>*
  ***************************************************************************************/
bool ParseDefclass(
  Environment *theEnv,
  const char *readSource)
  {
   CLIPSLexeme *cname;
   Defclass *cls;
   PACKED_CLASS_LINKS *sclasses,*preclist;
   TEMP_SLOT_LINK *slots = NULL;
   bool parseError;
   bool roleSpecified = false, abstract = false;
#if DEFRULE_CONSTRUCT
   bool patternMatchSpecified = false;
   bool reactive = true;
#endif

   SetPPBufferStatus(theEnv,true);
   FlushPPBuffer(theEnv);
   SetIndentDepth(theEnv,3);
   SavePPBuffer(theEnv,"(defclass ");

#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE
   if ((Bloaded(theEnv)) && (! ConstructData(theEnv)->CheckSyntaxMode))
     {
      CannotLoadWithBloadMessage(theEnv,"defclass");
      return true;
     }
#endif

   cname = GetConstructNameAndComment(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken,"defclass",
                                      (FindConstructFunction *) FindDefclassInModule,NULL,"#",true,
                                      true,true,false);
   if (cname == NULL)
     return true;

   if (ValidClassName(theEnv,cname->contents,&cls) == false)
     return true;

   sclasses = ParseSuperclasses(theEnv,readSource,cname);
   if (sclasses == NULL)
     return true;
   preclist = FindPrecedenceList(theEnv,cls,sclasses);
   if (preclist == NULL)
     {
      DeletePackedClassLinks(theEnv,sclasses,true);
      return true;
     }
   parseError = false;
   GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
   while (DefclassData(theEnv)->ObjectParseToken.tknType != RIGHT_PARENTHESIS_TOKEN)
     {
      if (DefclassData(theEnv)->ObjectParseToken.tknType != LEFT_PARENTHESIS_TOKEN)
        {
         SyntaxErrorMessage(theEnv,"defclass");
         parseError = true;
         break;
        }
      PPBackup(theEnv);
      PPCRAndIndent(theEnv);
      SavePPBuffer(theEnv,"(");
      GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
      if (DefclassData(theEnv)->ObjectParseToken.tknType != SYMBOL_TOKEN)
        {
         SyntaxErrorMessage(theEnv,"defclass");
         parseError = true;
         break;
        }
      if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,ROLE_RLN) == 0)
        {
         if (ParseSimpleQualifier(theEnv,readSource,ROLE_RLN,CONCRETE_RLN,ABSTRACT_RLN,
                                  &roleSpecified,&abstract) == false)
           {
            parseError = true;
            break;
           }
        }
#if DEFRULE_CONSTRUCT
      else if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,MATCH_RLN) == 0)
        {
         if (ParseSimpleQualifier(theEnv,readSource,MATCH_RLN,NONREACTIVE_RLN,REACTIVE_RLN,
                                  &patternMatchSpecified,&reactive) == false)
           {
            parseError = true;
            break;
           }
        }
#endif
      else if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,SLOT_RLN) == 0)
        {
         slots = ParseSlot(theEnv,readSource,cname->contents,slots,preclist,false);
         if (slots == NULL)
           {
            parseError = true;
            break;
           }
        }
      else if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,MLT_SLOT_RLN) == 0)
        {
         slots = ParseSlot(theEnv,readSource,cname->contents,slots,preclist,true);
         if (slots == NULL)
           {
            parseError = true;
            break;
           }
        }
      else if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,HANDLER_DECL) == 0)
        {
         if (ReadUntilClosingParen(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken) == false)
           {
            parseError = true;
            break;
           }
        }
      else
        {
         SyntaxErrorMessage(theEnv,"defclass");
         parseError = true;
         break;
        }
      GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
     }

   if ((DefclassData(theEnv)->ObjectParseToken.tknType != RIGHT_PARENTHESIS_TOKEN) ||
       (parseError == true))
     {
      DeletePackedClassLinks(theEnv,sclasses,true);
      DeletePackedClassLinks(theEnv,preclist,true);
      DeleteSlots(theEnv,slots);
      return true;
     }
   SavePPBuffer(theEnv,"\n");

   /* =========================================================================
      The abstract/reactive qualities of a class are inherited if not specified
      ========================================================================= */
   if (roleSpecified == false)
     {
      if (preclist->classArray[1]->system &&                             /* Change to cause         */
          (DefclassData(theEnv)->ClassDefaultsModeValue == CONVENIENCE_MODE)) /* default role of         */
        { abstract = false; }                                            /* classes to be concrete. */
      else
        { abstract = preclist->classArray[1]->abstract; }
     }
#if DEFRULE_CONSTRUCT
   if (patternMatchSpecified == false)
     {
      if ((preclist->classArray[1]->system) &&                           /* Change to cause       */
          (! abstract) &&                                                /* default pattern-match */
          (DefclassData(theEnv)->ClassDefaultsModeValue == CONVENIENCE_MODE)) /* of classes to be      */
        { reactive = true; }                                             /* reactive.             */
      else
        { reactive = preclist->classArray[1]->reactive; }
     }

   /* ================================================================
      An abstract class cannot have direct instances, thus it makes no
      sense for it to be reactive since it will have no objects to
      respond to pattern-matching
      ================================================================ */
   if (abstract && reactive)
     {
      PrintErrorID(theEnv,"CLASSPSR",1,false);
      WriteString(theEnv,STDERR,"An abstract class cannot be reactive.\n");
      DeletePackedClassLinks(theEnv,sclasses,true);
      DeletePackedClassLinks(theEnv,preclist,true);
      DeleteSlots(theEnv,slots);
      return true;
     }

#endif

   /* =======================================================
      If we're only checking syntax, don't add the
      successfully parsed defclass to the KB.
      ======================================================= */

   if (ConstructData(theEnv)->CheckSyntaxMode)
     {
      DeletePackedClassLinks(theEnv,sclasses,true);
      DeletePackedClassLinks(theEnv,preclist,true);
      DeleteSlots(theEnv,slots);
      return false;
     }

   cls = NewClass(theEnv,cname);
   cls->abstract = abstract;
#if DEFRULE_CONSTRUCT
   cls->reactive = reactive;
#endif
   cls->directSuperclasses.classCount = sclasses->classCount;
   cls->directSuperclasses.classArray = sclasses->classArray;

   /* =======================================================
      This is a hack to let functions which need to iterate
      over a class AND its superclasses to conveniently do so

      The real precedence list starts in position 1
      ======================================================= */
   preclist->classArray[0] = cls;
   cls->allSuperclasses.classCount = preclist->classCount;
   cls->allSuperclasses.classArray = preclist->classArray;
   rtn_struct(theEnv,packedClassLinks,sclasses);
   rtn_struct(theEnv,packedClassLinks,preclist);

   /* =================================
      Shove slots into contiguous array
      ================================= */
   if (slots != NULL)
     PackSlots(theEnv,cls,slots);
   AddClass(theEnv,cls);

   return false;
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************
  NAME         : ValidClassName
  DESCRIPTION  : Determines if a new class of the given
                 name can be defined in the current module
  INPUTS       : 1) The new class name
                 2) Buffer to hold class address
  RETURNS      : True if OK, false otherwise
  SIDE EFFECTS : Error message printed if not OK
  NOTES        : GetConstructNameAndComment() (called before
                 this function) ensures that the defclass
                 name does not conflict with one from
                 another module
 ***********************************************************/
static bool ValidClassName(
  Environment *theEnv,
  const char *theClassName,
  Defclass **theDefclass)
  {
   *theDefclass = FindDefclassInModule(theEnv,theClassName);
   if (*theDefclass != NULL)
     {
      /* ===================================
         System classes (which are visible
         in all modules) cannot be redefined
         =================================== */
      if ((*theDefclass)->system)
        {
         PrintErrorID(theEnv,"CLASSPSR",2,false);
         WriteString(theEnv,STDERR,"Cannot redefine a predefined system class.\n");
         return false;
        }

      /* ===============================================
         A class in the current module can only be
         redefined if it is not in use, e.g., instances,
         generic function method restrictions, etc.
         =============================================== */
      if ((DefclassIsDeletable(*theDefclass) == false) &&
          (! ConstructData(theEnv)->CheckSyntaxMode))
        {
         PrintErrorID(theEnv,"CLASSPSR",3,false);
         WriteString(theEnv,STDERR,"Class '");
         WriteString(theEnv,STDERR,DefclassName(*theDefclass));
         WriteString(theEnv,STDERR,"' cannot be redefined while ");
         WriteString(theEnv,STDERR,"outstanding references to it still exist.\n");
         return false;
        }
     }
   return true;
  }

/***************************************************************
  NAME         : ParseSimpleQualifier
  DESCRIPTION  : Parses abstract/concrete role and
                 pattern-matching reactivity for class
  INPUTS       : 1) The input logical name
                 2) The name of the qualifier being parsed
                 3) The qualifier value indicating that the
                    qualifier should be false
                 4) The qualifier value indicating that the
                    qualifier should be true
                 5) A pointer to a bitmap indicating
                    if the qualifier has already been parsed
                 6) A buffer to store the value of the qualifier
  RETURNS      : True if all OK, false otherwise
  SIDE EFFECTS : Bitmap and qualifier buffers set
                 Messages printed on errors
  NOTES        : None
 ***************************************************************/
static bool ParseSimpleQualifier(
  Environment *theEnv,
  const char *readSource,
  const char *classQualifier,
  const char *clearRelation,
  const char *setRelation,
  bool *alreadyTestedFlag,
  bool *binaryFlag)
  {
   if (*alreadyTestedFlag)
     {
      PrintErrorID(theEnv,"CLASSPSR",4,false);
      WriteString(theEnv,STDERR,"The '");
      WriteString(theEnv,STDERR,classQualifier);
      WriteString(theEnv,STDERR,"' class attribute is already specified.\n");
      return false;
     }
   SavePPBuffer(theEnv," ");
   GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
   if (DefclassData(theEnv)->ObjectParseToken.tknType != SYMBOL_TOKEN)
     goto ParseSimpleQualifierError;
   if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,setRelation) == 0)
     *binaryFlag = true;
   else if (strcmp(DefclassData(theEnv)->ObjectParseToken.lexemeValue->contents,clearRelation) == 0)
     *binaryFlag = false;
   else
     goto ParseSimpleQualifierError;
   GetToken(theEnv,readSource,&DefclassData(theEnv)->ObjectParseToken);
   if (DefclassData(theEnv)->ObjectParseToken.tknType != RIGHT_PARENTHESIS_TOKEN)
     goto ParseSimpleQualifierError;
   *alreadyTestedFlag = true;
   return true;

ParseSimpleQualifierError:
   SyntaxErrorMessage(theEnv,"defclass");
   return false;
  }

/***************************************************
  NAME         : ReadUntilClosingParen
  DESCRIPTION  : Skips over tokens until a ')' is
                 encountered.
  INPUTS       : 1) The logical input source
                 2) A buffer for scanned tokens
  RETURNS      : True if ')' read, otherwise false
                 otherwise
  SIDE EFFECTS : Tokens read
  NOTES        : Expects first token after opening
                 paren has already been scanned
 ***************************************************/
static bool ReadUntilClosingParen(
  Environment *theEnv,
  const char *readSource,
  struct token *inputToken)
  {
   int cnt = 1;
   bool lparen_read = false;

   do
     {
      if (lparen_read == false)
        SavePPBuffer(theEnv," ");
      GetToken(theEnv,readSource,inputToken);
      if (inputToken->tknType == STOP_TOKEN)
        {
         SyntaxErrorMessage(theEnv,"message-handler declaration");
         return false;
        }
      else if (inputToken->tknType == LEFT_PARENTHESIS_TOKEN)
        {
         lparen_read = true;
         cnt++;
        }
      else if (inputToken->tknType == RIGHT_PARENTHESIS_TOKEN)
        {
         cnt--;
         if (lparen_read == false)
           {
            PPBackup(theEnv);
            PPBackup(theEnv);
            SavePPBuffer(theEnv,")");
           }
         lparen_read = false;
        }
      else
        lparen_read = false;
     }
   while (cnt > 0);

   return true;
  }

/*****************************************************************************
  NAME         : AddClass
  DESCRIPTION  : Determines the precedence list of the new class.
                 If it is valid, the routine checks to see if the class
                 already exists.  If it does not, all the subclass
                 links are made from the class's direct superclasses,
                 and the class is inserted in the hash table.  If it
                 does, all sublclasses are deleted. An error will occur
                 if any instances of the class (direct or indirect) exist.
                 If all checks out, the old definition is replaced by the new.
  INPUTS       : The new class description
  RETURNS      : Nothing useful
  SIDE EFFECTS : The class is deleted if there is an error.
  NOTES        : No change in the class graph state will occur
                 if there were any errors.
                 Assumes class is not busy!!!
 *****************************************************************************/
static void AddClass(
  Environment *theEnv,
  Defclass *cls)
  {
   Defclass *ctmp;
#if DEBUGGING_FUNCTIONS
   bool oldTraceInstances = false,
       oldTraceSlots = false;
#endif

   /* ===============================================
      If class does not already exist, insert and
      form progeny links with all direct superclasses
      =============================================== */
   cls->hashTableIndex = HashClass(GetDefclassNamePointer(cls));
   ctmp = FindDefclassInModule(theEnv,DefclassName(cls));

   if (ctmp != NULL)
     {
#if DEBUGGING_FUNCTIONS
      oldTraceInstances = ctmp->traceInstances;
      oldTraceSlots = ctmp->traceSlots;
#endif
      DeleteClassUAG(theEnv,ctmp);
     }
   PutClassInTable(theEnv,cls);

   BuildSubclassLinks(theEnv,cls);
   InstallClass(theEnv,cls,true);
   AddConstructToModule(&cls->header);

   FormInstanceTemplate(theEnv,cls);
   FormSlotNameMap(theEnv,cls);

   AssignClassID(theEnv,cls);

#if DEBUGGING_FUNCTIONS
   if (cls->abstract)
     {
      cls->traceInstances = false;
      cls->traceSlots = false;
     }
   else
     {
      if (oldTraceInstances)
        cls->traceInstances = true;
      if (oldTraceSlots)
        cls->traceSlots = true;
     }
#endif

#if DEBUGGING_FUNCTIONS
   if (GetConserveMemory(theEnv) == false)
     SetDefclassPPForm(theEnv,cls,CopyPPBuffer(theEnv));
#endif

#if DEFMODULE_CONSTRUCT

   /* =========================================
      Create a bitmap indicating whether this
      class is in scope or not for every module
      ========================================= */
   cls->scopeMap = (CLIPSBitMap *) CreateClassScopeMap(theEnv,cls);

#endif

   /* ==============================================
      Define get- and put- handlers for public slots
      ============================================== */
   CreatePublicSlotMessageHandlers(theEnv,cls);
  }

/*******************************************************
  NAME         : BuildSubclassLinks
  DESCRIPTION  : Follows the list of superclasses
                 for a class and puts the class in
                 each of the superclasses' subclass
                 list.
  INPUTS       : The address of the class
  RETURNS      : Nothing useful
  SIDE EFFECTS : The subclass lists for every superclass
                 are modified.
  NOTES        : Assumes the superclass list is formed.
 *******************************************************/
static void BuildSubclassLinks(
  Environment *theEnv,
  Defclass *cls)
  {
   unsigned long i;

   for (i = 0 ; i < cls->directSuperclasses.classCount ; i++)
     AddClassLink(theEnv,&cls->directSuperclasses.classArray[i]->directSubclasses,cls,true,0);
  }

/**********************************************************
  NAME         : FormInstanceTemplate
  DESCRIPTION  : Forms a contiguous array of instance
                  slots for use in creating instances later
                 Also used in determining instance slot
                  indices a priori during handler defns
  INPUTS       : The class
  RETURNS      : Nothing useful
  SIDE EFFECTS : Contiguous array of instance slots formed
  NOTES        : None
 **********************************************************/
static void FormInstanceTemplate(
  Environment *theEnv,
  Defclass *cls)
  {
   TEMP_SLOT_LINK *islots = NULL,*stmp;
   unsigned short scnt = 0;
   unsigned long i;

   /* ========================
      Get direct class's slots
      ======================== */
   islots = MergeSlots(theEnv,islots,cls,&scnt,DIRECT);

   /* ===================================================================
      Get all inherited slots - a more specific slot takes precedence
      over more general, i.e. the first class in the precedence list with
      a particular slot gets to specify its default value
      =================================================================== */
   for (i = 1 ; i < cls->allSuperclasses.classCount ; i++)
     islots = MergeSlots(theEnv,islots,cls->allSuperclasses.classArray[i],&scnt,INHERIT);

   /* ===================================================
      Allocate a contiguous array to store all the slots.
      =================================================== */
   cls->instanceSlotCount = scnt;
   cls->localInstanceSlotCount = 0;
   if (scnt > 0)
     cls->instanceTemplate = (SlotDescriptor **) gm2(theEnv,(scnt * sizeof(SlotDescriptor *)));
   for (i = 0 ; i < scnt ; i++)
     {
      stmp = islots;
      islots = islots->nxt;
      cls->instanceTemplate[i] = stmp->desc;
      if (stmp->desc->shared == 0)
        cls->localInstanceSlotCount++;
      rtn_struct(theEnv,tempSlotLink,stmp);
     }
  }

/**********************************************************
  NAME         : FormSlotNameMap
  DESCRIPTION  : Forms a mapping of the slot name ids into
                 the instance template.  Given the slot
                 name id, this map provides a much faster
                 lookup of a slot.  The id is stored
                 statically in object patterns and can
                 be looked up via a hash table at runtime
                 as well.
  INPUTS       : The class
  RETURNS      : Nothing useful
  SIDE EFFECTS : Contiguous array of integers formed
                 The position in the array corresponding
                 to a slot name id holds an the index
                 into the instance template array holding
                 the slot
                 The max slot name id for the class is
                 also stored to make deletion of the slots
                 easier
  NOTES        : Assumes the instance template has already
                 been formed
 **********************************************************/
static void FormSlotNameMap(
  Environment *theEnv,
  Defclass *cls)
  {
   unsigned i;

   cls->maxSlotNameID = 0;
   cls->slotNameMap = NULL;
   if (cls->instanceSlotCount == 0)
     return;
   for (i = 0 ; i < cls->instanceSlotCount ; i++)
     if (cls->instanceTemplate[i]->slotName->id > cls->maxSlotNameID)
       cls->maxSlotNameID = cls->instanceTemplate[i]->slotName->id;
   cls->slotNameMap = (unsigned *) gm2(theEnv,(sizeof(unsigned) * (cls->maxSlotNameID + 1)));
   for (i = 0 ; i <= cls->maxSlotNameID ; i++)
     cls->slotNameMap[i] = 0;
   for (i = 0 ; i < cls->instanceSlotCount ; i++)
     cls->slotNameMap[cls->instanceTemplate[i]->slotName->id] = i + 1;
  }

/********************************************************************
  NAME         : MergeSlots
  DESCRIPTION  : Adds non-duplicate slots to list and increments
                   slot count for the class instance template
  INPUTS       : 1) The old slot list
                 2) The address of class containing new slots
                 3) Caller's buffer for # of slots
                 4) A flag indicating whether the new list of slots
                    is from the direct parent-class or not.
  RETURNS      : The address of the new expanded list, or NULL
                   for an empty list
  SIDE EFFECTS : The list is expanded
                 Caller's slot count is adjusted.
  NOTES        : Lists are assumed to contain no duplicates
 *******************************************************************/
static TEMP_SLOT_LINK *MergeSlots(
  Environment *theEnv,
  TEMP_SLOT_LINK *old,
  Defclass *cls,
  unsigned short *scnt,
  unsigned short src)
  {
   TEMP_SLOT_LINK *cur,*tmp;
   unsigned int i;
   SlotDescriptor *newSlot;
   
   /*=========================================*/
   /* Process the slots in reverse order      */
   /* since we are pushing them onto a stack. */
   /*=========================================*/
      
   for (i = cls->slotCount; i > 0 ; i--)
     {
      newSlot = &cls->slots[i-1];

      /*=============================================*/
      /* A class can prevent it slots from being     */
      /* propagated to all but its direct instances. */
      /*=============================================*/
      
      if ((newSlot->noInherit == 0) ? true : (src == DIRECT))
        {
         cur = old;
         while ((cur != NULL) ? (newSlot->slotName != cur->desc->slotName) : false)
           cur = cur->nxt;
         if (cur == NULL)
           {
            tmp = get_struct(theEnv,tempSlotLink);
            tmp->desc = newSlot;
            tmp->nxt = old;
            old = tmp;
            (*scnt)++;
           }
        }
     }
     
   return old;
  }

/***********************************************************************
  NAME         : PackSlots
  DESCRIPTION  : Groups class-slots into a contiguous array
                  "slots" field points to array
                  "slotCount" field set
  INPUTS       : 1) The class
                 2) The list of slots
  RETURNS      : Nothing useful
  SIDE EFFECTS : Temporary list deallocated, contiguous array allocated,
                   and nxt pointers linked
                 Class pointer set for slots
  NOTES        : Assumes class->slotCount == 0 && class->slots == NULL
 ***********************************************************************/
static void PackSlots(
  Environment *theEnv,
  Defclass *cls,
  TEMP_SLOT_LINK *slots)
  {
   TEMP_SLOT_LINK *stmp,*sprv;
   long i;

   stmp = slots;
   while  (stmp != NULL)
     {
      stmp->desc->cls = cls;
      cls->slotCount++;
      stmp = stmp->nxt;
     }
   cls->slots = (SlotDescriptor *) gm2(theEnv,(sizeof(SlotDescriptor) * cls->slotCount));
   stmp = slots;
   for (i = 0 ; i < cls->slotCount ; i++)
     {
      sprv = stmp;
      stmp = stmp->nxt;
      GenCopyMemory(SlotDescriptor,1,&(cls->slots[i]),sprv->desc);
      cls->slots[i].sharedValue.desc = &(cls->slots[i]);
      cls->slots[i].sharedValue.value = NULL;
      rtn_struct(theEnv,slotDescriptor,sprv->desc);
      rtn_struct(theEnv,tempSlotLink,sprv);
     }
  }

/*****************************************************************************
  NAME         : CreatePublicSlotMessageHandlers
  DESCRIPTION  : Creates a get-<slot-name> and
                 put-<slot-name> handler for every
                 public slot in a class.

                 The syntax of the message-handlers
                 created are:

                 (defmessage-handler <class> get-<slot-name> primary ()
                    ?self:<slot-name>)

                 For single-field slots:

                 (defmessage-handler <class> put-<slot-name> primary (?value)
                    (bind ?self:<slot-name> ?value))

                 For multifield slots:

                 (defmessage-handler <class> put-<slot-name> primary ($?value)
                    (bind ?self:<slot-name> ?value))
  INPUTS       : The defclass
  RETURNS      : Nothing useful
  SIDE EFFECTS : Message-handlers created
  NOTES        : None
 ******************************************************************************/
static void CreatePublicSlotMessageHandlers(
  Environment *theEnv,
  Defclass *theDefclass)
  {
   long i;
   SlotDescriptor *sd;

   for (i = 0 ; i < theDefclass->slotCount ; i++)
     {
      sd = &theDefclass->slots[i];
        CreateGetAndPutHandlers(theEnv,sd);
     }
   for (i = 0 ; i < theDefclass->handlerCount ; i++)
     theDefclass->handlers[i].system = true;
  }

#endif

#if DEFMODULE_CONSTRUCT && OBJECT_SYSTEM

/********************************************************
  NAME         : CreateClassScopeMap
  DESCRIPTION  : Creates a bitmap where each bit position
                 corresponds to a module id. If the bit
                 is set, the class is in scope for that
                 module, otherwise it is not.
  INPUTS       : The class
  RETURNS      : Nothing useful
  SIDE EFFECTS : Scope bitmap created and attached
  NOTES        : Uses FindImportedConstruct()
 ********************************************************/
void *CreateClassScopeMap(
  Environment *theEnv,
  Defclass *theDefclass)
  {
   unsigned short scopeMapSize;
   char *scopeMap;
   const char *className;
   Defmodule *matchModule, *theModule;
   unsigned long moduleID;
   unsigned int count;
   void *theBitMap;

   className = theDefclass->header.name->contents;
   matchModule = theDefclass->header.whichModule->theModule;

   scopeMapSize = (sizeof(char) * ((GetNumberOfDefmodules(theEnv) / BITS_PER_BYTE) + 1));
   scopeMap = (char *) gm2(theEnv,scopeMapSize);

   ClearBitString(scopeMap,scopeMapSize);
   SaveCurrentModule(theEnv);
   for (theModule = GetNextDefmodule(theEnv,NULL) ;
        theModule != NULL ;
        theModule = GetNextDefmodule(theEnv,theModule))
     {
      SetCurrentModule(theEnv,theModule);
      moduleID = theModule->header.bsaveID;
      if (FindImportedConstruct(theEnv,"defclass",matchModule,
                                className,&count,true,NULL) != NULL)
        SetBitMap(scopeMap,moduleID);
     }
   RestoreCurrentModule(theEnv);
   theBitMap = (CLIPSBitMap *) AddBitMap(theEnv,scopeMap,scopeMapSize);
   IncrementBitMapCount(theBitMap);
   rm(theEnv,scopeMap,scopeMapSize);
   return(theBitMap);
  }

#endif

