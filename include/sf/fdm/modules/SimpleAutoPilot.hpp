
#ifndef __AutoPilot_H__
#define __AutoPilot_H__

#include "sf/fdm/modules/FDMModule.hpp"

#include "sf/xml/Node.hpp"

#define TURNTYPE_HDG 0
#define TURNTYPE_TRAJECTORY 1

namespace sf {
namespace xml { class Node; }
namespace fdm {

//------------------------------------------------------------------------------
// Class: SimpleAutoPilot
//------------------------------------------------------------------------------
class SimpleAutoPilot : public FDMModule
{
 public:
   SimpleAutoPilot(FDMGlobals* globals, const double frameRate);
   ~SimpleAutoPilot();

   // module interface
   virtual void initialize(xml::Node *node) override;
   virtual void update(double timestep) override;

   void updateHdg(double timestep, double cmdHdg);
   void updateAlt(double timestep);
   void updateVS(double timestep, double cmdVs);
   void updateSpeed(double timestep);

 protected:
   double kphi {0.05};
   double maxBankRate {0.2};   // rads/sec
   double kalt {};
   double kpitch {};
   double maxG {2.0};
   double minG {};
   double maxG_rate {2.0};
   double minG_rate {};

   double maxThrottle {};
   double minThrottle {};
   double spoolTime {1.0};

   double lastVz {};

   int turnType {};

   double hdgErrTol {};

   bool vsHoldOn {}, altHoldOn {}, hdgHoldOn {};
};
}
}

#endif
