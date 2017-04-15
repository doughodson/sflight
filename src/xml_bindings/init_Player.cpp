
#include "sflight/xml_bindings/init_Player.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Euler.hpp"
#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"
#include "sflight/mdls/modules/Atmosphere.hpp"

#include <cmath>

namespace sflight {
namespace xml_bindings {

void init_Player(xml::Node* node, mdls::Player* player)
{
   player->mass =
       (mdls::UnitConvert::toKilos(xml::getDouble(node, "InitialConditions/Weight", 0.0)));

   xml::Node* wind = node->getChild("Wind");
   if (wind != nullptr) {
      const double wspeed = mdls::UnitConvert::toMPS(xml::getDouble(wind, "Speed", 0.0));
      const double dir =
          mdls::UnitConvert::toRads(xml::getDouble(wind, "Direction", 0.0) + 180);
      player->windVel.set1(wspeed * std::cos(dir));
      player->windVel.set2(wspeed * std::sin(dir));
      player->windVel.set3(0);
   }

   // general
   xml::Node* tmp = node->getChild("InitialConditions/Position");
   player->lat = mdls::UnitConvert::toRads(xml::getDouble(tmp, "Latitude", 0.0));
   player->lon = mdls::UnitConvert::toRads(xml::getDouble(tmp, "Longitude", 0.0));
   player->alt = mdls::UnitConvert::toMeters(xml::getDouble(tmp, "Altitude", 0.0));

   tmp = node->getChild("InitialConditions/Orientation");
   player->eulers = mdls::Euler(mdls::UnitConvert::toRads(xml::getDouble(tmp, "Heading", 0.0)),
                                mdls::UnitConvert::toRads(xml::getDouble(tmp, "Pitch", 0.0)),
                                mdls::UnitConvert::toRads(xml::getDouble(tmp, "Roll", 0.0)));

   const double speed =
       mdls::UnitConvert::toMPS(xml::getDouble(node, "InitialConditions/Airspeed", 0.0));
   const double mach = xml::getDouble(node, "InitialConditions/Mach", 0.0);
   if (mach != 0 && speed == 0)
      player->uvw.set1(
          mdls::Atmosphere::getSpeedSound(mdls::Atmosphere::getTemp(player->alt)) *
          player->mach);
   else
      player->uvw.set1(speed);

   player->eulers.getDxDyDz(player->nedVel, player->uvw);

   player->autoPilotCmds.setCmdHeading(player->eulers.getPsi());
   player->autoPilotCmds.setCmdAltitude(player->alt);
   player->autoPilotCmds.setCmdSpeed(player->uvw.get1());
   player->autoPilotCmds.setCmdVertSpeed(0);
   player->autoPilotCmds.setUseMach(false);
   player->autoPilotCmds.setAltHoldOn(true);
   player->autoPilotCmds.setAutoThrottleOn(true);
   player->autoPilotCmds.setHdgHoldOn(true);

   player->fuel =
       mdls::UnitConvert::toKilos(xml::getDouble(node, "InitialConditions/Fuel", 0.0));
}
}
}
