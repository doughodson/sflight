
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>

#include "FDMGlobals.hpp"
#include "EOMFiveDOF.hpp"
#include "InterpAero.hpp"
#include "SimpleAutoPilot.hpp"
#include "WaypointFollower.hpp"
#include "SimpleEngine.hpp"
#include "Atmosphere.hpp"
#include <time.h>
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"
#include "xml/XMLParser.hpp"
#include "SimTimer.hpp"
#include "ModuleLoader.hpp"
#include "Table2D.hpp"
#include "Table3D.hpp"
#include "TableAero.hpp"
#include "InverseDesign.hpp"

using namespace sf;

using std :: cout;
using std :: cerr;
using std :: endl;

int main(int argc, char** argv) {

	if (argc < 4) {
		std::cout << "usage: SimpleFlight <input file> <total time (sec)> <frame rate (frames/sec)>" << std::endl;
		return 1;
	} 

    
	FDMGlobals *globals = new FDMGlobals();

	XMLParser* parser = new XMLParser();
	Node* node = parser->parse(argv[1], true);

	ModuleLoader::loadModules(node, globals);
	globals->initialize(node);

	SimTimer *timer = new SimTimer(globals, atof( argv[3]), atof(argv[3]) * atof( argv[2]) );

	std::cout << "Running SimpleFlight for " << argv[2] << " seconds.\n" << std::endl;

	timer->startConstructive();

	std::cout << "Done.\n" << std::endl;
    
	return 0;
    
}
