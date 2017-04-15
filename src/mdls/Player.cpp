
#include "sflight/mdls/Player.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"
#include "sflight/mdls/modules/Module.hpp"

#include "sflight/mdls/AutoPilotCmds.hpp"
#include "sflight/mdls/Euler.hpp"
#include "sflight/mdls/Quaternion.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/Vector3.hpp"

#include "sflight/mdls/nav_utils.hpp"
#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include <cmath>
#include <string>

namespace sflight {
namespace mdls {

Player::Player() { g = nav::getG(0, 0, 0); }

Player::~Player()
{
   for (int i = 0; i < modules.size(); i++) {
      delete modules[i];
   }
}

void Player::addModule(Module* module) { this->modules.push_back(module); }

void Player::initialize(xml::Node* node)
{
   mass = (UnitConvert::toKilos(xml::getDouble(node, "InitialConditions/Weight", 0.0)));

   xml::Node* wind = node->getChild("Wind");
   if (wind != nullptr) {
      const double wspeed = UnitConvert::toMPS(xml::getDouble(wind, "Speed", 0.0));
      const double dir = UnitConvert::toRads(xml::getDouble(wind, "Direction", 0.0) + 180);
      windVel.set1(wspeed * std::cos(dir));
      windVel.set2(wspeed * std::sin(dir));
      windVel.set3(0);
   }

   // general
   xml::Node* tmp = node->getChild("InitialConditions/Position");
   lat = UnitConvert::toRads(xml::getDouble(tmp, "Latitude", 0.0));
   lon = UnitConvert::toRads(xml::getDouble(tmp, "Longitude", 0.0));
   alt = UnitConvert::toMeters(xml::getDouble(tmp, "Altitude", 0.0));

   tmp = node->getChild("InitialConditions/Orientation");
   eulers = Euler(UnitConvert::toRads(xml::getDouble(tmp, "Heading", 0.0)),
                  UnitConvert::toRads(xml::getDouble(tmp, "Pitch", 0.0)),
                  UnitConvert::toRads(xml::getDouble(tmp, "Roll", 0.0)));

   const double speed =
       UnitConvert::toMPS(xml::getDouble(node, "InitialConditions/Airspeed", 0.0));
   const double mach = xml::getDouble(node, "InitialConditions/Mach", 0);
   if (mach != 0 && speed == 0)
      uvw.set1(Atmosphere::getSpeedSound(Atmosphere::getTemp(alt)) * mach);
   else
      uvw.set1(speed);

   eulers.getDxDyDz(nedVel, uvw);

   autoPilotCmds.setCmdHeading(eulers.getPsi());
   autoPilotCmds.setCmdAltitude(alt);
   autoPilotCmds.setCmdSpeed(uvw.get1());
   autoPilotCmds.setCmdVertSpeed(0);
   autoPilotCmds.setUseMach(false);
   autoPilotCmds.setAltHoldOn(true);
   autoPilotCmds.setAutoThrottleOn(true);
   autoPilotCmds.setHdgHoldOn(true);

   fuel = UnitConvert::toKilos(xml::getDouble(node, "InitialConditions/Fuel", 0.0));
}

void Player::update(double timestep)
{
   simTime += timestep;

   for (unsigned int i = 0; i < modules.size(); i++) {
      timestep = simTime - modules[i]->lastTime;
      if (timestep >= modules[i]->frameTime) {
         modules[i]->lastTime = simTime;
         modules[i]->update(timestep);
      }
   }
   frameNum++;
}
}
}
