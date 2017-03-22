
#ifndef __sf_fdm_InterpAero_H__
#define __sf_fdm_InterpAero_H__

#include "sf/fdm/modules/FDMModule.hpp"

namespace sf {
namespace xml {
class Node;
}
namespace fdm {
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: InterpAero
// Description: Sets up a simple aero lookup table system
//------------------------------------------------------------------------------
class InterpAero : public virtual FDMModule
{
 public:
   InterpAero(FDMGlobals* globals, double frameRate);

   // module interface
   virtual void initialize(xml::Node* node) override;
   virtual void update(const double timestep) override;

   void createCoefs(double pitch, double u, double vz, double thrust,
                    double& alpha, double& cl, double& cd);

   double getBetaMach(double mach);

 protected:
   double designWeight{};
   double designAlt{};
   double wingSpan{};
   double wingArea{};
   double thrustRatio{};
   double thrustAngle{};

   double qdes{};

   double cruiseAlpha{};
   double cruiseCL{};
   double cruiseCD{};

   double climbAlpha{};
   double climbCL{};
   double climbCD{};

   double a1{};
   double a2{};
   double b1{};
   double b2{};

   double cdo{};
   double clo{};
   double liftSlope{};
   double stallCL{};
   double wingEffects{};

   bool usingMachEffects{true};
};
}
}

#endif
