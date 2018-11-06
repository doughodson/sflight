
#include "sflight/xml/Node.hpp"
#include "sflight/xml/parser_utils.hpp"

#include "SimExec.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/xml_bindings/builder.hpp"

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
   const std::string filename{argv[1]};
   const double total_time{std::atof(argv[2])}; // sec
   const double frame_rate{std::atof(argv[3])}; // hz
   const long num_frames{static_cast<long>(total_time * frame_rate)};

   std::cout << "Filename      : " << filename   << std::endl;
   std::cout << "Total time    : " << total_time << std::endl;
   std::cout << "Frame rate    : " << frame_rate << std::endl;
   std::cout << "Num of frames : " << num_frames << std::endl;

   auto player{new mdls::Player()};

   // parse input file and return top node
   xml::Node* node{xml::parse(filename, true)};

   xml_bindings::builder(node, player);

   auto exec{new SimExec(player, frame_rate, num_frames)};

   std::cout << "Running SimpleFlight for " << total_time << " seconds." << std::endl;

   exec->startConstructive();

   std::cout << "Simulation finished" << std::endl;
   std::cout << std::flush;

   return 0;
}
