

#ifndef WAYPOINTFOLLOWER_H
#define WAYPOINTFOLLOWER_H

#include "FDMModule.hpp"
#include <vector>


namespace SimpleFlight {
    
    class Waypoint {
        
        public:
            double radLat;
            double radLon;
            double meterAlt;
            double mpsSpeed;
            double radHeading;
    };
    
    class WaypointFollower : public FDMModule {
        
        public:
            
            WaypointFollower(FDMGlobals *globals, double frameRate);
            ~WaypointFollower();
            
            void update(double timestep);
            void initialize(Node* node);
            
            void loadWaypoint();
            void setState(bool isOn);
            void setCurrentWp(int num);
            int getCurrentWp();
            int getNumWaypoints();
            void clearAllWaypoints();
            void addWaypoint(double radLat, double radLon, double meterAlt, double mpsSpeed, double radBearing);
            
            
            protected:
                
                std::vector <Waypoint> waypoints;
                Waypoint *currentWp;
                int wpNum;
                
                double altTol;
                double distTol;
                double azTol;
                
                bool isOn;
                
                enum pathType {
                    PATHTYPE_DIRECT,
                    PATHTYPE_BEARING,
                };
                
                pathType cmdPathType;
                
    };
    
}//namespace SimpleFlight

#endif
