
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
   std::string filename = get(node, "FileOutput/Path", "");
   fileOutput->rate = static_cast<int>(xml::getDouble(node, "FileOutput/Rate", 1.0));
   std::cout << "Saving output to: " << filename << std::endl;
   fileOutput->fout.open(filename.c_str());
   fileOutput->frameCounter = 0;
   if (fileOutput->fout.is_open()) {
      fileOutput->fout << "Time "
                          "(sec)\tLatitude(deg)\tLongitude(deg)\tAltitude(ft)\tSpeed(ktas)"
                          "\tBank(deg)\tPitch(deg)\tHeading(deg)\tThrottle"
                       << std::endl;
   }
}
}
}
