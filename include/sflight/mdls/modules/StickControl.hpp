
#ifndef __sflight_mdls_StickControl_H__
#define __sflight_mdls_StickControl_H__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/mdls/Vector3.hpp"

#include "sflight/xml_bindings/init_StickControl.hpp"
#include "sflight/xml/Node.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
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
   virtual void update(const double timestep) override;

   friend void xml_bindings::init_StickControl(xml::Node*, StickControl*);

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
