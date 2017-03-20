
#include "ModuleLoader.hpp"
#include "FDMGlobals.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"

#include "EOMFiveDOF.hpp"
#include "InterpAero.hpp"
#include "SimpleAutoPilot.hpp"
#include "SimpleEngine.hpp"
#include "Atmosphere.hpp"
#include "StickControl.hpp"
#include "WaypointFollower.hpp"
#include "TableAero.hpp"
#include "InverseDesign.hpp"
#include "FileOutput.hpp"

#include <string>
#include <vector>
#include <iostream>

namespace sf {

	void ModuleLoader :: loadModules(Node* parent, FDMGlobals *globals) {

		Node *node = parent->getChild("Modules");
		vector <Node*> nodeList = NodeUtil ::  getList(node, "Module" );
		double defaultRate = NodeUtil :: getDouble( node, "Rate", 0);

		for(int i=0; i< nodeList.size(); i++ ) {
			string className = NodeUtil :: get( nodeList[i], "Class", "");
			double rate = NodeUtil :: getDouble(nodeList[i], "Rate", 0);

			if ( className == "EOMFiveDOF" ) {
				new EOMFiveDOF(globals, rate);
			}
			else if ( className == "InterpAero" ) {
				new InterpAero(globals, rate);
			}
			else if ( className == "TableAero" ) {
				new TableAero(globals, rate);
			}
			else if ( className == "SimpleAutopilot" ) {
				new SimpleAutoPilot(globals, rate);
			}
		        else if ( className == "SimpleEngine" ) {
				new SimpleEngine(globals, rate);
			}
			else if ( className == "Atmosphere" ) {
				new Atmosphere(globals, rate);
			}
			else if ( className == "WaypointFollower" ) {
				new WaypointFollower(globals, rate);
			}
			else if ( className == "StickControl" ) {
				new StickControl(globals, rate);
			}
			else if ( className == "FileOutput" ) {
				new FileOutput(globals, rate);
			}
			else if ( className == "InverseDesign" ) {
				new InverseDesign(globals, rate);
			}
			
		}
	}

}
