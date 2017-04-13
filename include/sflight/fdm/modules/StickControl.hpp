
#ifndef __sflight_fdm_StickControl_H__
#define __sflight_fdm_StickControl_H__

#include "sflight/fdm/modules/Module.hpp"

#include "sflight/fdm/Vector3.hpp"

#include "sflight/xml/Node.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {
class Player;

//------------------------------------------------------------------------------
// Class: StickControl
//------------------------------------------------------------------------------
class StickControl : public Module
{
 public:
   StickControl(Player*, const double frameRate);
   virtual ~StickControl() = default;

   // module interface
   virtual void initialize(xml::Node*) override;
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
