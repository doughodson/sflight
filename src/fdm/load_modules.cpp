
#include "sflt/fdm/load_modules.hpp"

#include "sflt/xml/Node.hpp"
#include "sflt/xml/node_utils.hpp"

#include "sflt/fdm/modules/Atmosphere.hpp"
#include "sflt/fdm/modules/EOMFiveDOF.hpp"
#include "sflt/fdm/modules/FileOutput.hpp"
#include "sflt/fdm/modules/InterpAero.hpp"
#include "sflt/fdm/modules/InverseDesign.hpp"
#include "sflt/fdm/modules/SimpleAutoPilot.hpp"
#include "sflt/fdm/modules/SimpleEngine.hpp"
#include "sflt/fdm/modules/StickControl.hpp"
#include "sflt/fdm/modules/TableAero.hpp"
#include "sflt/fdm/modules/WaypointFollower.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace sflt {
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
