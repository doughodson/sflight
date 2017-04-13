
#ifndef __sflight_fdm_InverseDesign_H__
#define __sflight_fdm_InverseDesign_H__

#include "sflight/fdm/modules/Module.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: InverseDesign
// Description: Sets up a simple aero lookup table system
//------------------------------------------------------------------------------
class InverseDesign : public Module
{
 public:
   InverseDesign(FDMGlobals* globals, const double frameRate);

   // module interface
   virtual void initialize(xml::Node* node) override;
   virtual void update(const double timestep) override;

   void getAeroCoefs(double pitch, double u, double vz, double rho,
                     double weight, double thrust, double& alpha, double& cl,
                     double& cd);

   double getThrust(double rho, double mach, double throttle);

   double getFuelFlow(double rho, double mach, double thrust);

 protected:
   double designWeight{};
   double designAlt{};
   double wingSpan{};
   double wingArea{};

   double staticThrust{};
   double staticTSFC{};
   double thrustAngle{};
   double dTdM{};
   double dTdRho{};
   double dTSFCdM{};

   double qdes{};

   double cdo{};
   double clo{};
   double a{};
   double b{};

   bool usingMachEffects{true};
};
}
}

#endif