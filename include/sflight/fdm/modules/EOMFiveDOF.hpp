
#ifndef __sflight_fdm_EOMFiveDOF_H__
#define __sflight_fdm_EOMFiveDOF_H__

#include "sflight/fdm/modules/Module.hpp"

#include "sflight/fdm/Quaternion.hpp"
#include "sflight/fdm/Vector3.hpp"

namespace sflight {
namespace xml { class Node; }
namespace fdm {
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
   virtual void initialize(xml::Node*) override;
   virtual void update(const double timestep) override;

   void computeEOM(double timestep);

private:
   Quaternion quat;
   Quaternion qdot;
   Vector3 forces;
   Vector3 uvw;
   Vector3 pqr;
   Vector3 xyz;
   Vector3 gravAccel;
   double gravConst {};
   bool autoRudder {};
};

}
}

#endif
