
#ifndef __sflight_fdm_Engine_H__
#define __sflight_fdm_Engine_H__

#include "sflight/fdm/modules/Module.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {
class Player;

//------------------------------------------------------------------------------
// Class: Engine
// Description: Implements a simple engine
//------------------------------------------------------------------------------
class Engine : public Module
{
 public:
   Engine(Player*, const double timestep);
   ~Engine();

   // module interface
   virtual void initialize(xml::Node*) override;
   virtual void update(const double timestep) override;

 protected:
   double thrustRatio{};
   double designWeight{};
   double thrustAngle{};

   double designRho{};
   double designTemp{};
   double designPress{};
   double designMach{};
   double designThrust{};

   double seaLevelTemp{};
   double seaLevelPress{};

   double staticThrust{};
   double thrustSlope{};
   double staticFF{};
   double FFslope{};

   double thrust{};
};
}
}

#endif
