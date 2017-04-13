
#include "sflight/fdm/load_modules.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/fdm/modules/Atmosphere.hpp"
#include "sflight/fdm/modules/AutoPilot.hpp"
#include "sflight/fdm/modules/EOMFiveDOF.hpp"
#include "sflight/fdm/modules/Engine.hpp"
#include "sflight/fdm/modules/FileOutput.hpp"
#include "sflight/fdm/modules/InterpAero.hpp"
#include "sflight/fdm/modules/InverseDesign.hpp"
#include "sflight/fdm/modules/StickControl.hpp"
#include "sflight/fdm/modules/TableAero.hpp"
#include "sflight/fdm/modules/WaypointFollower.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace sflight {
namespace fdm {

void load_modules(xml::Node* parent, FDMGlobals* globals)
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
