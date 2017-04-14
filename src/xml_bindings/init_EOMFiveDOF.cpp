
#include "sflight/xml_bindings/init_EOMFiveDOF.hpp"

#include "sflight/mdls/modules/EOMFiveDOF.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/nav_utils.hpp"

namespace sflight {
namespace xml_bindings {

void init_EOMFiveDOF(xml::Node* node, mdls::EOMFiveDOF* eom)
{
   eom->quat = mdls::Quaternion(eom->player->eulers);
   eom->qdot = mdls::Quaternion();
   eom->forces = mdls::Vector3();

   eom->uvw = mdls::Vector3();
   eom->pqr = mdls::Vector3();
   eom->xyz = mdls::Vector3();
   eom->gravAccel = mdls::Vector3();

   eom->gravConst = mdls::nav::getG(0, 0, 0);
   eom->autoRudder = xml::getBool(node, "Control/AutoRudder", true);
}
}
}
