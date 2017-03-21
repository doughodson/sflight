
#ifndef _SimTimer_H
#define _SimTimer_H

namespace sf
{
class FDMGlobals;
class Node;

//------------------------------------------------------------------------------
// Class: SimTimer
//------------------------------------------------------------------------------
class SimTimer
{
 public:
   SimTimer(FDMGlobals *globals, const double frameRate);
   SimTimer(FDMGlobals *globals, const double frameRate, const long maxFrames);
   ~SimTimer();

   void start();
   void startConstructive();
   void stop();
   void initialize(Node *node);

 private:
   FDMGlobals* globals {};
   double frameRate {};
   long maxFrames {1000000000};
};

}

#endif
