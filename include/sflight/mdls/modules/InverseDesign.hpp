
#ifndef __sflight_mdls_InverseDesign_H__
#define __sflight_mdls_InverseDesign_H__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_InverseDesign.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: InverseDesign
// Description: Sets up a simple aero lookup table system
//------------------------------------------------------------------------------
class InverseDesign : public Module
{
 public:
   InverseDesign(Player*, const double frameRate);

   // module interface
   virtual void update(const double timestep) override;

   void getAeroCoefs(const double pitch, const double u, const double vz, const double rho,
                     const double weight, const double thrust, double& alpha, double& cl, double& cd);

   double getThrust(double rho, double mach, double throttle);

   double getFuelFlow(double rho, double mach, double thrust);

   friend void xml_bindings::init_InverseDesign(xml::Node*, InverseDesign*);

 private:
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
