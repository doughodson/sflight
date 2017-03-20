/** base class for all flight model modules */

#include "FDMModule.hpp"
#include "FDMGlobals.hpp"
#include <string>
using namespace std;
namespace SimpleFlight
{


FDMModule :: FDMModule( FDMGlobals *globals, double frameRate )
{
    this->globals = globals;
    
	if(globals != 0)
		globals->addModule(this);

    if (frameRate <= 0 ) {
        this->frameTime = 0;
    }
    else {
        this->frameTime = 1./ frameRate;
    }
}

FDMModule :: ~FDMModule()
{}

void FDMModule :: update(double timestep)
{}

void FDMModule :: initialize(Node* node) 
{}

void FDMModule :: setProperty(string tag, double val) {
}
	
} //namespace SimpleFlight
