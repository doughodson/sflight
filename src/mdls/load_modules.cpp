
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

#include <iostream>
#include <string>
#include <vector>

namespace sflight {
namespace mdls {

void load_modules(xml::Node* parent, Player* globals)
{
   xml::Node* node = parent->getChild("Modules");
   std::vector<xml::Node*> nodeList = xml::getList(node, "Module");
   const double defaultRate = xml::getDouble(node, "Rate", 0);

   for (int i = 0; i < nodeList.size(); i++) {
      const std::string className = xml::get(nodeList[i], "Class", "");
      const double rate = xml::getDouble(nodeList[i], "Rate", 0);

      if (className == "EOMFiveDOF") {
         new EOMFiveDOF(globals, rate);
      } else if (className == "InterpAero") {
         new InterpAero(globals, rate);
      } else if (className == "TableAero") {
         new TableAero(globals, rate);
      } else if (className == "Autopilot") {
         new AutoPilot(globals, rate);
      } else if (className == "Engine") {
         new Engine(globals, rate);
      } else if (className == "Atmosphere") {
         new Atmosphere(globals, rate);
      } else if (className == "WaypointFollower") {
         new WaypointFollower(globals, rate);
      } else if (className == "StickControl") {
         new StickControl(globals, rate);
      } else if (className == "FileOutput") {
         new FileOutput(globals, rate);
      } else if (className == "InverseDesign") {
         new InverseDesign(globals, rate);
      }
   }
}
}
}
