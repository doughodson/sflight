
#ifndef __sflight_mdls_InterpAero_H__
#define __sflight_mdls_InterpAero_H__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_InterpAero.hpp"
#include "sflight/xml/Node.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: InterpAero
// Description: Sets up a simple aero lookup table system
//------------------------------------------------------------------------------
class InterpAero : public Module
{
 public:
   InterpAero(Player*, const double frameRate);

   // module interface
   virtual void update(const double timestep) override;

   void createCoefs(const double pitch, const double u, const double vz, const double thrust,
                    double& alpha, double& cl, double& cd);

   double getBetaMach(double mach) const;

   friend void xml_bindings::init_InterpAero(xml::Node*, InterpAero*);

 private:
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
