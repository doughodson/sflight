

#ifndef __WaypointFollower_H__
#define __WaypointFollower_H__

#include "FDMModule.hpp"

#include <vector>

namespace sf
{

//------------------------------------------------------------------------------
// Class: Waypoint
//------------------------------------------------------------------------------
class Waypoint
{
 public:
   double radLat {};
   double radLon {};
   double meterAlt {};
   double mpsSpeed {};
   double radHeading {};
};

//------------------------------------------------------------------------------
// Class: WaypointFollower
//------------------------------------------------------------------------------
class WaypointFollower : public FDMModule
{
 public:
   WaypointFollower(FDMGlobals *globals, double frameRate);
   ~WaypointFollower();

   // module interface
   void initialize(Node *node);
   void update(double timestep);

   void loadWaypoint();
   void setState(bool isOn);
   void setCurrentWp(int num);
   int getCurrentWp();
   int getNumWaypoints();
   void clearAllWaypoints();
   void addWaypoint(double radLat, double radLon, double meterAlt, double mpsSpeed, double radBearing);

 protected:
   std::vector<Waypoint> waypoints;
   Waypoint* currentWp {};
   int wpNum {};

   double altTol {};
   double distTol {};
   double azTol {};

   bool isOn {};

   enum class PathType { DIRECT, BEARING };

   PathType cmdPathType;
};
}

#endif