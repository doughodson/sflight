/** sets up a simple aero lookup table system */

#ifndef INTERPAEROCOEF_H
#define INTERPAEROCOEF_H

#include "FDMModule.hpp"
namespace SimpleFlight {
    class FDMGlobals;
    
    
    class InterpAero : public virtual FDMModule{
        
        public:
            
            InterpAero( FDMGlobals *globals, double frameRate);
            
            // functions from FDMModule
            void initialize(Node* node);
            void update( double timestep );
            
            void createCoefs( double pitch, double u, double vz, double thrust, double& alpha, double& cl, double& cd );
            
            double getBetaMach(double mach); 
            
            protected:
                
                double designWeight;
                double designAlt;
                double wingSpan;
                double wingArea;
                double thrustRatio;
                double thrustAngle;
                
                double qdes;
                
                double cruiseAlpha;
                double cruiseCL;
                double cruiseCD;
                
                double climbAlpha;
                double climbCL;
                double climbCD;
                
                double a1;
                double a2;
                double b1;
                double b2;
                
                double cdo;
                double clo;
                double liftSlope;
                double stallCL;
                double wingEffects;
                
                bool usingMachEffects;
                
                
    };
    
}//namespace SimpleFlight
#endif
