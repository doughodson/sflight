
#ifndef _SimTimer_H
#define _SimTimer_H

namespace sf {
    
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
    
}

#endif


