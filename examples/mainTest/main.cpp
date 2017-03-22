
#include "sf/xml/Node.hpp"
#include "sf/xml/parser_utils.hpp"

#include "sf/fdm/FDMGlobals.hpp"
#include "sf/fdm/SimTimer.hpp"
#include "sf/fdm/load_modules.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
   if (argc < 4) {
      std::cout << "usage: SimpleFlight <input file> <total time (sec)> <frame "
                   "rate (frames/sec)>"
                << std::endl;
      return 1;
   }

   auto globals = new sf::fdm::FDMGlobals();

   // parse input file and return top node
   sf::xml::Node* node = sf::xml::parse(argv[1], true);

   //
   load_modules(node, globals);
   globals->initialize(node);

   auto timer = new sf::fdm::SimTimer(globals, std::atof(argv[3]),
                                      std::atof(argv[3]) * std::atof(argv[2]));

   std::cout << "Running SimpleFlight for " << argv[2] << " seconds.\n"
             << std::endl;

   timer->startConstructive();

   std::cout << "Done\n" << std::endl;

   return 0;
}
