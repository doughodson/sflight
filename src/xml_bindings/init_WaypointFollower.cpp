
#include "sflight/xml_bindings/init_WaypointFollower.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/WaypointFollower.hpp"

#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"

#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_WaypointFollower(xml::Node* node, mdls::WaypointFollower* wp)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Module: WaypointFollower"  << std::endl;
   std::cout << "-------------------------" << std::endl;

   xml::Node* tmp{node->getChild("WaypointFollower")};

   wp->isOn = xml::getBool(tmp, "WaypointFollow", true);

   wp->cmdPathType = (xml::getString(tmp, "PathType", "DIRECT") == "BEARING")
                         ? mdls::WaypointFollower::PathType::BEARING
                         : mdls::WaypointFollower::PathType::DIRECT;

   std::cout << "[init_WaypointFollower] begin\n";

   std::vector<xml::Node*> wps{xml::getList(tmp->getChild("WaypointList"), "Waypoint")};

   std::cout << "[init_WaypointFollower] end\n";

   for (std::size_t i = 0; i < wps.size(); i++) {
      xml::Node* wpNode{wps[i]};
      wp->addWaypoint(mdls::UnitConvert::toRads(xml::getDouble(wpNode, "Lat", 0.0)),
                      mdls::UnitConvert::toRads(xml::getDouble(wpNode, "Lon", 0.0)),
                      mdls::UnitConvert::toMeters(xml::getDouble(wpNode, "Alt", 0.0)),
                      mdls::UnitConvert::toMPS(xml::getDouble(wpNode, "Speed", 0.0)),
                      mdls::UnitConvert::toRads(xml::getDouble(wpNode, "Heading", 0.0)));
   }

   if (wp->isOn) {
      wp->wpNum = 0;
      wp->currentWp = nullptr;

      wp->altTol = 10.0;
      wp->distTol = mdls::UnitConvert::NMtoEarthRadians(0.1);
      wp->azTol = mdls::UnitConvert::toRads(1);

      wp->player->autoPilotCmds.setAltHoldOn(true);
      wp->player->autoPilotCmds.setHdgHoldOn(true);
      wp->player->autoPilotCmds.setAutoPilotOn(true);
      wp->player->autoPilotCmds.setAutoThrottleOn(true);

      wp->loadWaypoint();
   }

   std::cout << "-------------------------" << std::endl;
}
}
}
