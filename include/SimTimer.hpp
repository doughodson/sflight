//
// File:   SimTimer.h
// Author: default
//
// Created on October 31, 2006, 11:52 AM
//



#ifndef _SimTimer_H
#define	_SimTimer_H

namespace SimpleFlight {
    
    class FDMGlobals;
    class Node;
    
    class SimTimer {
        
        public:
        
        SimTimer(FDMGlobals *globals, double frameRate);
        SimTimer(FDMGlobals *globals, double frameRate, long maxFrames);
        ~SimTimer();
        
        void start();
		void startConstructive();
        void stop();
        void initialize(Node* node);
        
        private:
        
        FDMGlobals* globals;
        double frameRate;
        long maxFrames;
        
        
    };
    
}//namespace

#endif	/* _SimTimer_H */


