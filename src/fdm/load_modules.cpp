
#include "sf/fdm/load_modules.hpp"

#include "sf/xml/Node.hpp"
#include "sf/xml/node_utils.hpp"

#include "sf/fdm/modules/Atmosphere.hpp"
#include "sf/fdm/modules/EOMFiveDOF.hpp"
#include "sf/fdm/modules/FileOutput.hpp"
#include "sf/fdm/modules/InterpAero.hpp"
#include "sf/fdm/modules/InverseDesign.hpp"
#include "sf/fdm/modules/SimpleAutoPilot.hpp"
#include "sf/fdm/modules/SimpleEngine.hpp"
#include "sf/fdm/modules/StickControl.hpp"
#include "sf/fdm/modules/TableAero.hpp"
#include "sf/fdm/modules/WaypointFollower.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace sf {
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
      }
      else if (className == "InterpAero") {
         new InterpAero(globals, rate);
      }
      else if (className == "TableAero") {
         new TableAero(globals, rate);
      }
      else if (className == "SimpleAutopilot") {
         new SimpleAutoPilot(globals, rate);
      }
      else if (className == "SimpleEngine") {
         new SimpleEngine(globals, rate);
      }
      else if (className == "Atmosphere") {
         new Atmosphere(globals, rate);
      }
      else if (className == "WaypointFollower") {
         new WaypointFollower(globals, rate);
      }
      else if (className == "StickControl") {
         new StickControl(globals, rate);
      }
      else if (className == "FileOutput") {
         new FileOutput(globals, rate);
      }
      else if (className == "InverseDesign") {
         new InverseDesign(globals, rate);
      }
   }
}
}
}
