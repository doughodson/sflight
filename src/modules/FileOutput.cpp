

#include "modules/FileOutput.hpp"

#include "FDMGlobals.hpp"
#include "UnitConvert.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"

#include <iostream>
#include <iomanip>

namespace sf
{

FileOutput::FileOutput(FDMGlobals *globals, double frameRate) : FDMModule(globals, frameRate)
{}

FileOutput::~FileOutput()
{
    fout.close();
}

void FileOutput::initialize(Node* node)
{
   std::string filename = NodeUtil::get(node, "FileOutput/Path", "");
   rate = NodeUtil::getDouble(node, "FileOutput/Rate", 1);
   std::cout << "Saving output to: " << filename << std::endl;
   fout.open(filename.c_str());
   frameCounter = 0;
   if (fout.is_open() ) {
      fout << "Time (sec)\tLatitude(deg)\tLongitude(deg)\tAltitude(ft)\tSpeed(ktas)\tBank(deg)\tPitch(deg)\tHeading(deg)\tThrottle" << std::endl;
   }
}

void FileOutput::update(double timestep)
{
   if ( globals->simTime - lastTime > 1./rate) {
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

   fout << UnitConvert :: toDegs(globals->lat) << "\t";
   fout << UnitConvert :: toDegs(globals->lon) << "\t";
   fout << UnitConvert :: toFeet(globals->alt) << "\t";

   fout << UnitConvert :: toKnots(globals->vInf) << "\t";

   fout << UnitConvert :: toDegs(globals->eulers.getPhi()) << "\t";
   fout << UnitConvert :: toDegs(globals->eulers.getTheta()) << "\t";
   fout << UnitConvert :: toDegs(globals->eulers.getPsi()) << "\t";

   fout << globals->throttle << "\t";
   fout << std::endl;
}

void FileOutput::close()
{
   fout.close();
}

}

