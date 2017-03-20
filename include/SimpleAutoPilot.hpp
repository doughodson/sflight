
#ifndef AUTOPILOT_H
#define AUTOPILOT_H

#include "FDMModule.hpp"
#include "xml/Node.hpp"

#define TURNTYPE_HDG 0
#define TURNTYPE_TRAJECTORY 1

namespace sf {
    
    class SimpleAutoPilot : public FDMModule {
        
        public:
            
            SimpleAutoPilot( FDMGlobals *globals, double frameRate);
            ~SimpleAutoPilot();
            
            //overrides of FDMModule functions
            void initialize(Node* node);
            void update(double timestep);
            
            
            void updateHdg( double timestep, double cmdHdg);
            void updateAlt( double timestep);
            void updateVS( double timestep, double cmdVs);
            void updateSpeed(double timestep);
            
            protected:
                
                double kphi, maxBankRate;
                double kalt;
                double kpitch;
                double maxG;
                double minG;
                double maxG_rate;
                double minG_rate;
                
                double maxThrottle;
                double minThrottle;
                double spoolTime;
                
                double lastVz;
                
                int turnType;
                
                double hdgErrTol;
                
                bool vsHoldOn, altHoldOn, hdgHoldOn;
                
                
    };

}

#endif
