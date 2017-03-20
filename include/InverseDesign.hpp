
#ifndef __InverseDesign_H__
#define __InverseDesign_H__

#include "FDMModule.hpp"

namespace sf
{
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: InverseDesign
// Description: Sets up a simple aero lookup table system
//------------------------------------------------------------------------------
class InverseDesign : public virtual FDMModule
{

 public:
   InverseDesign(FDMGlobals *globals, double frameRate);

   virtual void initialize(Node *node) override;
   virtual void update(double timestep) override;

   void getAeroCoefs(double pitch, double u, double vz, double rho, double weight, double thrust, double &alpha, double &cl, double &cd);

   double getThrust(double rho, double mach, double throttle);

   double getFuelFlow(double rho, double mach, double thrust);

 protected:
   double designWeight {};
   double designAlt {};
   double wingSpan {};
   double wingArea {};

   double staticThrust {};
   double staticTSFC {};
   double thrustAngle {};
   double dTdM {};
   double dTdRho {};
   double dTSFCdM {};

   double qdes {};

   double cdo {};
   double clo {};
   double a {};
   double b {};

   bool usingMachEffects {true};
};
}

#endif
