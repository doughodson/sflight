
#ifndef __sflight_fdm_SimExec_H__
#define __sflight_fdm_SimExec_H__

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {
class Player;

//------------------------------------------------------------------------------
// Class: SimExec
//------------------------------------------------------------------------------
class SimExec
{
 public:
   SimExec(Player*, const double frameRate);
   SimExec(Player*, const double frameRate, const long maxFrames);
   virtual ~SimExec() = default;

   void start();
   void startConstructive();
   void stop();
   void initialize(xml::Node* node);

 private:
   Player* globals{};
   double frameRate{};
   long maxFrames{1000000000};
};
}
}

#endif
