
#ifndef __sflight_mdls_AutoPilot_HPP__
#define __sflight_mdls_AutoPilot_HPP__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_AutoPilot.hpp"

namespace sflight {
namespace xml { class Node; }
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: AutoPilot
// Description: Implements a simple autopilot
//------------------------------------------------------------------------------
class AutoPilot : public Module
{
public:
   AutoPilot(Player*, const double frameRate);
   ~AutoPilot();

   // module interface
   virtual void update(const double timestep) override;

   void updateHdg(const double timestep, const double cmdHdg);
   void updateAlt(const double timestep);
   void updateVS(const double timestep, const double cmdVs);
   void updateSpeed(const double timestep);

   friend void xml_bindings::init_AutoPilot(xml::Node*, AutoPilot*);

private:
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
