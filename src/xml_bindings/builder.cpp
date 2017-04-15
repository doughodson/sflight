
#include "sflight/xml_bindings/builder.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/Atmosphere.hpp"
#include "sflight/mdls/modules/AutoPilot.hpp"
#include "sflight/mdls/modules/EOMFiveDOF.hpp"
#include "sflight/mdls/modules/Engine.hpp"
#include "sflight/mdls/modules/FileOutput.hpp"
#include "sflight/mdls/modules/InterpAero.hpp"
#include "sflight/mdls/modules/InverseDesign.hpp"
#include "sflight/mdls/modules/StickControl.hpp"
#include "sflight/mdls/modules/TableAero.hpp"
#include "sflight/mdls/modules/WaypointFollower.hpp"

#include "sflight/xml_bindings/init_AutoPilot.hpp"
#include "sflight/xml_bindings/init_Engine.hpp"
#include "sflight/xml_bindings/init_EOMFiveDOF.hpp"
#include "sflight/xml_bindings/init_FileOutput.hpp"
#include "sflight/xml_bindings/init_InterpAero.hpp"
#include "sflight/xml_bindings/init_InverseDesign.hpp"
#include "sflight/xml_bindings/init_StickControl.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace sflight {
namespace xml_bindings {

void builder(xml::Node* parent, mdls::Player* player)
{
   player->initialize(parent);

   xml::Node* node = parent->getChild("Modules");
   std::vector<xml::Node*> nodeList = xml::getList(node, "Module");
   const double defaultRate = xml::getDouble(node, "Rate", 0.0);

   for (int i = 0; i < nodeList.size(); i++) {
      const std::string className = xml::get(nodeList[i], "Class", "");
      const double rate = xml::getDouble(nodeList[i], "Rate", 0.0);

      if (className == "EOMFiveDOF") {
         auto eomFiveDOF = new mdls::EOMFiveDOF(player, rate);
         init_EOMFiveDOF(node, eomFiveDOF);
      } else if (className == "InterpAero") {
         auto interpAero = new mdls::InterpAero(player, rate);
         init_InterpAero(node, interpAero);
      } else if (className == "TableAero") {
         auto tableAero = new mdls::TableAero(player, rate);
         tableAero->initialize(node);
      } else if (className == "Autopilot") {
         auto autoPilot = new mdls::AutoPilot(player, rate);
         init_AutoPilot(node, autoPilot);
      } else if (className == "Engine") {
         auto engine = new mdls::Engine(player, rate);
         init_Engine(node, engine);
      } else if (className == "Atmosphere") {
         auto atmosphere = new mdls::Atmosphere(player, rate);
         //atmosphere->initialize(node);
      } else if (className == "WaypointFollower") {
         auto waypointFollower = new mdls::WaypointFollower(player, rate);
         waypointFollower->initialize(node);
      } else if (className == "StickControl") {
         auto stickControl = new mdls::StickControl(player, rate);
         init_StickControl(node, stickControl);
      } else if (className == "FileOutput") {
         auto fileOutput = new mdls::FileOutput(player, rate);
         init_FileOutput(node, fileOutput);
      } else if (className == "InverseDesign") {
         auto inverseDesign = new mdls::InverseDesign(player, rate);
         init_InverseDesign(node, inverseDesign);
      }
   }
}
}
}
