
#include "sflight/xml_bindings/init_Player.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Euler.hpp"
#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"
#include "sflight/mdls/modules/Atmosphere.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_Player(xml::Node* node, mdls::Player* player)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Player: InitialConditions" << std::endl;
   std::cout << "-------------------------" << std::endl;

   player->mass = mdls::UnitConvert::toKilos(xml::getDouble(node, "InitialConditions/Weight", 0.0));
   std::cout << "Player mass      : " << player->mass << " Kilograms\n";

   xml::Node* wind{node->getChild("Wind")};
   if (wind != nullptr) {
      const double wspeed{mdls::UnitConvert::toMPS(xml::getDouble(wind, "Speed", 0.0))};
      const double dir{mdls::UnitConvert::toRads(xml::getDouble(wind, "Direction", 0.0) + 180)};
      std::cout << "Player wind speed     : " << wspeed << "MPS\n";
      std::cout << "Player wind direction : " << dir << "radians\n";
      player->windVel.set1(wspeed * std::cos(dir));
      player->windVel.set2(wspeed * std::sin(dir));
      player->windVel.set3(0);
   }

   xml::Node* tmp{node->getChild("InitialConditions/Position")};
   player->lat = mdls::UnitConvert::toRads(xml::getDouble(tmp, "Latitude", 0.0));
   player->lon = mdls::UnitConvert::toRads(xml::getDouble(tmp, "Longitude", 0.0));
   player->alt = mdls::UnitConvert::toMeters(xml::getDouble(tmp, "Altitude", 0.0));
   std::cout << "Player latitude  : " << player->lat << " degrees\n";
   std::cout << "Player longitude : " << player->lon << " degrees\n";
   std::cout << "Player altitude  : " << player->alt << " feet\n";

   tmp = node->getChild("InitialConditions/Orientation");
   const double heading{mdls::UnitConvert::toRads(xml::getDouble(tmp, "Heading", 0.0))};
   const double pitch{mdls::UnitConvert::toRads(xml::getDouble(tmp, "Pitch", 0.0))};
   const double roll{mdls::UnitConvert::toRads(xml::getDouble(tmp, "Roll", 0.0))};
   std::cout << "Player heading   : " << heading << " degrees\n";
   std::cout << "Player pitch     : " << pitch   << " degrees\n";
   std::cout << "Player roll      : " << roll  << " degrees\n";
   player->eulers = mdls::Euler(heading, pitch, roll);

   const double speed{mdls::UnitConvert::toMPS(xml::getDouble(node, "InitialConditions/Airspeed", 0.0))};
   const double mach{xml::getDouble(node, "InitialConditions/Mach", 0.0)};
   std::cout << "Player speed     : " << speed << " MPS\n";
   std::cout << "Player mach      : " << mach  << std::endl;
   if (mach != 0 && speed == 0) {
      player->uvw.set1(mdls::Atmosphere::getSpeedSound(mdls::Atmosphere::getTemp(player->alt)) * player->mach);
   } else {
      player->uvw.set1(speed);
   }

   player->eulers.getDxDyDz(player->nedVel, player->uvw);

   player->autoPilotCmds.setCmdHeading(player->eulers.getPsi());
   player->autoPilotCmds.setCmdAltitude(player->alt);
   player->autoPilotCmds.setCmdSpeed(player->uvw.get1());
   player->autoPilotCmds.setCmdVertSpeed(0);
   player->autoPilotCmds.setUseMach(false);
   player->autoPilotCmds.setAltHoldOn(true);
   player->autoPilotCmds.setAutoThrottleOn(true);
   player->autoPilotCmds.setHdgHoldOn(true);

   const double fuel{mdls::UnitConvert::toKilos(xml::getDouble(node, "InitialConditions/Fuel", 0.0))};
   std::cout << "Player fuel      : " << fuel << std::endl;
   player->fuel = fuel;
   std::cout << "-------------------------" << std::endl;
}
}
}
