
#ifndef __sflight_fdm_AutoPilot_H__
#define __sflight_fdm_AutoPilot_H__

#include "sflight/fdm/modules/Module.hpp"

#include "sflight/xml/Node.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {

//------------------------------------------------------------------------------
// Class: AutoPilot
// Description: Implements a simple autopilot
//------------------------------------------------------------------------------
class AutoPilot : public Module
{
 public:
   AutoPilot(FDMGlobals* globals, const double frameRate);
   ~AutoPilot();

   // module interface
   virtual void initialize(xml::Node* node) override;
   virtual void update(const double timestep) override;

   void updateHdg(double timestep, double cmdHdg);
   void updateAlt(double timestep);
   void updateVS(double timestep, double cmdVs);
   void updateSpeed(double timestep);

 protected:
   enum class TurnType { HDG = 0, TRAJECTORY = 1 };

   double kphi{0.05};
   double maxBankRate{0.2}; // rads/sec
   double kalt{};
   double kpitch{};
   double maxG{2.0};
   double minG{};
   double maxG_rate{2.0};
   double minG_rate{};

   double maxThrottle{};
   double minThrottle{};
   double spoolTime{1.0};

   double lastVz{};

   TurnType turnType{TurnType::HDG};

   double hdgErrTol{};

   bool vsHoldOn{}, altHoldOn{}, hdgHoldOn{};
};
}
}

#endif
