
#include "sflight/xml_bindings/load_modules.hpp"

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

#include <iostream>
#include <string>
#include <vector>

namespace sflight {
namespace xml_bindings {

void load_modules(xml::Node* parent, mdls::Player* player)
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
         eomFiveDOF->initialize(node);
      } else if (className == "InterpAero") {
         auto interpAero = new mdls::InterpAero(player, rate);
         interpAero->initialize(node);
      } else if (className == "TableAero") {
         auto tableAero = new mdls::TableAero(player, rate);
         tableAero->initialize(node);
      } else if (className == "Autopilot") {
         auto autoPilot = new mdls::AutoPilot(player, rate);
         autoPilot->initialize(node);
      } else if (className == "Engine") {
         auto engine = new mdls::Engine(player, rate);
         engine->initialize(node);
      } else if (className == "Atmosphere") {
         auto atmosphere = new mdls::Atmosphere(player, rate);
         atmosphere->initialize(node);
      } else if (className == "WaypointFollower") {
         auto waypointFollower = new mdls::WaypointFollower(player, rate);
         waypointFollower->initialize(node);
      } else if (className == "StickControl") {
         auto stickControl = new mdls::StickControl(player, rate);
         stickControl->initialize(node);
      } else if (className == "FileOutput") {
         auto fileOutput = new mdls::FileOutput(player, rate);
         fileOutput->initialize(node);
      } else if (className == "InverseDesign") {
         auto inverseDesign = new mdls::InverseDesign(player, rate);
         inverseDesign->initialize(node);
      }
   }
}
}
}
