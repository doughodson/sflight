
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
   const std::string filename = argv[1];
   const double total_time = std::atof(argv[2]); // sec
   const double frame_rate = std::atof(argv[3]); // hz
   const long num_frames = static_cast<long>(total_time * frame_rate);

   auto globals = new sf::fdm::FDMGlobals();

   // parse input file and return top node
   sf::xml::Node* node = sf::xml::parse(filename, true);

   // construct obj tree and have each obj read their own configuration
   load_modules(node, globals);
   globals->initialize(node);

   auto timer = new sf::fdm::SimTimer(globals, frame_rate, num_frames);

   std::cout << "Running SimpleFlight for " << argv[2] << " seconds.\n"
             << std::endl;

   timer->startConstructive();

   std::cout << "Done\n" << std::endl;

   return 0;
}
