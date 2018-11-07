
#ifndef __sflight_mdls_Engine_HPP__
#define __sflight_mdls_Engine_HPP__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_Engine.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
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
   virtual void update(const double timestep) override;

   friend void xml_bindings::init_Engine(xml::Node*, Engine*);

 private:
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
