
#include "sf/fdm/modules/WaypointFollower.hpp"

#include "sf/xml/Node.hpp"
#include "sf/xml/node_utils.hpp"

#include "sf/fdm/Earth.hpp"
#include "sf/fdm/FDMGlobals.hpp"
#include "sf/fdm/UnitConvert.hpp"
#include "sf/fdm/UnitConvert.hpp"
#include "sf/fdm/constants.hpp"

#include <cmath>
#include <iostream>

namespace sf {
namespace fdm {

WaypointFollower::WaypointFollower(FDMGlobals* globals, double frameRate)
    : FDMModule(globals, frameRate)
{
}

void WaypointFollower::initialize(xml::Node* node)
{
   xml::Node* tmp = node->getChild("WaypointFollower");

   isOn = getBool(tmp, "WaypointFollow", true);

   cmdPathType = (get(tmp, "PathType", "DIRECT") == "BEARING")
                     ? PathType::BEARING
                     : PathType::DIRECT;

   std::vector<xml::Node*> wps =
       xml::getList(tmp->getChild("WaypointList"), "Waypoint");

   for (unsigned int i = 0; i < wps.size(); i++) {
      xml::Node* wp = wps[i];
      addWaypoint(UnitConvert::toRads(getDouble(wp, "Lat", 0)),
                  UnitConvert::toRads(getDouble(wp, "Lon", 0)),
                  UnitConvert::toMeters(getDouble(wp, "Alt", 0)),
                  UnitConvert::toMPS(getDouble(wp, "Speed", 0)),
                  UnitConvert::toRads(getDouble(wp, "Heading", 0)));
   }

   if (isOn) {
      wpNum = 0;
      currentWp = nullptr;

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

void WaypointFollower::setState(bool isOn)
{
   if (isOn) {
      globals->autoPilotCmds.setAltHoldOn(isOn);
      globals->autoPilotCmds.setHdgHoldOn(isOn);
      globals->autoPilotCmds.setAutoPilotOn(isOn);
      globals->autoPilotCmds.setAutoThrottleOn(isOn);
   }
   this->isOn = isOn;
}

void WaypointFollower::update(const double timestep)
{
   if (!isOn)
      return;

   if (currentWp == nullptr) {
      loadWaypoint();
      return;
   }

   double az = Earth::headingBetween(globals->lat, globals->lon,
                                     currentWp->radLat, currentWp->radLon);
   double dist = Earth::distance(globals->lat, globals->lon, currentWp->radLat,
                                 currentWp->radLon);

   // double hdgDiff = fabs( UnitConvert :: wrapHeading(globals->eulers.getPsi()
   // - az, true) );
   double hdg = std::atan2(globals->nedVel.get2(), globals->nedVel.get1());
   double hdgDiff = std::fabs(UnitConvert::wrapHeading(hdg - az, true));

   bool isClose = dist < distTol;
   bool isBehind = std::fabs(hdgDiff) > PI / 2.;

   if (isClose && isBehind) {
      loadWaypoint();
      return;
   }

   if (cmdPathType == PathType::DIRECT) {
      globals->autoPilotCmds.setCmdHeading(az);
   } else {
      if (hdgDiff >= PI) {
         globals->autoPilotCmds.setCmdHeading(az);
      } else {
         globals->autoPilotCmds.setCmdHeading(UnitConvert::wrapHeading(
             2 * az - currentWp->radHeading - 0.01, true));
      }
   }
}

void WaypointFollower::loadWaypoint()
{
   currentWp = nullptr;

   std::cout << "loading wpt: " << wpNum << std::endl;

   if (waypoints.size() > wpNum) {
      setState(true);
      currentWp = &waypoints[wpNum];
      globals->autoPilotCmds.setCmdAltitude(currentWp->meterAlt);
      globals->autoPilotCmds.setCmdSpeed(currentWp->mpsSpeed);

      // set the distance tolerance to x seconds of flight time
      distTol = (currentWp->mpsSpeed * 5) * Earth::metersToRadian;
      std::cerr << distTol << std::endl;

      wpNum++;
   } else {
      setState(false);
   }
}

void WaypointFollower::addWaypoint(const double radLat, const double radLon,
                                   const double meterAlt, const double mpsSpeed,
                                   const double radHeading)
{
   Waypoint wp;
   wp.radLat = radLat;
   wp.radLon = radLon;
   wp.meterAlt = meterAlt;
   wp.mpsSpeed = mpsSpeed;
   wp.radHeading = radHeading;
   waypoints.push_back(wp);
   setState(true);
}

void WaypointFollower::clearAllWaypoints()
{
   waypoints.clear();
   wpNum = 0;
   currentWp = nullptr;
}

int WaypointFollower::getCurrentWp() { return wpNum; }

int WaypointFollower::getNumWaypoints() { return waypoints.size(); }

void WaypointFollower::setCurrentWp(int num) { wpNum = num; }
}
}
