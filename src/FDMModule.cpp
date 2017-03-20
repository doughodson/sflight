
#include "FDMModule.hpp"
#include "FDMGlobals.hpp"
#include <string>

namespace sf {

FDMModule::FDMModule(FDMGlobals *globals, double frameRate)
{
    this->globals = globals;
    
	if (globals != nullptr)
		globals->addModule(this);

    if (frameRate <= 0 ) {
        this->frameTime = 0;
    }
    else {
        this->frameTime = 1./ frameRate;
    }
}

FDMModule::~FDMModule()
{}

void FDMModule::update(double timestep)
{}

void FDMModule::initialize(Node* node) 
{}

void FDMModule::setProperty(std::string tag, double val) {
}
	
}
