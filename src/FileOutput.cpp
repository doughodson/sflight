
#include "FileOutput.hpp"
#include "FDMGlobals.hpp"
#include <iostream>
#include "UnitConvert.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"
#include <iomanip>

using namespace std;

namespace sf
{


FileOutput :: FileOutput(FDMGlobals *globals, double frameRate) : FDMModule( globals, frameRate)
{
	lastTime = 0;
}


FileOutput :: ~FileOutput()
{
    fout.close();
}

void FileOutput :: initialize(Node* node) {
	string filename = NodeUtil::get(node, "FileOutput/Path", "");
	rate = NodeUtil::getDouble(node, "FileOutput/Rate", 1);
	cout << "Saving output to: " << filename << endl;
	fout.open( filename.c_str() );
    frameCounter = 0;
	if (fout.is_open() ) {
		    fout << "Time (sec)\tLatitude(deg)\tLongitude(deg)\tAltitude(ft)\tSpeed(ktas)\tBank(deg)\tPitch(deg)\tHeading(deg)\tThrottle" << std::endl;
	}
}


void FileOutput :: update( double timestep ) {
	if ( globals->simTime - lastTime > 1./rate) {
		update();
		lastTime = globals->simTime;
	}
}

void FileOutput :: update()
{
    frameCounter++;

    frameCounter = 0;
    fout << globals->simTime << "\t";

	fout << setprecision(7);

    fout << UnitConvert :: toDegs( globals->lat ) << "\t";
    fout << UnitConvert :: toDegs( globals->lon ) << "\t";
    fout << UnitConvert :: toFeet( globals->alt ) << "\t";

    fout << UnitConvert :: toKnots( globals->vInf ) << "\t";

    fout << UnitConvert :: toDegs( globals->eulers.getPhi() ) << "\t";
    fout << UnitConvert :: toDegs( globals->eulers.getTheta() ) << "\t";
    fout << UnitConvert :: toDegs( globals->eulers.getPsi() ) << "\t";

    fout << globals->throttle << "\t";
    fout << endl;
}

void FileOutput :: close() {

        fout.close();
}
} // namespace SimpleFlight

