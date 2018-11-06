
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
#include "sflight/xml_bindings/init_Player.hpp"
#include "sflight/xml_bindings/init_StickControl.hpp"
#include "sflight/xml_bindings/init_TableAero.hpp"
#include "sflight/xml_bindings/init_WaypointFollower.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace sflight {
namespace xml_bindings {

void builder(xml::Node* parent, mdls::Player* player)
{
   init_Player(parent, player);

   xml::Node* node{parent->getChild("Modules")};
   std::vector<xml::Node*> nodeList{xml::getList(node, "Module")};
   const double defaultRate{xml::getDouble(node, "Rate", 0.0)};

   for (std::size_t i = 0; i < nodeList.size(); i++) {

      const std::string className{xml::get(nodeList[i], "Class", "")};
      const double rate{xml::getDouble(nodeList[i], "Rate", 0.0)};

      if (className == "EOMFiveDOF") {
         auto eomFiveDOF{new mdls::EOMFiveDOF(player, rate)};
         player->addModule(eomFiveDOF);
         init_EOMFiveDOF(node, eomFiveDOF);
      } else if (className == "InterpAero") {
         auto interpAero{new mdls::InterpAero(player, rate)};
         player->addModule(interpAero);
         init_InterpAero(node, interpAero);
      } else if (className == "TableAero") {
         auto tableAero{new mdls::TableAero(player, rate)};
         player->addModule(tableAero);
         init_TableAero(node, tableAero);
      } else if (className == "Autopilot") {
         auto autoPilot{new mdls::AutoPilot(player, rate)};
         player->addModule(autoPilot);
         init_AutoPilot(node, autoPilot);
      } else if (className == "Engine") {
         auto engine{new mdls::Engine(player, rate)};
         player->addModule(engine);
         init_Engine(node, engine);
      } else if (className == "Atmosphere") {
         auto atmosphere{new mdls::Atmosphere(player, rate)};
         player->addModule(atmosphere);
      } else if (className == "WaypointFollower") {
         auto waypointFollower{new mdls::WaypointFollower(player, rate)};
         player->addModule(waypointFollower);
         init_WaypointFollower(node, waypointFollower);
      } else if (className == "StickControl") {
         auto stickControl{new mdls::StickControl(player, rate)};
         player->addModule(stickControl);
         init_StickControl(node, stickControl);
      } else if (className == "FileOutput") {
         auto fileOutput{new mdls::FileOutput(player, rate)};
         player->addModule(fileOutput);
         init_FileOutput(node, fileOutput);
      } else if (className == "InverseDesign") {
         auto inverseDesign{new mdls::InverseDesign(player, rate)};
         player->addModule(inverseDesign);
         init_InverseDesign(node, inverseDesign);
      }
   }
}
}
}
