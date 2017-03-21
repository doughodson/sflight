
#include "xml/Node.hpp"
#include "xml/XMLParser.hpp"

#include "FDMGlobals.hpp"
#include "SimTimer.hpp"
#include "load_modules.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 4) {
		std::cout << "usage: SimpleFlight <input file> <total time (sec)> <frame rate (frames/sec)>" << std::endl;
		return 1;
	}

	auto globals = new sf::FDMGlobals();

   // parse input file and return top node
	auto parser = new sf::XMLParser();
	sf::Node* node = parser->parse(argv[1], true);

   //
	load_modules(node, globals);
	globals->initialize(node);

	auto timer = new sf::SimTimer(globals, std::atof(argv[3]),
                                  std::atof(argv[3]) * std::atof(argv[2]));

	std::cout << "Running SimpleFlight for " << argv[2] << " seconds.\n" << std::endl;

	timer->startConstructive();

	std::cout << "Done\n" << std::endl;

	return 0;
}
