
#ifndef __sflight_mdls_EOMFiveDOF_H__
#define __sflight_mdls_EOMFiveDOF_H__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/mdls/Quaternion.hpp"
#include "sflight/mdls/Vector3.hpp"

#include "sflight/xml_bindings/init_EOMFiveDOF.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: EOMFiveDOF
// Description: Implements psuedo five DOF dynamics for fixed wing aircraft
//              (imposes a no-slip condition)
//------------------------------------------------------------------------------
class EOMFiveDOF : public Module
{
 public:
   EOMFiveDOF(Player*, const double frameRate);
   virtual ~EOMFiveDOF() = default;

   // module interface
   virtual void update(const double timestep) override;

   void computeEOM(const double timestep);

   friend void xml_bindings::init_EOMFiveDOF(xml::Node*, EOMFiveDOF*);

 private:
   Quaternion quat;
   Quaternion qdot;
   Vector3 forces;
   Vector3 uvw;
   Vector3 pqr;
   Vector3 xyz;
   Vector3 gravAccel;
   double gravConst{};
   bool autoRudder{};
};
}
}

#endif
