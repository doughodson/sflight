
#include "sflight/fdm/modules/FileOutput.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/fdm/FDMGlobals.hpp"
#include "sflight/fdm/UnitConvert.hpp"

#include <iomanip>
#include <iostream>

namespace sflight {
namespace fdm {

FileOutput::FileOutput(FDMGlobals* globals, double frameRate)
    : FDMModule(globals, frameRate)
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
   if (globals->simTime - lastTime > 1. / rate) {
      update();
      lastTime = globals->simTime;
   }
}

void FileOutput::update()
{
   frameCounter++;

   frameCounter = 0;
   fout << globals->simTime << "\t";

   fout << std::setprecision(7);

   fout << UnitConvert::toDegs(globals->lat) << "\t";
   fout << UnitConvert::toDegs(globals->lon) << "\t";
   fout << UnitConvert::toFeet(globals->alt) << "\t";

   fout << UnitConvert::toKnots(globals->vInf) << "\t";

   fout << UnitConvert::toDegs(globals->eulers.getPhi()) << "\t";
   fout << UnitConvert::toDegs(globals->eulers.getTheta()) << "\t";
   fout << UnitConvert::toDegs(globals->eulers.getPsi()) << "\t";

   fout << globals->throttle << "\t";
   fout << std::endl;
}

void FileOutput::close() { fout.close(); }
}
}
