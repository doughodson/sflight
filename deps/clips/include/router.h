   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  01/15/18            */
   /*                                                     */
   /*                 ROUTER HEADER FILE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides a centralized mechanism for handling    */
/*   input and output requests.                              */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed conversion of '\r' to '\n' from the    */
/*            EnvGetcRouter function.                        */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Added support for passing context information  */
/*            to the router functions.                       */
/*                                                           */
/*      6.30: Fixed issues with passing context to routers.  */
/*                                                           */
/*            Added AwaitingInput flag.                      */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Added STDOUT and STDIN logical name            */
/*            definitions.                                   */
/*                                                           */
/*      6.40: Added InputBufferCount function.               */
/*                                                           */
/*            Added check for reuse of existing router name. */
/*                                                           */
/*            Removed LOCALE definition.                     */
/*                                                           */
/*            Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Changed return values for router functions.    */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*            Callbacks must be environment aware.           */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*            Removed WPROMPT, WDISPLAY, WTRACE, and WDIALOG */
/*            logical names.                                 */
/*                                                           */
/*************************************************************/

#ifndef _H_router

#pragma once

#define _H_router

#include <stdio.h>

typedef struct router Router;
typedef bool RouterQueryFunction(Environment *,const char *,void *);
typedef void RouterWriteFunction(Environment *,const char *,const char *,void *);
typedef void RouterExitFunction(Environment *,int,void *);
typedef int RouterReadFunction(Environment *,const char *,void *);
typedef int RouterUnreadFunction(Environment *,const char *,int,void *);

extern const char *STDOUT;
extern const char *STDIN;
extern const char *STDERR;
extern const char *STDWRN;

#define ROUTER_DATA 46

struct router
  {
   const char *name;
   bool active;
   int priority;
   void *context;
   RouterQueryFunction *queryCallback;
   RouterWriteFunction *writeCallback;
   RouterExitFunction *exitCallback;
   RouterReadFunction *readCallback;
   RouterUnreadFunction *unreadCallback;
   Router *next;
  };

struct routerData
  {
   size_t CommandBufferInputCount;
   size_t InputUngets;
   bool AwaitingInput;
   const char *LineCountRouter;
   const char *FastCharGetRouter;
   const char *FastCharGetString;
   long FastCharGetIndex;
   struct router *ListOfRouters;
   FILE *FastLoadFilePtr;
   FILE *FastSaveFilePtr;
   bool Abort;
  };

#define RouterData(theEnv) ((struct routerData *) GetEnvironmentData(theEnv,ROUTER_DATA))

   void                           InitializeDefaultRouters(Environment *);
   void                           WriteString(Environment *,const char *,const char *);
   void                           Write(Environment *,const char *);
   void                           Writeln(Environment *,const char *);
   int                            ReadRouter(Environment *,const char *);
   int                            UnreadRouter(Environment *,const char *,int);
   void                           ExitRouter(Environment *,int);
   void                           AbortExit(Environment *);
   bool                           AddRouter(Environment *,const char *,int,
                                            RouterQueryFunction *,RouterWriteFunction *,
                                            RouterReadFunction *,RouterUnreadFunction *,
                                            RouterExitFunction *,void *);
   bool                           DeleteRouter(Environment *,const char *);
   bool                           QueryRouters(Environment *,const char *);
   bool                           DeactivateRouter(Environment *,const char *);
   bool                           ActivateRouter(Environment *,const char *);
   void                           SetFastLoad(Environment *,FILE *);
   void                           SetFastSave(Environment *,FILE *);
   FILE                          *GetFastLoad(Environment *);
   FILE                          *GetFastSave(Environment *);
   void                           UnrecognizedRouterMessage(Environment *,const char *);
   void                           PrintNRouter(Environment *,const char *,const char *,unsigned long);
   size_t                         InputBufferCount(Environment *);
   Router                        *FindRouter(Environment *,const char *);
   bool                           PrintRouterExists(Environment *,const char *);

#endif /* _H_router */
