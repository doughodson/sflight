
#ifndef __Engine_H__
#define __Engine_H__

#include "fdm/modules/FDMModule.hpp"

namespace xml { class Node; }
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

   // module interface
   virtual void initialize(xml::Node *node) override;
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
