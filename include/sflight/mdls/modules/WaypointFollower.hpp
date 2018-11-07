

#ifndef __sflight_mdls_WaypointFollower_HPP__
#define __sflight_mdls_WaypointFollower_HPP__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_WaypointFollower.hpp"

#include <vector>

namespace sflight {
namespace xml { class Node; }
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: Waypoint
//------------------------------------------------------------------------------
class Waypoint
{
 public:
   double radLat{};
   double radLon{};
   double meterAlt{};
   double mpsSpeed{};
   double radHeading{};
};

//------------------------------------------------------------------------------
// Class: WaypointFollower
//------------------------------------------------------------------------------
class WaypointFollower : public Module
{
 public:
   WaypointFollower(Player*, const double frameRate);
   virtual ~WaypointFollower() = default;

   // module interface
   void update(const double timestep) override;

   void loadWaypoint();
   void setState(const bool isOn);
   void setCurrentWp(const int num);
   std::size_t getCurrentWp();
   std::size_t getNumWaypoints();
   void clearAllWaypoints();
   void addWaypoint(const double radLat, const double radLon, const double meterAlt,
                    const double mpsSpeed, const double radBearing);

   friend void xml_bindings::init_WaypointFollower(xml::Node*, WaypointFollower*);

 private:
   std::vector<Waypoint> waypoints;
   Waypoint* currentWp{};
   std::size_t wpNum{};

   double altTol{};
   double distTol{};
   double azTol{};

   bool isOn{};

   enum class PathType { DIRECT, BEARING };

   PathType cmdPathType;
};
}
}

#endif
