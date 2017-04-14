
#include "sflight/xml/Node.hpp"
#include "sflight/xml/parser_utils.hpp"

#include "SimExec.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/xml_bindings/load_modules.hpp"

#include <cstdlib>
#include <iostream>

using namespace sflight;

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

   auto player = new mdls::Player();

   // parse input file and return top node
   xml::Node* node = xml::parse(filename, true);

   // construct obj tree and have each obj read their own configuration
   xml_bindings::load_modules(node, player);
//   player->initialize(node);

   auto exec = new SimExec(player, frame_rate, num_frames);

   std::cout << "Running SimpleFlight for " << total_time << " seconds.\n" << std::endl;

   exec->startConstructive();

   std::cout << "Done\n" << std::endl;

   return 0;
}
