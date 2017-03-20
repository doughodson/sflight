/** implements psuedo five DOF dynamics for fixed wing aircraft
 *
 * imposes a no-slip condition
 *
 */

#ifndef EOMFIVEDOF_H
#define EOMFIVEDOF_H

#include "FDMModule.hpp"
#include "Quaternion.hpp"
#include "Vector3.hpp"

namespace sf {
    // forward references
    class FDMGlobals;
    
    
    class EOMFiveDOF : public FDMModule {
        
        public:
            EOMFiveDOF(FDMGlobals *globals, double frameRate);
            ~EOMFiveDOF();
            
            void computeEOM(double timestep);
            void initialize(Node* node);
            void update(double timestep);
            
            
            private:
                
                Quaternion quat;
                Quaternion qdot;
                Vector3 forces;
                Vector3 uvw;
                Vector3 pqr;
                Vector3 xyz;
                Vector3 gravAccel;
                double gravConst;
                bool autoRudder;
                
                
    };
    
}

#endif
