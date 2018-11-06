
#include "sflight/xml_bindings/init_FileOutput.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/FileOutput.hpp"

#include <iostream>
#include <fstream>

namespace sflight {
namespace xml_bindings {

void init_FileOutput(xml::Node* node, mdls::FileOutput* fileOutput)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Module: FileOutput"        << std::endl;
   std::cout << "-------------------------" << std::endl;

   std::string filename{xml::getString(node, "FileOutput/Path", "")};
   std::cout << "Filename : " << filename << std::endl;
   fileOutput->fout.open(filename.c_str());
   if (fileOutput->fout.is_open()) {
      fileOutput->fout << "Time "
                          "(sec)\tLatitude(deg)\tLongitude(deg)\tAltitude(ft)\tSpeed(ktas)"
                          "\tBank(deg)\tPitch(deg)\tHeading(deg)\tThrottle"
                       << std::endl;
   }

   const double rate{xml::getDouble(node, "FileOutput/Rate", 1.0)};
   std::cout << "Rate     : " << rate << std::endl;
   fileOutput->rate = static_cast<int>(rate);

   fileOutput->frameCounter = 0;

   std::cout << "-------------------------" << std::endl;
}
}
}
