
#include "SimTimer.hpp"
#include "FDMGlobals.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"
#include <time.h>
#include <iostream>


#ifdef _WIN32
#include "windows.h"
#else
#include <unistd.h>
#endif

namespace SimpleFlight {
    
    
    SimTimer :: SimTimer(FDMGlobals *globals, double frameRate) {
        this->globals = globals;
        this->frameRate = frameRate;
        this->maxFrames = (long) 1E16;
    }
    
    SimTimer :: SimTimer(FDMGlobals *globals, double frameRate, long maxFrames) {
        this->globals = globals;
        this->frameRate = frameRate;
        this->maxFrames = maxFrames;
    }
    
    SimTimer :: ~SimTimer() {}
    
    void SimTimer :: start() {
        
        if (globals == 0 || frameRate == 0) return;
        
        double time = 0;
        double frameTime = 1./frameRate;
        long sleepTime = (long) ( frameTime * 1E3);
        
        while( globals->frameNum < maxFrames )  {
            if ( !globals->paused) {
                globals->update(frameTime);
            }
            #ifdef _WIN32
            Sleep(sleepTime);
            #else
            usleep(sleepTime);
            #endif
        }
        
        
    }

	void SimTimer :: startConstructive() {
        
        if (globals == 0 || frameRate == 0) return;
        
        double time = 0;
        double frameTime = 1./frameRate;
		globals->paused = false;
		double frameGroup = 0;
        
        while( globals->frameNum < maxFrames )  {
			
		    if (frameGroup >= 100) {
				std::cout << "updating frame " << globals->frameNum << " of " << maxFrames << std::endl;
				frameGroup = 0;
			}
			frameGroup ++;
            if ( !globals->paused) {
                globals->update(frameTime);
             }
        }
        
        
    }
    
    void SimTimer :: stop() {}
    
    void SimTimer :: initialize(Node* node) {
        
        this->frameRate = NodeUtil :: getDouble( node, "Modules/Rate", 20 );
        
    }
    
    
} //namespace
