
#ifndef _StickControl_H
#define _StickControl_H

#include "fdm/modules/FDMModule.hpp"

#include "fdm/Vector3.hpp"

#include "xml/Node.hpp"

namespace xml { class Node; }
namespace sf
{

//------------------------------------------------------------------------------
// Class: StickControl
//------------------------------------------------------------------------------
class StickControl : public virtual FDMModule
{
 public:
   StickControl(FDMGlobals *globals, const double frameRate);
   virtual ~StickControl() = default;

   // module interface
   virtual void initialize(xml::Node *node) override;
   virtual void update(double timestep) override;
   virtual void setProperty(std::string tag, double val) override  {};

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
