
#include "sflight/mdls/modules/FileOutput.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

#include <iostream>
#include <iomanip>

namespace sflight {
namespace mdls {

FileOutput::FileOutput(Player* player, const double frameRate) : Module(player, frameRate) {}

FileOutput::~FileOutput() { fout.close(); }

void FileOutput::update(const double timestep)
{
   if (player->simTime - lastTime > 1.0 / rate) {
      update();
      lastTime = player->simTime;
   }
}

void FileOutput::update()
{
   frameCounter++;

   frameCounter = 0;

   if (!fout.is_open()) {
      return;
   }

   fout << std::fixed;
   fout << std::setprecision(3);

   fout << std::setw(14) << player->simTime;

   fout << std::setprecision(4);

   fout << std::setw(14) << UnitConvert::toDegs(player->lat);
   fout << std::setw(14) << UnitConvert::toDegs(player->lon);
   fout << std::setw(14) << UnitConvert::toFeet(player->alt);

   fout << std::setw(14) << UnitConvert::toKnots(player->vInf);

   fout << std::setw(14) << UnitConvert::toDegs(player->eulers.getPhi());
   fout << std::setw(14) << UnitConvert::toDegs(player->eulers.getTheta());
   fout << std::setw(14) << UnitConvert::toDegs(player->eulers.getPsi());

   fout << std::setw(14) << player->throttle;
   fout << std::endl;
}
}
}
