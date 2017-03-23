
#ifndef __sflt_fdm_SimExec_H__
#define __sflt_fdm_SimExec_H__

namespace sflt {
namespace xml {
class Node;
}
namespace fdm {
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: SimExec
//------------------------------------------------------------------------------
class SimExec
{
 public:
   SimExec(FDMGlobals* globals, const double frameRate);
   SimExec(FDMGlobals* globals, const double frameRate, const long maxFrames);
   virtual ~SimExec() = default;

   void start();
   void startConstructive();
   void stop();
   void initialize(xml::Node* node);

 private:
   FDMGlobals* globals{};
   double frameRate{};
   long maxFrames{1000000000};
};
}
}

#endif
