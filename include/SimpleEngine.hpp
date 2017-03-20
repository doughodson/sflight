/** implements a simple engine */

#ifndef ENGINE_H
#define ENGINE_H

#include "FDMModule.hpp"

namespace sf {
    
    class SimpleEngine : public virtual FDMModule {
        
        public:
            
            SimpleEngine(FDMGlobals *globals, double timestep);
            ~SimpleEngine();
            
            // overrides
            void initialize(Node* node);
            void update(double timestep);
            
            protected:
                
                double thrustRatio;
                double designWeight;
                double thrustAngle;
                
                double designRho;
                double designTemp;
                double designPress;
                double designMach;
                double designThrust;
                
                double seaLevelTemp;
                double seaLevelPress;
                
                double staticThrust;
                double thrustSlope;
                double staticFF;
                double FFslope;
                
                double thrust;
                
    };

}

#endif
