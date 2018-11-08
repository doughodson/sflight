   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/09/18             */
   /*                                                     */
   /*                  MULTIFIELD MODULE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
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
/*            Corrected code to remove compiler warnings.    */
/*                                                           */
/*            Moved ImplodeMultifield from multifun.c.       */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Used DataObjectToString instead of             */
/*            ValueToString in implode$ to handle            */
/*            print representation of external addresses.    */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Fixed issue with StoreInMultifield when        */
/*            asserting void values in implied deftemplate   */
/*            facts.                                         */
/*                                                           */
/*      6.40: Refactored code to reduce header dependencies  */
/*            in sysdep.c.                                   */
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
/*            The explode$ function via StringToMultifield   */
/*            now converts non-primitive value tokens        */
/*            (such as parentheses) to symbols rather than   */
/*            strings.                                       */
/*                                                           */
/*************************************************************/

#include <stdio.h>

#include "setup.h"

#include "constant.h"
#include "envrnmnt.h"
#include "evaluatn.h"
#include "exprnops.h"
#include "memalloc.h"
#if OBJECT_SYSTEM
#include "object.h"
#endif
#include "scanner.h"
#include "prntutil.h"
#include "router.h"
#include "strngrtr.h"
#include "symbol.h"
#include "utility.h"

#include "multifld.h"

/******************************/
/* CreateUnmanagedMultifield: */
/******************************/
Multifield *CreateUnmanagedMultifield(
  Environment *theEnv,
  size_t size)
  {
   Multifield *theSegment;
   size_t newSize = size;

   if (size == 0) newSize = 1;

   theSegment = get_var_struct(theEnv,multifield,sizeof(struct clipsValue) * (newSize - 1));

   theSegment->header.type = MULTIFIELD_TYPE;
   theSegment->length = size;
   theSegment->busyCount = 0;
   theSegment->next = NULL;

   return theSegment;
  }

/*********************/
/* ReturnMultifield: */
/*********************/
void ReturnMultifield(
  Environment *theEnv,
  Multifield *theSegment)
  {
   size_t newSize;

   if (theSegment == NULL) return;

   if (theSegment->length == 0) newSize = 1;
   else newSize = theSegment->length;

   rtn_var_struct(theEnv,multifield,sizeof(struct clipsValue) * (newSize - 1),theSegment);
  }

/*********************/
/* RetainMultifield: */
/*********************/
void RetainMultifield(
  Environment *theEnv,
  Multifield *theSegment)
  {
   size_t length, i;
   CLIPSValue *contents;

   if (theSegment == NULL) return;

   length = theSegment->length;

   theSegment->busyCount++;
   contents = theSegment->contents;

   for (i = 0 ; i < length ; i++)
     { AtomInstall(theEnv,contents[i].header->type,contents[i].value); }
  }

/**********************/
/* ReleaseMultifield: */
/**********************/
void ReleaseMultifield(
  Environment *theEnv,
  Multifield *theSegment)
  {
   size_t length, i;
   CLIPSValue *contents;

   if (theSegment == NULL) return;

   length = theSegment->length;
   theSegment->busyCount--;
   contents = theSegment->contents;

   for (i = 0 ; i < length ; i++)
     { AtomDeinstall(theEnv,contents[i].header->type,contents[i].value); }
  }

/************************************************/
/* IncrementCLIPSValueMultifieldReferenceCount: */
/************************************************/
void IncrementCLIPSValueMultifieldReferenceCount(
  Environment *theEnv,
  Multifield *theSegment)
  {
   size_t length, i;
   CLIPSValue *contents;

   if (theSegment == NULL) return;

   length = theSegment->length;

   theSegment->busyCount++;
   contents = theSegment->contents;

   for (i = 0 ; i < length ; i++)
     { Retain(theEnv,contents[i].header); }
  }

/************************************************/
/* DecrementCLIPSValueMultifieldReferenceCount: */
/************************************************/
void DecrementCLIPSValueMultifieldReferenceCount(
  Environment *theEnv,
  Multifield *theSegment)
  {
   size_t length, i;
   CLIPSValue *contents;

   if (theSegment == NULL) return;

   length = theSegment->length;
   theSegment->busyCount--;
   contents = theSegment->contents;

   for (i = 0 ; i < length ; i++)
     { Release(theEnv,contents[i].header); }
  }

/*******************************************************/
/* StringToMultifield: Returns a multifield structure  */
/*    that represents the string sent as the argument. */
/*******************************************************/
Multifield *StringToMultifield(
  Environment *theEnv,
  const char *theString)
  {
   struct token theToken;
   Multifield *theSegment;
   CLIPSValue *contents;
   unsigned long numberOfFields = 0;
   struct expr *topAtom = NULL, *lastAtom = NULL, *theAtom;

   /*====================================================*/
   /* Open the string as an input source and read in the */
   /* list of values to be stored in the multifield.     */
   /*====================================================*/

   OpenStringSource(theEnv,"multifield-str",theString,0);

   GetToken(theEnv,"multifield-str",&theToken);
   while (theToken.tknType != STOP_TOKEN)
     {
      if ((theToken.tknType == SYMBOL_TOKEN) || (theToken.tknType == STRING_TOKEN) ||
          (theToken.tknType == FLOAT_TOKEN) || (theToken.tknType == INTEGER_TOKEN) ||
          (theToken.tknType == INSTANCE_NAME_TOKEN))
        { theAtom = GenConstant(theEnv,TokenTypeToType(theToken.tknType),theToken.value); }
      else
        { theAtom = GenConstant(theEnv,SYMBOL_TYPE,CreateSymbol(theEnv,theToken.printForm)); }

      numberOfFields++;
      if (topAtom == NULL) topAtom = theAtom;
      else lastAtom->nextArg = theAtom;

      lastAtom = theAtom;
      GetToken(theEnv,"multifield-str",&theToken);
     }

   CloseStringSource(theEnv,"multifield-str");

   /*====================================================================*/
   /* Create a multifield of the appropriate size for the values parsed. */
   /*====================================================================*/

   theSegment = CreateMultifield(theEnv,numberOfFields);
   contents = theSegment->contents;

   /*====================================*/
   /* Copy the values to the multifield. */
   /*====================================*/

   theAtom = topAtom;
   numberOfFields = 0;
   while (theAtom != NULL)
     {
      contents[numberOfFields].value = theAtom->value;
      numberOfFields++;
      theAtom = theAtom->nextArg;
     }

   /*===========================*/
   /* Return the parsed values. */
   /*===========================*/

   ReturnExpression(theEnv,topAtom);

   /*============================*/
   /* Return the new multifield. */
   /*============================*/

   return(theSegment);
  }

/**********************/
/* ArrayToMultifield: */
/**********************/
Multifield *ArrayToMultifield(
  Environment *theEnv,
  CLIPSValue *theArray,
  unsigned long size) // TBD size_t
  {
   Multifield *rv;
   unsigned int i;
   
   rv = CreateMultifield(theEnv,size);
   
   for (i = 0; i < size; i++)
     { rv->contents[i].value = theArray[i].value; }
   
   return rv;
  }

/***************************************************/
/* EmptyMultifield: Creates a multifield of length */
/*   0 and adds it to the list of segments.        */
/***************************************************/
Multifield *EmptyMultifield(
  Environment *theEnv)
  {
   return CreateMultifield(theEnv,0);
  }

/***********************************************************/
/* CreateMultifield: Creates a multifield of the specified */
/*   size and adds it to the list of segments.             */
/***********************************************************/
Multifield *CreateMultifield(
  Environment *theEnv,
  size_t size)
  {
   Multifield *theSegment;
   size_t newSize;

   if (size == 0) newSize = 1;
   else newSize = size;

   theSegment = get_var_struct(theEnv,multifield,sizeof(struct clipsValue) * (newSize - 1));

   theSegment->header.type = MULTIFIELD_TYPE;
   theSegment->length = size;
   theSegment->busyCount = 0;
   theSegment->next = NULL;

   theSegment->next = UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields;
   UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields = theSegment;
   UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
   if (UtilityData(theEnv)->CurrentGarbageFrame->LastMultifield == NULL)
     { UtilityData(theEnv)->CurrentGarbageFrame->LastMultifield = theSegment; }

   return theSegment;
  }

/*******************/
/* DOToMultifield: */
/*******************/
Multifield *DOToMultifield(
  Environment *theEnv,
  UDFValue *theValue)
  {
   Multifield *dst, *src;

   if (theValue->header->type != MULTIFIELD_TYPE) return NULL;

   dst = CreateUnmanagedMultifield(theEnv,(unsigned long) theValue->range);

   src = theValue->multifieldValue;
   GenCopyMemory(struct clipsValue,dst->length,
              &(dst->contents[0]),&(src->contents[theValue->begin]));

   return dst;
  }

/************************/
/* AddToMultifieldList: */
/************************/
void AddToMultifieldList(
  Environment *theEnv,
  Multifield *theSegment)
  {
   theSegment->next = UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields;
   UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields = theSegment;
   UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
   if (UtilityData(theEnv)->CurrentGarbageFrame->LastMultifield == NULL)
     { UtilityData(theEnv)->CurrentGarbageFrame->LastMultifield = theSegment; }
  }

/*********************/
/* FlushMultifields: */
/*********************/
void FlushMultifields(
  Environment *theEnv)
  {
   Multifield *theSegment, *nextPtr, *lastPtr = NULL;
   size_t newSize;

   theSegment = UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields;
   while (theSegment != NULL)
     {
      nextPtr = theSegment->next;
      if (theSegment->busyCount == 0)
        {
         if (theSegment->length == 0) newSize = 1;
         else newSize = theSegment->length;
         rtn_var_struct(theEnv,multifield,sizeof(struct clipsValue) * (newSize - 1),theSegment);
         if (lastPtr == NULL) UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields = nextPtr;
         else lastPtr->next = nextPtr;

         /*=================================================*/
         /* If the multifield deleted was the last in the   */
         /* list, update the pointer to the last multifield */
         /* to the prior multifield.                        */
         /*=================================================*/

         if (nextPtr == NULL)
           { UtilityData(theEnv)->CurrentGarbageFrame->LastMultifield = lastPtr; }
        }
      else
        { lastPtr = theSegment; }

      theSegment = nextPtr;
     }
  }

/********************/
/* CLIPSToUDFValue: */
/********************/
void CLIPSToUDFValue(
  CLIPSValue *cv,
  UDFValue *uv)
  {
   uv->value = cv->value;
   if (cv->header->type == MULTIFIELD_TYPE)
     {
      uv->begin = 0;
      uv->range = cv->multifieldValue->length;
     }
  }

/********************/
/* UDFToCLIPSValue: */
/********************/
void UDFToCLIPSValue(
  Environment *theEnv,
  UDFValue *uv,
  CLIPSValue *cv)
  {
   Multifield *copy;
   
   if (uv->header->type != MULTIFIELD_TYPE)
     {
      cv->value = uv->value;
      return;
     }
   
   if ((uv->begin == 0) &&
       (uv->range == uv->multifieldValue->length))
     {
      cv->multifieldValue = uv->multifieldValue;
      return;
     }
     
   copy = CreateMultifield(theEnv,uv->range);
   GenCopyMemory(struct clipsValue,uv->range,&copy->contents[0],
                 &uv->multifieldValue->contents[uv->begin]);
      
   cv->multifieldValue = copy;
  }

/************************************************/
/* NormalizeMultifield: Allocates a new segment */
/*   and copies results from old value to new.  */
/************************************************/
void NormalizeMultifield(
  Environment *theEnv,
  UDFValue *theMF)
  {
   Multifield *copy;
   
   if (theMF->header->type != MULTIFIELD_TYPE) return;
   
   if ((theMF->begin == 0) &&
       (theMF->range == theMF->multifieldValue->length))
     { return; }

   copy = CreateMultifield(theEnv,theMF->range);
   GenCopyMemory(struct clipsValue,theMF->range,&copy->contents[0],
                 &theMF->multifieldValue->contents[theMF->begin]);
   theMF->begin = 0;
   theMF->value = copy;
  }

/************************************************************************/
/* DuplicateMultifield: Allocates a new segment and copies results from */
/*   old value to new. This value is not put on the ListOfMultifields.  */
/************************************************************************/
void DuplicateMultifield(
  Environment *theEnv,
  UDFValue *dst,
  UDFValue *src)
  {
   dst->begin = 0;
   dst->range = src->range;
   dst->value = CreateUnmanagedMultifield(theEnv,(unsigned long) dst->range);
   GenCopyMemory(struct clipsValue,dst->range,&dst->multifieldValue->contents[0],
                                         &src->multifieldValue->contents[src->begin]);
  }

/*******************/
/* CopyMultifield: */
/*******************/
Multifield *CopyMultifield(
  Environment *theEnv,
  Multifield *src)
  {
   Multifield *dst;

   dst = CreateUnmanagedMultifield(theEnv,src->length);
   GenCopyMemory(struct clipsValue,src->length,&(dst->contents[0]),&(src->contents[0]));
   return dst;
  }

/**********************************************************/
/* EphemerateMultifield: Marks the values of a multifield */
/*   as ephemeral if they have not already been marker.   */
/**********************************************************/
void EphemerateMultifield(
  Environment *theEnv,
  Multifield *theSegment)
  {
   size_t length, i;
   CLIPSValue *contents;

   if (theSegment == NULL) return;

   length = theSegment->length;

   contents = theSegment->contents;

   for (i = 0 ; i < length ; i++)
     { EphemerateValue(theEnv,contents[i].value); }
  }

/*********************************************/
/* WriteMultifield: Prints out a multifield. */
/*********************************************/
void WriteMultifield(
  Environment *theEnv,
  const char *fileid,
  Multifield *segment)
  {
   PrintMultifieldDriver(theEnv,fileid,segment,0,segment->length,true);
  }
  
/***************************************************/
/* PrintMultifieldDriver: Prints out a multifield. */
/***************************************************/
void PrintMultifieldDriver(
  Environment *theEnv,
  const char *fileid,
  Multifield *segment,
  size_t begin,
  size_t range,
  bool printParens)
  {
   CLIPSValue *theMultifield;
   size_t i;

   theMultifield = segment->contents;
   
   if (printParens)
     { WriteString(theEnv,fileid,"("); }

   for (i = 0; i < range; i++)
     {
      PrintAtom(theEnv,fileid,theMultifield[begin+i].header->type,theMultifield[begin+i].value);
     
      if ((i + 1) < range)
        { WriteString(theEnv,fileid," "); }
     }

   if (printParens)
     { WriteString(theEnv,fileid,")"); }
  }

/****************************************************/
/* StoreInMultifield: Append function for segments. */
/****************************************************/
void StoreInMultifield(
  Environment *theEnv,
  UDFValue *returnValue,
  Expression *expptr,
  bool garbageSegment)
  {
   UDFValue val_ptr;
   UDFValue *val_arr;
   Multifield *theMultifield;
   Multifield *orig_ptr;
   size_t start, range;
   size_t i, j, k;
   unsigned int argCount;
   unsigned long seg_size;

   argCount = CountArguments(expptr);

   /*=========================================*/
   /* If no arguments are given return a NULL */
   /* multifield of length zero.              */
   /*=========================================*/

   if (argCount == 0)
     {
      returnValue->begin = 0;
      returnValue->range = 0;
      if (garbageSegment) theMultifield = CreateMultifield(theEnv,0L);
      else theMultifield = CreateUnmanagedMultifield(theEnv,0L);
      returnValue->value = theMultifield;
      return;
     }
   else
     {
      /*========================================*/
      /* Get a new segment with length equal to */
      /* the total length of all the arguments. */
      /*========================================*/

      val_arr = (UDFValue *) gm2(theEnv,sizeof(UDFValue) * argCount);
      seg_size = 0;

      for (i = 1; i <= argCount; i++, expptr = expptr->nextArg)
        {
         EvaluateExpression(theEnv,expptr,&val_ptr);
         if (EvaluationData(theEnv)->EvaluationError)
           {
            returnValue->begin = 0;
            returnValue->range = 0;
            if (garbageSegment)
              { theMultifield = CreateMultifield(theEnv,0L); }
            else theMultifield = CreateUnmanagedMultifield(theEnv,0L);
            returnValue->value = theMultifield;
            rm(theEnv,val_arr,sizeof(UDFValue) * argCount);
            return;
           }
         if (val_ptr.header->type == MULTIFIELD_TYPE)
           {
            (val_arr+i-1)->value = val_ptr.value;
            start = val_ptr.begin;
            range = val_ptr.range;
           }
         else if (val_ptr.header->type == VOID_TYPE)
           {
            (val_arr+i-1)->value = val_ptr.value;
            start = 0;
            range = 0;
           }
         else
           {
            (val_arr+i-1)->value = val_ptr.value;
            start = 0;
            range = 1;
           }

         seg_size += (unsigned long) range;
         (val_arr+i-1)->begin = start;
         (val_arr+i-1)->range = range;
        }

      if (garbageSegment)
        { theMultifield = CreateMultifield(theEnv,seg_size); }
      else theMultifield = CreateUnmanagedMultifield(theEnv,seg_size);

      /*========================================*/
      /* Copy each argument into new segment.  */
      /*========================================*/

      for (k = 0, j = 0; k < argCount; k++)
        {
         if ((val_arr+k)->header->type == MULTIFIELD_TYPE)
           {
            start = (val_arr+k)->begin;
            range = (val_arr+k)->range;
            orig_ptr = (val_arr+k)->multifieldValue;
            for (i = start; i < (start + range); i++, j++)
              {
               theMultifield->contents[j].value = orig_ptr->contents[i].value;
              }
           }
         else if ((val_arr+k)->header->type != VOID_TYPE)
           {
            theMultifield->contents[j].value = (val_arr+k)->value;
            j++;
           }
        }

      /*=========================*/
      /* Return the new segment. */
      /*=========================*/

      returnValue->begin = 0;
      returnValue->range = seg_size;
      returnValue->value = theMultifield;
      rm(theEnv,val_arr,sizeof(UDFValue) * argCount);
      return;
     }
  }

/*************************************************************/
/* MultifieldDOsEqual: Determines if two segments are equal. */
/*************************************************************/
bool MultifieldDOsEqual(
  UDFValue *dobj1,
  UDFValue *dobj2)
  {
   size_t extent1, extent2;
   CLIPSValue *e1, *e2;

   extent1 = dobj1->range;
   extent2 = dobj2->range;
   
   if (extent1 != extent2)
     { return false; }

   e1 = &dobj1->multifieldValue->contents[dobj1->begin];
   e2 = &dobj2->multifieldValue->contents[dobj2->begin];

   while (extent1 > 0)
     {
      if (e1->value != e2->value)
        { return false; }

      extent1--;

      if (extent1 > 0)
        {
         e1++;
         e2++;
        }
     }
     
   return true;
  }

/******************************************************************/
/* MultifieldsEqual: Determines if two multifields are identical. */
/******************************************************************/
bool MultifieldsEqual(
  Multifield *segment1,
  Multifield *segment2)
  {
   CLIPSValue *elem1;
   CLIPSValue *elem2;
   size_t length, i = 0;

   length = segment1->length;
   if (length != segment2->length)
     { return false; }

   elem1 = segment1->contents;
   elem2 = segment2->contents;

   /*==================================================*/
   /* Compare each field of both facts until the facts */
   /* match completely or the facts mismatch.          */
   /*==================================================*/

   while (i < length)
     {
      if (elem1[i].header->type == MULTIFIELD_TYPE)
        {
         if (MultifieldsEqual(elem1[i].multifieldValue,
                              elem2[i].multifieldValue) == false)
          { return false; }
        }
      else if (elem1[i].value != elem2[i].value)
        { return false; }

      i++;
     }
   return true;
  }

/************************************************************/
/* HashMultifield: Returns the hash value for a multifield. */
/************************************************************/
size_t HashMultifield(
  Multifield *theSegment,
  size_t theRange)
  {
   size_t length, i;
   size_t tvalue;
   size_t count;
   CLIPSValue *fieldPtr;
   union
     {
      double fv;
      void *vv;
      unsigned long liv;
     } fis;

   /*================================================*/
   /* Initialize variables for computing hash value. */
   /*================================================*/

   count = 0;
   length = theSegment->length;
   fieldPtr = theSegment->contents;

   /*====================================================*/
   /* Loop through each value in the multifield, compute */
   /* its hash value, and add it to the running total.   */
   /*====================================================*/

   for (i = 0;
        i < length;
        i++)
     {
      switch(fieldPtr[i].header->type)
         {
          case MULTIFIELD_TYPE:
            count += HashMultifield(fieldPtr[i].multifieldValue,theRange);
            break;

          case FLOAT_TYPE:
            fis.liv = 0;
            fis.fv = fieldPtr[i].floatValue->contents;
            count += (fis.liv * (i + 29))  +
                     (unsigned long) fieldPtr[i].floatValue->contents;
            break;

          case INTEGER_TYPE:
            count += (((unsigned long) fieldPtr[i].integerValue->contents) * (i + 29)) +
                      ((unsigned long) fieldPtr[i].integerValue->contents);
            break;

          case FACT_ADDRESS_TYPE:
#if OBJECT_SYSTEM
          case INSTANCE_ADDRESS_TYPE:
#endif
            fis.liv = 0;
            fis.vv = fieldPtr[i].value;
            count += fis.liv * (i + 29);
            break;

          case EXTERNAL_ADDRESS_TYPE:
            fis.liv = 0;
            fis.vv = fieldPtr[i].externalAddressValue->contents;
            count += fis.liv * (i + 29);
            break;

          case SYMBOL_TYPE:
          case STRING_TYPE:
#if OBJECT_SYSTEM
          case INSTANCE_NAME_TYPE:
#endif
            tvalue = HashSymbol(fieldPtr[i].lexemeValue->contents,theRange);
            count += tvalue * (i + 29);
            break;
         }
     }

   /*========================*/
   /* Return the hash value. */
   /*========================*/

   return count;
  }

/**********************/
/* GetMultifieldList: */
/**********************/
Multifield *GetMultifieldList(
  Environment *theEnv)
  {
   return(UtilityData(theEnv)->CurrentGarbageFrame->ListOfMultifields);
  }

/***************************************/
/* ImplodeMultifield: C access routine */
/*   for the implode$ function.        */
/***************************************/
CLIPSLexeme *ImplodeMultifield(
  Environment *theEnv,
  UDFValue *value)
  {
   size_t strsize = 0;
   size_t i, j;
   const char *tmp_str;
   char *ret_str;
   CLIPSLexeme *rv;
   Multifield *theMultifield;
   UDFValue tempDO;

   /*===================================================*/
   /* Determine the size of the string to be allocated. */
   /*===================================================*/

   theMultifield = value->multifieldValue;
   for (i = value->begin ; i < (value->begin + value->range) ; i++)
     {
      if (theMultifield->contents[i].header->type == FLOAT_TYPE)
        {
         tmp_str = FloatToString(theEnv,theMultifield->contents[i].floatValue->contents);
         strsize += strlen(tmp_str) + 1;
        }
      else if (theMultifield->contents[i].header->type == INTEGER_TYPE)
        {
         tmp_str = LongIntegerToString(theEnv,theMultifield->contents[i].integerValue->contents);
         strsize += strlen(tmp_str) + 1;
        }
      else if (theMultifield->contents[i].header->type == STRING_TYPE)
        {
         strsize += strlen(theMultifield->contents[i].lexemeValue->contents) + 3;
         tmp_str = theMultifield->contents[i].lexemeValue->contents;
         while(*tmp_str)
           {
            if (*tmp_str == '"')
              { strsize++; }
            else if (*tmp_str == '\\') /* GDR 111599 #835 */
              { strsize++; }           /* GDR 111599 #835 */
            tmp_str++;
           }
        }
      else
        {
         tempDO.value = theMultifield->contents[i].value;
         strsize += strlen(DataObjectToString(theEnv,&tempDO)) + 1;
        }
     }

   /*=============================================*/
   /* Allocate the string and copy all components */
   /* of the MULTIFIELD_TYPE variable to it.      */
   /*=============================================*/

   if (strsize == 0) return(CreateString(theEnv,""));
   ret_str = (char *) gm2(theEnv,strsize);
   for(j = 0, i = value->begin ; i < (value->begin + value->range) ; i++)
     {
      /*============================*/
      /* Convert numbers to strings */
      /*============================*/

      if (theMultifield->contents[i].header->type == FLOAT_TYPE)
        {
         tmp_str = FloatToString(theEnv,theMultifield->contents[i].floatValue->contents);
         while(*tmp_str)
           {
            *(ret_str+j) = *tmp_str;
            j++;
            tmp_str++;
           }
        }
      else if (theMultifield->contents[i].header->type == INTEGER_TYPE)
        {
         tmp_str = LongIntegerToString(theEnv,theMultifield->contents[i].integerValue->contents);
         while(*tmp_str)
           {
            *(ret_str+j) = *tmp_str;
            j++;
            tmp_str++;
           }
        }

      /*=======================================*/
      /* Enclose strings in quotes and preceed */
      /* imbedded quotes with a backslash      */
      /*=======================================*/

      else if (theMultifield->contents[i].header->type == STRING_TYPE)
        {
         tmp_str = theMultifield->contents[i].lexemeValue->contents;
         *(ret_str+j) = '"';
         j++;
         while(*tmp_str)
           {
            if (*tmp_str == '"')
              {
               *(ret_str+j) = '\\';
               j++;
              }
            else if (*tmp_str == '\\') /* GDR 111599 #835 */
              {                        /* GDR 111599 #835 */
               *(ret_str+j) = '\\';    /* GDR 111599 #835 */
               j++;                    /* GDR 111599 #835 */
              }                        /* GDR 111599 #835 */

            *(ret_str+j) = *tmp_str;
            j++;
            tmp_str++;
           }
         *(ret_str+j) = '"';
         j++;
        }
      else
        {
         tempDO.value = theMultifield->contents[i].value;
         tmp_str = DataObjectToString(theEnv,&tempDO);
         while(*tmp_str)
           {
            *(ret_str+j) = *tmp_str;
            j++;
            tmp_str++;
           }
         }
      *(ret_str+j) = ' ';
      j++;
     }
   *(ret_str+j-1) = '\0';

   /*====================*/
   /* Return the string. */
   /*====================*/

   rv = CreateString(theEnv,ret_str);
   rm(theEnv,ret_str,strsize);
   return rv;
  }

/****************************/
/* CreateMultifieldBuilder: */
/****************************/
MultifieldBuilder *CreateMultifieldBuilder(
  Environment *theEnv,
  size_t theSize)
  {
   MultifieldBuilder *theMB;
   
   theMB = get_struct(theEnv,multifieldBuilder);
   
   theMB->mbEnv = theEnv;
   theMB->bufferReset = theSize;
   theMB->bufferMaximum = theSize;
   theMB->length = 0;
   
   if (theSize == 0)
     { theMB->contents = NULL; }
   else
     { theMB->contents = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * theSize); }
   
   return theMB;
  }

/*********************/
/* MBAppendUDFValue: */
/*********************/
void MBAppendUDFValue(
  MultifieldBuilder *theMB,
  UDFValue *theValue)
  {
   Environment *theEnv = theMB->mbEnv;
   size_t i, neededSize, newSize;
   size_t j;
   CLIPSValue *newArray;

   /*==============================================*/
   /* A void value can't be added to a multifield. */
   /*==============================================*/
   
   if (theValue->header->type == VOID_TYPE)
     { return; }

   /*=======================================*/
   /* Determine the amount of space needed. */
   /*=======================================*/
   
   if (theValue->header->type == MULTIFIELD_TYPE)
     {
      if (theValue->range == 0)
        { return; }
        
      neededSize = theMB->length + theValue->range;
     }
   else
     { neededSize = theMB->length + 1; }

   /*============================================*/
   /* Increase the size of the buffer if needed. */
   /*============================================*/
   
   if (neededSize > theMB->bufferMaximum)
     {
      newSize = neededSize * 2;
      
      newArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * newSize);
      
      for (i = 0; i < theMB->length; i++)
        { newArray[i] = theMB->contents[i]; }
        
      if (theMB->bufferMaximum != 0)
        { rm(theMB->mbEnv,theMB->contents,sizeof(CLIPSValue) * theMB->bufferMaximum); }
        
      theMB->bufferMaximum = newSize;
      theMB->contents = newArray;
     }
     
   /*===================================*/
   /* Copy the new values to the array. */
   /*===================================*/

   if (theValue->header->type == MULTIFIELD_TYPE)
     {
      for (j = theValue->begin; j < (theValue->begin +theValue->range); j++)
        {
         theMB->contents[theMB->length].value = theValue->multifieldValue->contents[j].value;
         Retain(theEnv,theMB->contents[theMB->length].header);
         theMB->length++;
        }
     }
   else
     {
      theMB->contents[theMB->length].value = theValue->value;
      Retain(theEnv,theMB->contents[theMB->length].header);
      theMB->length++;
     }
  }

/*************/
/* MBAppend: */
/*************/
void MBAppend(
  MultifieldBuilder *theMB,
  CLIPSValue *theValue)
  {
   Environment *theEnv = theMB->mbEnv;
   size_t i, neededSize, newSize;
   size_t j;
   CLIPSValue *newArray;

   /*==============================================*/
   /* A void value can't be added to a multifield. */
   /*==============================================*/
   
   if (theValue->header->type == VOID_TYPE)
     { return; }

   /*=======================================*/
   /* Determine the amount of space needed. */
   /*=======================================*/
   
   if (theValue->header->type == MULTIFIELD_TYPE)
     {
      if (theValue->multifieldValue->length == 0)
        { return; }
        
      neededSize = theMB->length + theValue->multifieldValue->length;
     }
   else
     { neededSize = theMB->length + 1; }

   /*============================================*/
   /* Increase the size of the buffer if needed. */
   /*============================================*/
   
   if (neededSize > theMB->bufferMaximum)
     {
      newSize = neededSize * 2;
      
      newArray = (CLIPSValue *) gm2(theEnv,sizeof(CLIPSValue) * newSize);
      
      for (i = 0; i < theMB->length; i++)
        { newArray[i] = theMB->contents[i]; }
        
      if (theMB->bufferMaximum != 0)
        { rm(theMB->mbEnv,theMB->contents,sizeof(CLIPSValue) * theMB->bufferMaximum); }
        
      theMB->bufferMaximum = newSize;
      theMB->contents = newArray;
     }
     
   /*===================================*/
   /* Copy the new values to the array. */
   /*===================================*/

   if (theValue->header->type == MULTIFIELD_TYPE)
     {
      for (j = 0; j < theValue->multifieldValue->length; j++)
        {
         theMB->contents[theMB->length].value = theValue->multifieldValue->contents[j].value;
         Retain(theEnv,theMB->contents[theMB->length].header);
         theMB->length++;
        }
     }
   else
     {
      theMB->contents[theMB->length].value = theValue->value;
      Retain(theEnv,theMB->contents[theMB->length].header);
      theMB->length++;
     }
  }

/*************************/
/* MBAppendCLIPSInteger: */
/*************************/
void MBAppendCLIPSInteger(
  MultifieldBuilder *theMB,
  CLIPSInteger *pv)
  {
   CLIPSValue theValue;
   
   theValue.integerValue = pv;
   MBAppend(theMB,&theValue);
  }

/********************/
/* MBAppendInteger: */
/********************/
void MBAppendInteger(
  MultifieldBuilder *theMB,
  long long intValue)
  {
   CLIPSValue theValue;
   CLIPSInteger *pv = CreateInteger(theMB->mbEnv,intValue);
   
   theValue.integerValue = pv;
   MBAppend(theMB,&theValue);
  }

/***********************/
/* MBAppendCLIPSFloat: */
/***********************/
void MBAppendCLIPSFloat(
  MultifieldBuilder *theMB,
  CLIPSFloat *pv)
  {
   CLIPSValue theValue;
   
   theValue.floatValue = pv;
   MBAppend(theMB,&theValue);
  }

/******************/
/* MBAppendFloat: */
/******************/
void MBAppendFloat(
  MultifieldBuilder *theMB,
  double floatValue)
  {
   CLIPSValue theValue;
   CLIPSFloat *pv = CreateFloat(theMB->mbEnv,floatValue);
   
   theValue.floatValue = pv;
   MBAppend(theMB,&theValue);
  }

/************************/
/* MBAppendCLIPSLexeme: */
/************************/
void MBAppendCLIPSLexeme(
  MultifieldBuilder *theMB,
  CLIPSLexeme *pv)
  {
   CLIPSValue theValue;
   
   theValue.lexemeValue = pv;
   MBAppend(theMB,&theValue);
  }

/*******************/
/* MBAppendSymbol: */
/*******************/
void MBAppendSymbol(
  MultifieldBuilder *theMB,
  const char *strValue)
  {
   CLIPSValue theValue;
   CLIPSLexeme *pv = CreateSymbol(theMB->mbEnv,strValue);
   
   theValue.lexemeValue = pv;
   MBAppend(theMB,&theValue);
  }

/*******************/
/* MBAppendString: */
/*******************/
void MBAppendString(
  MultifieldBuilder *theMB,
  const char *strValue)
  {
   CLIPSValue theValue;
   CLIPSLexeme *pv = CreateString(theMB->mbEnv,strValue);
   
   theValue.lexemeValue = pv;
   MBAppend(theMB,&theValue);
  }

/*************************/
/* MBAppendInstanceName: */
/*************************/
void MBAppendInstanceName(
  MultifieldBuilder *theMB,
  const char *strValue)
  {
   CLIPSValue theValue;
   CLIPSLexeme *pv = CreateInstanceName(theMB->mbEnv,strValue);
   
   theValue.lexemeValue = pv;
   MBAppend(theMB,&theValue);
  }

/*********************************/
/* MBAppendCLIPSExternalAddress: */
/*********************************/
void MBAppendCLIPSExternalAddress(
  MultifieldBuilder *theMB,
  CLIPSExternalAddress *pv)
  {
   CLIPSValue theValue;
   
   theValue.externalAddressValue = pv;
   MBAppend(theMB,&theValue);
  }

/*****************/
/* MBAppendFact: */
/*****************/
void MBAppendFact(
  MultifieldBuilder *theMB,
  Fact *pv)
  {
   CLIPSValue theValue;
   
   theValue.factValue = pv;
   MBAppend(theMB,&theValue);
  }

/*********************/
/* MBAppendInstance: */
/*********************/
void MBAppendInstance(
  MultifieldBuilder *theMB,
  Instance *pv)
  {
   CLIPSValue theValue;
   
   theValue.instanceValue = pv;
   MBAppend(theMB,&theValue);
  }

/***********************/
/* MBAppendMultifield: */
/***********************/
void MBAppendMultifield(
  MultifieldBuilder *theMB,
  Multifield *pv)
  {
   CLIPSValue theValue;
   
   theValue.multifieldValue = pv;
   MBAppend(theMB,&theValue);
  }

/*************/
/* MBCreate: */
/*************/
Multifield *MBCreate(
  MultifieldBuilder *theMB)
  {
   size_t i;
   Multifield *rv;
   
   rv = CreateMultifield(theMB->mbEnv,theMB->length);
   
   if (rv == NULL) return NULL;
   
   for (i = 0; i < theMB->length; i++)
     {
      rv->contents[i].value = theMB->contents[i].value;
      Release(theMB->mbEnv,rv->contents[i].header);
     }

   theMB->length = 0;
   
   return rv;
  }

/************/
/* MBReset: */
/************/
void MBReset(
  MultifieldBuilder *theMB)
  {
   size_t i;
   
   for (i = 0; i < theMB->length; i++)
     { Release(theMB->mbEnv,theMB->contents[i].header); }
     
   if (theMB->bufferReset != theMB->bufferMaximum)
     {
      if (theMB->bufferMaximum != 0)
        { rm(theMB->mbEnv,theMB->contents,sizeof(CLIPSValue) * theMB->bufferMaximum); }
      
      if (theMB->bufferReset == 0)
        { theMB->contents = NULL; }
      else
        { theMB->contents = (CLIPSValue *) gm2(theMB->mbEnv,sizeof(CLIPSValue) * theMB->bufferReset); }
      
      theMB->bufferMaximum = theMB->bufferReset;
     }
     
   theMB->length = 0;
  }

/**************/
/* MBDispose: */
/**************/
void MBDispose(
  MultifieldBuilder *theMB)
  {
   Environment *theEnv = theMB->mbEnv;
   size_t i;
   
   for (i = 0; i < theMB->length; i++)
     { Release(theMB->mbEnv,theMB->contents[i].header); }
   
   if (theMB->bufferMaximum != 0)
     { rm(theMB->mbEnv,theMB->contents,sizeof(CLIPSValue) * theMB->bufferMaximum); }
     
   rtn_struct(theEnv,multifieldBuilder,theMB);
  }
