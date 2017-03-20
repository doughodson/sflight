
#include "WaypointFollower.hpp"
#include <iostream>
#include "FDMGlobals.hpp"
#include "Earth.hpp"
#include "UnitConvert.hpp"

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#else
#include <math.h>
#endif

#include "xml/NodeUtil.hpp"
#include "UnitConvert.hpp"

using std :: string;
using std::cout;
using std::endl;

namespace SimpleFlight {
    WaypointFollower :: WaypointFollower(FDMGlobals *globals, double frameRate) : FDMModule(globals, frameRate) {
        currentWp = 0;
        wpNum = 0;
    }
    
    WaypointFollower :: ~WaypointFollower() {}
    
    
    void WaypointFollower :: initialize(Node* node) {
        Node* tmp = node->getChild("WaypointFollower");
        
        isOn = NodeUtil :: getBool(tmp, "WaypointFollow", true);
        
        cmdPathType = (NodeUtil::get(tmp, "PathType", "DIRECT") == "BEARING") ? PATHTYPE_BEARING : PATHTYPE_DIRECT;
        
        vector <Node*> wps = NodeUtil :: getList(tmp->getChild("WaypointList"), "Waypoint");
        
        for( unsigned int i=0; i<wps.size(); i++) {
            Node* wp = wps[i];
            addWaypoint( UnitConvert::toRads( NodeUtil::getDouble(wp, "Lat", 0) ),
            UnitConvert::toRads( NodeUtil::getDouble(wp, "Lon", 0) ),
            UnitConvert::toMeters( NodeUtil::getDouble(wp, "Alt", 0) ),
            UnitConvert::toMPS( NodeUtil::getDouble(wp, "Speed", 0) ),
            UnitConvert::toRads( NodeUtil::getDouble(wp, "Heading", 0) ));
        }
        
        if (isOn)  {
            wpNum = 0;
            currentWp = 0;
            
            altTol = 10.;
            distTol = UnitConvert::NMtoEarthRadians(0.1);
            azTol = UnitConvert::toRads(1);
            
	    globals->autoPilotCmds.setAltHoldOn(true);
	    globals->autoPilotCmds.setHdgHoldOn(true);
	    globals->autoPilotCmds.setAutoPilotOn(true);
	    globals->autoPilotCmds.setAutoThrottleOn(true);
            
            
            loadWaypoint();
        }
        
    }
    
    void WaypointFollower :: setState(bool isOn) {
        if (isOn) {
            globals->autoPilotCmds.setAltHoldOn(isOn);
            globals->autoPilotCmds.setHdgHoldOn(isOn);
            globals->autoPilotCmds.setAutoPilotOn(isOn);
            globals->autoPilotCmds.setAutoThrottleOn(isOn);
        }
        this->isOn = isOn;
    }
    
    void WaypointFollower :: update( double timestep) {

        if (!isOn)
            return;
        
        if (currentWp == 0) {
            loadWaypoint();
            return;
        }
        
        double az = Earth :: headingBetween( globals->lat, globals->lon, currentWp->radLat, currentWp->radLon );
        double dist = Earth :: distance(globals->lat, globals->lon, currentWp->radLat, currentWp->radLon );
        
        //double hdgDiff = fabs( UnitConvert :: wrapHeading(globals->eulers.getPsi() - az, true) );
        double hdg = atan2( globals->nedVel.get2(), globals->nedVel.get1() ); 
        double hdgDiff = fabs( UnitConvert :: wrapHeading(hdg - az, true) );
        
        bool isClose = dist < distTol;
        bool isBehind =  fabs(hdgDiff) > M_PI / 2.;
        
        if ( isClose && isBehind) {
            loadWaypoint();
            return;
        }
        
        if (cmdPathType == PATHTYPE_DIRECT) {
	    globals->autoPilotCmds.setCmdHeading(az);
        }
        else {
            if ( hdgDiff >= M_PI ) {
                globals->autoPilotCmds.setCmdHeading( az );
            }
            else {
                globals->autoPilotCmds.setCmdHeading( UnitConvert::wrapHeading( 2 * az - currentWp->radHeading - 0.01, true) );
            }
        }
        
    }
    
    
    void WaypointFollower :: loadWaypoint() {
        currentWp = 0;
        
        cout << "loading wpt: " << wpNum << endl;
        
        if (waypoints.size() > wpNum) {
            setState(true);
            currentWp = &waypoints[wpNum];
	    globals->autoPilotCmds.setCmdAltitude(currentWp->meterAlt );
	    globals->autoPilotCmds.setCmdSpeed( currentWp->mpsSpeed );
            
            // set the distance tolerance to x seconds of flight time
            distTol = (currentWp->mpsSpeed * 5) * Earth::metersToRadian;
            std::cerr << distTol << std::endl;
            
            wpNum++;
        }
        else {
            setState(false);
        }
    }


    void WaypointFollower :: addWaypoint(double radLat, double radLon, double meterAlt, double mpsSpeed, double radHeading ) {
            Waypoint wp;
            wp.radLat = radLat;
            wp.radLon = radLon;
            wp.meterAlt = meterAlt;
            wp.mpsSpeed = mpsSpeed;
            wp.radHeading = radHeading;
            waypoints.push_back( wp);
            setState(true);
    }

    void WaypointFollower :: clearAllWaypoints() {
	  waypoints.clear(); 
          wpNum = 0;
          currentWp = 0;
    }
 
    int WaypointFollower :: getCurrentWp() {
          return wpNum;
    }

    int WaypointFollower :: getNumWaypoints() {
          return waypoints.size();
    }

    void WaypointFollower :: setCurrentWp(int num) {
          this->wpNum = num;
    }
}//namespace SimpleFlight
