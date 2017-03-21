
#ifndef _StickControl_H
#define _StickControl_H

#include "FDMModule.hpp"
#include "Vector3.hpp"
#include "xml/Node.hpp"

namespace sf
{

//------------------------------------------------------------------------------
// Class: StickControl
//------------------------------------------------------------------------------
class StickControl : public virtual FDMModule
{
 public:
   StickControl(FDMGlobals *globals, const double frameRate);
   ~StickControl();

   virtual void initialize(Node *node) override;
   virtual void update(double timestep) override;

 private:
   Vector3 maxRates;
   Vector3 maxDef;

   double designQbar {};
   double elevGain {};
   double rudGain {};
   double ailGain {};
   double pitchGain {};
};

}

#endif
