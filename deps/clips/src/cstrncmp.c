   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  07/30/16             */
   /*                                                     */
   /*          CONSTRAINT CONSTRUCTS-TO-C MODULE          */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements the constructs-to-c feature for       */
/*    constraint records.                                    */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Added allowed-classes slot facet.              */
/*                                                           */
/*            Added environment parameter to GenClose.       */
/*                                                           */
/*      6.30: Added support for path name argument to        */
/*            constructs-to-c.                               */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if CONSTRUCT_COMPILER && (! RUN_TIME)

#include "constant.h"

#include "conscomp.h"
#include "envrnmnt.h"
#include "memalloc.h"
#include "prntutil.h"
#include "router.h"
#include "sysdep.h"

#include "cstrncmp.h"

/***********************************************/
/* ConstraintsToCode: Produces the constraint  */
/*   record code for a run-time module created */
/*   using the constructs-to-c function.       */
/***********************************************/
void ConstraintsToCode(
  Environment *theEnv,
  const char *fileName,
  const char *pathName,
  char *fileNameBuffer,
  unsigned fileID,
  FILE *headerFP,
  unsigned imageID,
  unsigned maxIndices)
  {
   unsigned int i, j, count;
   bool newHeader = true;
   FILE *fp;
   unsigned int version = 1;
   int arrayVersion = 1;
   unsigned short numberOfConstraints = 0;
   CONSTRAINT_RECORD *tmpPtr;

   /*===============================================*/
   /* Count the total number of constraint records. */
   /*===============================================*/

   for (i = 0 ; i < SIZE_CONSTRAINT_HASH; i++)
     {
      for (tmpPtr = ConstraintData(theEnv)->ConstraintHashtable[i];
           tmpPtr != NULL;
           tmpPtr = tmpPtr->next)
        { tmpPtr->bsaveIndex = numberOfConstraints++; }
     }

   /*=====================================================*/
   /* If dynamic constraint checking is disabled, then    */
   /* contraints won't be saved. If there are constraints */
   /* which could be saved, then issue a warning message. */
   /*=====================================================*/

   if ((! GetDynamicConstraintChecking(theEnv)) && (numberOfConstraints != 0))
     {
      numberOfConstraints = 0;
      PrintWarningID(theEnv,"CSTRNCMP",1,false);
      WriteString(theEnv,STDWRN,"Constraints are not saved with a constructs-to-c image\n");
      WriteString(theEnv,STDWRN,"  when dynamic constraint checking is disabled.\n");
     }

   if (numberOfConstraints == 0)
     { return; }

   /*=================================================*/
   /* Print the extern definition in the header file. */
   /*=================================================*/

   for (i = 1; i <= (numberOfConstraints / maxIndices) + 1 ; i++)
     { fprintf(headerFP,"extern CONSTRAINT_RECORD C%d_%d[];\n",imageID,i); }

   /*==================*/
   /* Create the file. */
   /*==================*/

   if ((fp = NewCFile(theEnv,fileName,pathName,fileNameBuffer,fileID,version,false)) == NULL) return;

   /*===================*/
   /* List the entries. */
   /*===================*/

   j = 0;
   count = 0;

   for (i = 0; i < SIZE_CONSTRAINT_HASH; i++)
     {
      for (tmpPtr = ConstraintData(theEnv)->ConstraintHashtable[i];
           tmpPtr != NULL;
           tmpPtr = tmpPtr->next)
        {
         if (newHeader)
           {
            fprintf(fp,"CONSTRAINT_RECORD C%d_%d[] = {\n",imageID,arrayVersion);
            newHeader = false;
           }

         fprintf(fp,"{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                 tmpPtr->anyAllowed,
                 tmpPtr->symbolsAllowed,
                 tmpPtr->stringsAllowed,
                 tmpPtr->floatsAllowed,
                 tmpPtr->integersAllowed,
                 tmpPtr->instanceNamesAllowed,
                 tmpPtr->instanceAddressesAllowed,
                 tmpPtr->externalAddressesAllowed,
                 tmpPtr->factAddressesAllowed,
                 0, /* void allowed */
                 tmpPtr->anyRestriction,
                 tmpPtr->symbolRestriction,
                 tmpPtr->stringRestriction,
                 tmpPtr->floatRestriction,
                 tmpPtr->integerRestriction,
                 tmpPtr->classRestriction,
                 tmpPtr->instanceNameRestriction,
                 tmpPtr->multifieldsAllowed,
                 tmpPtr->singlefieldsAllowed,
                 tmpPtr->installed);

         fprintf(fp,",0,"); /* bsaveIndex */

         PrintHashedExpressionReference(theEnv,fp,tmpPtr->classList,imageID,maxIndices);
         fprintf(fp,",");
         PrintHashedExpressionReference(theEnv,fp,tmpPtr->restrictionList,imageID,maxIndices);
         fprintf(fp,",");
         PrintHashedExpressionReference(theEnv,fp,tmpPtr->minValue,imageID,maxIndices);
         fprintf(fp,",");
         PrintHashedExpressionReference(theEnv,fp,tmpPtr->maxValue,imageID,maxIndices);
         fprintf(fp,",");
         PrintHashedExpressionReference(theEnv,fp,tmpPtr->minFields,imageID,maxIndices);
         fprintf(fp,",");
         PrintHashedExpressionReference(theEnv,fp,tmpPtr->maxFields,imageID,maxIndices);

         /* multifield slot */

         fprintf(fp,",NULL");

         /* next slot */

         if (tmpPtr->next == NULL)
           { fprintf(fp,",NULL,"); }
         else
           {
            if ((j + 1) >= maxIndices)
              { fprintf(fp,",&C%d_%d[%d],",imageID,arrayVersion + 1,0); }
            else
              { fprintf(fp,",&C%d_%d[%d],",imageID,arrayVersion,j + 1); }
           }

         fprintf(fp,"%u,%u",tmpPtr->bucket,tmpPtr->count + 1);

         count++;
         j++;

         if ((count == numberOfConstraints) || (j >= maxIndices))
           {
            fprintf(fp,"}};\n");
            GenClose(theEnv,fp);
            j = 0;
            version++;
            arrayVersion++;
            if (count < numberOfConstraints)
              {
               if ((fp = NewCFile(theEnv,fileName,pathName,fileNameBuffer,1,version,false)) == NULL)
                 { return; }
               newHeader = true;
              }
           }
         else
           { fprintf(fp,"},\n"); }
        }
     }
  }

/**********************************************************/
/* PrintConstraintReference: Prints C code representation */
/*   of a constraint record data structure reference.     */
/**********************************************************/
void PrintConstraintReference(
  Environment *theEnv,
  FILE *fp,
  CONSTRAINT_RECORD *cPtr,
  unsigned int imageID,
  unsigned int maxIndices)
  {
   if ((cPtr == NULL) || (! GetDynamicConstraintChecking(theEnv)))
     { fprintf(fp,"NULL"); }
   else fprintf(fp,"&C%u_%u[%u]",imageID,
                                 (cPtr->bsaveIndex / maxIndices) + 1,
                                 cPtr->bsaveIndex % maxIndices);
  }

#endif /* CONSTRUCT_COMPILER && (! RUN_TIME) */



