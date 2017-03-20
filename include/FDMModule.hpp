/** The base class for all FDM modules */

#ifndef FDMMODULE_H
#define FDMMODULE_H

#include <iostream>

using std :: string;


namespace SimpleFlight {
    
    class FDMGlobals;
    class Node;
    
    class FDMModule {
        
        public:
            
            FDMModule();
            FDMModule( FDMGlobals *globals, double frameRate );
            virtual ~FDMModule();
            
            virtual void update(double timestep);
            virtual void initialize(Node* node);
            
            virtual void setProperty(string tag, double val);
            
            FDMGlobals* globals;
            
            double frameTime;
            double lastTime;
            
            
    };
    
};//namespace SimpleFlight
#endif
