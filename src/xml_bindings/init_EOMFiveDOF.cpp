
#include "sflight/xml_bindings/init_EOMFiveDOF.hpp"

#include "sflight/mdls/modules/EOMFiveDOF.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/nav_utils.hpp"

#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_EOMFiveDOF(xml::Node* node, mdls::EOMFiveDOF* eom)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Module: EOMFiveDOF"        << std::endl;
   std::cout << "-------------------------" << std::endl;

   eom->quat = mdls::Quaternion(eom->player->eulers);
   eom->qdot = mdls::Quaternion();
   eom->forces = mdls::Vector3();

   eom->uvw = mdls::Vector3();
   eom->pqr = mdls::Vector3();
   eom->xyz = mdls::Vector3();
   eom->gravAccel = mdls::Vector3();

   eom->gravConst = mdls::nav::getG(0, 0, 0);
   const bool auto_rudder{xml::getBool(node, "Control/AutoRudder", true)};
   std::cout << "Auto rudder : " << auto_rudder << std::endl;
   eom->autoRudder = auto_rudder;

   std::cout << "-------------------------" << std::endl;
}
}
}
