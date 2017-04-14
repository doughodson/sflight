
#include "sflight/mdls/modules/WaypointFollower.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"
#include "sflight/mdls/nav_utils.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

WaypointFollower::WaypointFollower(Player* player, const double frameRate)
    : Module(player, frameRate)
{
}

void WaypointFollower::initialize(xml::Node* node)
{
   xml::Node* tmp = node->getChild("WaypointFollower");

   isOn = getBool(tmp, "WaypointFollow", true);

   cmdPathType =
       (get(tmp, "PathType", "DIRECT") == "BEARING") ? PathType::BEARING : PathType::DIRECT;

   std::vector<xml::Node*> wps = xml::getList(tmp->getChild("WaypointList"), "Waypoint");

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

      player->autoPilotCmds.setAltHoldOn(true);
      player->autoPilotCmds.setHdgHoldOn(true);
      player->autoPilotCmds.setAutoPilotOn(true);
      player->autoPilotCmds.setAutoThrottleOn(true);

      loadWaypoint();
   }
}

void WaypointFollower::setState(bool isOn)
{
   if (isOn) {
      player->autoPilotCmds.setAltHoldOn(isOn);
      player->autoPilotCmds.setHdgHoldOn(isOn);
      player->autoPilotCmds.setAutoPilotOn(isOn);
      player->autoPilotCmds.setAutoThrottleOn(isOn);
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

   const double az =
       nav::headingBetween(player->lat, player->lon, currentWp->radLat, currentWp->radLon);
   const double dist =
       nav::distance(player->lat, player->lon, currentWp->radLat, currentWp->radLon);

   // double hdgDiff = fabs( UnitConvert :: wrapHeading(player->eulers.getPsi()
   // - az, true) );
   const double hdg = std::atan2(player->nedVel.get2(), player->nedVel.get1());
   const double hdgDiff = std::fabs(UnitConvert::wrapHeading(hdg - az, true));

   const bool isClose = dist < distTol;
   const bool isBehind = std::fabs(hdgDiff) > math::PI / 2.0;

   if (isClose && isBehind) {
      loadWaypoint();
      return;
   }

   if (cmdPathType == PathType::DIRECT) {
      player->autoPilotCmds.setCmdHeading(az);
   } else {
      if (hdgDiff >= math::PI) {
         player->autoPilotCmds.setCmdHeading(az);
      } else {
         player->autoPilotCmds.setCmdHeading(
             UnitConvert::wrapHeading(2 * az - currentWp->radHeading - 0.01, true));
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
      player->autoPilotCmds.setCmdAltitude(currentWp->meterAlt);
      player->autoPilotCmds.setCmdSpeed(currentWp->mpsSpeed);

      // set the distance tolerance to x seconds of flight time
      distTol = (currentWp->mpsSpeed * 5) * nav::metersToRadian;
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
