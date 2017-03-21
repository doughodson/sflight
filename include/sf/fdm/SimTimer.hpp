
#ifndef _SimTimer_H
#define _SimTimer_H

namespace sf {
namespace xml { class Node; }
namespace fdm {
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: SimTimer
//------------------------------------------------------------------------------
class SimTimer
{
 public:
   SimTimer(FDMGlobals *globals, const double frameRate);
   SimTimer(FDMGlobals *globals, const double frameRate, const long maxFrames);
   virtual ~SimTimer() = default;

   void start();
   void startConstructive();
   void stop();
   void initialize(xml::Node *node);

 private:
   FDMGlobals* globals {};
   double frameRate {};
   long maxFrames {1000000000};
};

}
}

#endif
