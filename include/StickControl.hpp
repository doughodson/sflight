//
// File:   StickControl.h
// Author: matt
//
// Created on June 27, 2006, 6:34 PM
//

#ifndef _StickControl_H
#define	_StickControl_H


#include "FDMModule.hpp"
#include "Vector3.hpp"
#include "xml/Node.hpp"

namespace SimpleFlight {
    
    
    class StickControl : public virtual FDMModule {
        
        public:
        
        StickControl( FDMGlobals *globals, double frameRate);
        ~StickControl();
        
        //overrides of FDMModule functions
        void initialize(Node* node);
        void update(double timestep);
        
        private:
        
        Vector3 maxRates;
        Vector3 maxDef;
        
        double designQbar;
        double elevGain;
        double rudGain;
        double ailGain;
        double pitchGain;
        
        
    };
    
}



#endif	/* _StickControl_H */

