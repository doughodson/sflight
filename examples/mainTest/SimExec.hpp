
#ifndef __SimExec_H__
#define __SimExec_H__

namespace sflight {
namespace fdm { class Player; }
namespace xml {  class Node; }
}

//------------------------------------------------------------------------------
// Class: SimExec
//------------------------------------------------------------------------------
class SimExec
{
 public:
   SimExec(sflight::fdm::Player*, const double frameRate);
   SimExec(sflight::fdm::Player*, const double frameRate, const long maxFrames);
   virtual ~SimExec() = default;

   void start();
   void startConstructive();
   void stop();
   void initialize(sflight::xml::Node* node);

 private:
   sflight::fdm::Player* globals{};
   double frameRate{};
   long maxFrames{1000000000};
};

#endif
