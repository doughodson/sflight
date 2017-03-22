
#ifndef __sf_fdm_Engine_H__
#define __sf_fdm_Engine_H__

#include "sf/fdm/modules/FDMModule.hpp"

namespace sf {
namespace xml {
class Node;
}
namespace fdm {

//------------------------------------------------------------------------------
// Class: SimpleEngine
// Description: Implements a simple engine
//------------------------------------------------------------------------------
class SimpleEngine : public virtual FDMModule
{
 public:
   SimpleEngine(FDMGlobals* globals, double timestep);
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
