
#ifndef __sflt_fdm_StickControl_H__
#define __sflt_fdm_StickControl_H__

#include "sflt/fdm/modules/FDMModule.hpp"

#include "sflt/fdm/Vector3.hpp"

#include "sflt/xml/Node.hpp"

namespace sflt {
namespace xml {
class Node;
}
namespace fdm {

//------------------------------------------------------------------------------
// Class: StickControl
//------------------------------------------------------------------------------
class StickControl : public virtual FDMModule
{
 public:
   StickControl(FDMGlobals* globals, const double frameRate);
   virtual ~StickControl() = default;

   // module interface
   virtual void initialize(xml::Node* node) override;
   virtual void update(const double timestep) override;

 private:
   Vector3 maxRates;
   Vector3 maxDef;

   double designQbar{};
   double elevGain{};
   double rudGain{};
   double ailGain{};
   double pitchGain{};
};
}
}

#endif
