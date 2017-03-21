
#ifndef __Engine_H__
#define __Engine_H__

#include "FDMModule.hpp"

namespace sf
{

//------------------------------------------------------------------------------
// Class: SimpleEngine
// Description: Implements a simple engine
//------------------------------------------------------------------------------
class SimpleEngine : public virtual FDMModule
{
 public:
   SimpleEngine(FDMGlobals* globals, double timestep);
   ~SimpleEngine();

   virtual void initialize(Node *node) override;
   virtual void update(double timestep) override;

 protected:
   double thrustRatio {};
   double designWeight {};
   double thrustAngle {};

   double designRho {};
   double designTemp {};
   double designPress {};
   double designMach {};
   double designThrust {};

   double seaLevelTemp {};
   double seaLevelPress {};

   double staticThrust {};
   double thrustSlope {};
   double staticFF {};
   double FFslope {};

   double thrust {};
};

}

#endif
