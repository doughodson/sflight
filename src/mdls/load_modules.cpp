
#include "sflight/mdls/load_modules.hpp"

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
namespace mdls {

void load_modules(xml::Node* parent, Player* player)
{
   xml::Node* node = parent->getChild("Modules");
   std::vector<xml::Node*> nodeList = xml::getList(node, "Module");
   const double defaultRate = xml::getDouble(node, "Rate", 0.0);

   for (int i = 0; i < nodeList.size(); i++) {
      const std::string className = xml::get(nodeList[i], "Class", "");
      const double rate = xml::getDouble(nodeList[i], "Rate", 0.0);

      if (className == "EOMFiveDOF") {
         auto eomFiveDOF = new EOMFiveDOF(player, rate);
         eomFiveDOF->initialize(node);
      } else if (className == "InterpAero") {
         auto interpAero = new InterpAero(player, rate);
         interpAero->initialize(node);
      } else if (className == "TableAero") {
         auto tableAero = new TableAero(player, rate);
         tableAero->initialize(node);
      } else if (className == "Autopilot") {
         auto autoPilot = new AutoPilot(player, rate);
         autoPilot->initialize(node);
      } else if (className == "Engine") {
         auto engine = new Engine(player, rate);
         engine->initialize(node);
      } else if (className == "Atmosphere") {
         auto atmosphere = new Atmosphere(player, rate);
         atmosphere->initialize(node);
      } else if (className == "WaypointFollower") {
         auto waypointFollower = new WaypointFollower(player, rate);
         waypointFollower->initialize(node);
      } else if (className == "StickControl") {
         auto stickControl = new StickControl(player, rate);
         stickControl->initialize(node);
      } else if (className == "FileOutput") {
         auto fileOutput = new FileOutput(player, rate);
         fileOutput->initialize(node);
      } else if (className == "InverseDesign") {
         auto inverseDesign = new InverseDesign(player, rate);
         inverseDesign->initialize(node);
      }
   }
}
}
}
