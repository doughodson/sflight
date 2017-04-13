
#ifndef __sflight_fdm_Engine_H__
#define __sflight_fdm_Engine_H__

#include "sflight/fdm/modules/Module.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {

//------------------------------------------------------------------------------
// Class: SimpleEngine
// Description: Implements a simple engine
//------------------------------------------------------------------------------
class SimpleEngine : public Module
{
 public:
   SimpleEngine(FDMGlobals* globals, const double timestep);
   ~SimpleEngine();

   // module interface
   virtual void initialize(xml::Node* node) override;
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