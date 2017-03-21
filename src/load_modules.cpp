
#include "load_modules.hpp"

#include "xml/Node.hpp"
#include "xml/node_utils.hpp"

#include "modules/Atmosphere.hpp"
#include "modules/EOMFiveDOF.hpp"
#include "modules/FileOutput.hpp"
#include "modules/InterpAero.hpp"
#include "modules/InverseDesign.hpp"
#include "modules/SimpleAutoPilot.hpp"
#include "modules/SimpleEngine.hpp"
#include "modules/StickControl.hpp"
#include "modules/TableAero.hpp"
#include "modules/WaypointFollower.hpp"

#include <string>
#include <vector>
#include <iostream>

namespace sf
{

void load_modules(Node* parent, FDMGlobals* globals)
{
   Node* node = parent->getChild("Modules");
   std::vector<Node*> nodeList = getList(node, "Module");
   const double defaultRate = getDouble(node, "Rate", 0);

   for (int i = 0; i < nodeList.size(); i++)
   {
      const std::string className = get(nodeList[i], "Class", "");
      const double rate = getDouble(nodeList[i], "Rate", 0);

      if (className == "EOMFiveDOF") {
         new EOMFiveDOF(globals, rate);
      } else if (className == "InterpAero") {
         new InterpAero(globals, rate);
      } else if (className == "TableAero") {
         new TableAero(globals, rate);
      } else if (className == "SimpleAutopilot") {
         new SimpleAutoPilot(globals, rate);
      } else if (className == "SimpleEngine") {
         new SimpleEngine(globals, rate);
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
