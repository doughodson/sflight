
#include "ModuleLoader.hpp"
#include "FDMGlobals.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"

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

using namespace std;

namespace sf
{

void ModuleLoader::loadModules(Node* parent, FDMGlobals* globals)
{
   Node* node = parent->getChild("Modules");
   vector<Node*> nodeList = NodeUtil::getList(node, "Module");
   const double defaultRate = NodeUtil::getDouble(node, "Rate", 0);

   for (int i = 0; i < nodeList.size(); i++)
   {
      string className = NodeUtil::get(nodeList[i], "Class", "");
      const double rate = NodeUtil::getDouble(nodeList[i], "Rate", 0);

      if (className == "EOMFiveDOF")
      {
         new EOMFiveDOF(globals, rate);
      }
      else if (className == "InterpAero")
      {
         new InterpAero(globals, rate);
      }
      else if (className == "TableAero")
      {
         new TableAero(globals, rate);
      }
      else if (className == "SimpleAutopilot")
      {
         new SimpleAutoPilot(globals, rate);
      }
      else if (className == "SimpleEngine")
      {
         new SimpleEngine(globals, rate);
      }
      else if (className == "Atmosphere")
      {
         new Atmosphere(globals, rate);
      }
      else if (className == "WaypointFollower")
      {
         new WaypointFollower(globals, rate);
      }
      else if (className == "StickControl")
      {
         new StickControl(globals, rate);
      }
      else if (className == "FileOutput")
      {
         new FileOutput(globals, rate);
      }
      else if (className == "InverseDesign")
      {
         new InverseDesign(globals, rate);
      }
   }
}
}
