/** sets up a simple aero lookup table system */

#ifndef INVERSEDESIGN_H
#define INVERSEDESIGN_H

#include "FDMModule.hpp"
namespace sf {
    class FDMGlobals;
    
    
    class InverseDesign : public virtual FDMModule{
        
        public:
            
            InverseDesign( FDMGlobals *globals, double frameRate);
            
            // functions from FDMModule
            void initialize(Node* node);
            void update( double timestep );
            
            void getAeroCoefs( double pitch, double u, double vz, double rho, double weight, double thrust, double& alpha, double& cl, double& cd );
            
            double getThrust(double rho, double mach, double throttle);

            double getFuelFlow(double rho, double mach, double thrust);
            
            protected:
                
                double designWeight;
                double designAlt;
                double wingSpan;
                double wingArea;
                
                double staticThrust;
                double staticTSFC;
                double thrustAngle;
                double dTdM;
                double dTdRho;
                double dTSFCdM;
                
                double qdes;         
                
                double cdo;
                double clo;
                double a;
                double b;

                bool usingMachEffects;
                
    };
    
}

#endif
