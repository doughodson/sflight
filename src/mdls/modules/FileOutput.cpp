
#include "sflight/mdls/modules/FileOutput.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

#include <iomanip>
#include <iostream>

namespace sflight {
namespace mdls {

FileOutput::FileOutput(Player* player, const double frameRate)
    : Module(player, frameRate)
{
}

FileOutput::~FileOutput() { fout.close(); }

void FileOutput::initialize(xml::Node* node)
{
   std::string filename = get(node, "FileOutput/Path", "");
   rate = getDouble(node, "FileOutput/Rate", 1);
   std::cout << "Saving output to: " << filename << std::endl;
   fout.open(filename.c_str());
   frameCounter = 0;
   if (fout.is_open()) {
      fout << "Time "
              "(sec)\tLatitude(deg)\tLongitude(deg)\tAltitude(ft)\tSpeed(ktas)"
              "\tBank(deg)\tPitch(deg)\tHeading(deg)\tThrottle"
           << std::endl;
   }
}

void FileOutput::update(const double timestep)
{
   if (player->simTime - lastTime > 1. / rate) {
      update();
      lastTime = player->simTime;
   }
}

void FileOutput::update()
{
   frameCounter++;

   frameCounter = 0;
   fout << player->simTime << "\t";

   fout << std::setprecision(7);

   fout << UnitConvert::toDegs(player->lat) << "\t";
   fout << UnitConvert::toDegs(player->lon) << "\t";
   fout << UnitConvert::toFeet(player->alt) << "\t";

   fout << UnitConvert::toKnots(player->vInf) << "\t";

   fout << UnitConvert::toDegs(player->eulers.getPhi()) << "\t";
   fout << UnitConvert::toDegs(player->eulers.getTheta()) << "\t";
   fout << UnitConvert::toDegs(player->eulers.getPsi()) << "\t";

   fout << player->throttle << "\t";
   fout << std::endl;
}

void FileOutput::close() { fout.close(); }
}
}
