
#include "sflight/mdls/modules/FileOutput.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

#include <iomanip>
#include <iostream>

namespace sflight {
namespace mdls {

FileOutput::FileOutput(Player* player, const double frameRate) : Module(player, frameRate) {}

FileOutput::~FileOutput() { fout.close(); }

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

   if (!fout.is_open())
      return;

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
