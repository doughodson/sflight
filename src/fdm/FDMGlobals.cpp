
#include "sflt/fdm/FDMGlobals.hpp"

#include "sflt/fdm/modules/Atmosphere.hpp"
#include "sflt/fdm/modules/FDMModule.hpp"

#include "sflt/fdm/AutoPilotCmds.hpp"
#include "sflt/fdm/Euler.hpp"
#include "sflt/fdm/Quaternion.hpp"
#include "sflt/fdm/UnitConvert.hpp"
#include "sflt/fdm/Vector3.hpp"

#include "sflt/xml/Node.hpp"
#include "sflt/fdm/nav_utils.hpp"
#include "sflt/xml/node_utils.hpp"

#include <string>
#include <cmath>

namespace sflt {
namespace fdm {

FDMGlobals::FDMGlobals()
{
   this->g = nav::getG(0, 0, 0);

   this->uvw = Vector3();
   this->uvwdot = Vector3();
   this->pqr = Vector3();
   this->pqrdot = Vector3();
   this->eulers = Euler();
   this->thrust = Vector3();
   this->thrustMoment = Vector3();
   this->aeroForce = Vector3();
   this->aeroMoment = Vector3();
   this->nedVel = Vector3();
   this->xyz = Vector3();
   this->deflections = Vector3();
   this->windVel = Vector3();
   this->windGust = Vector3();

   this->modules = std::vector<FDMModule *>();
}

FDMGlobals::~FDMGlobals()
{
   for (int i = 0; i < modules.size(); i++)
   {
      delete modules[i];
   }
}

void FDMGlobals::addModule(FDMModule *module)
{
   this->modules.push_back(module);
}

void FDMGlobals::initialize()
{
   if (rootNode == nullptr)
      return;

   for (unsigned int i = 0; i < modules.size(); i++)
   {
      modules[i]->initialize(rootNode);
      modules[i]->lastTime = 0;
   }
}

void FDMGlobals::initialize(xml::Node* node)
{
   rootNode = node;

   mass = (UnitConvert::toKilos(getDouble(node, "InitialConditions/Weight", 0.0)));

   xml::Node *wind = node->getChild("Wind");
   if (wind != 0)
   {
      double wspeed = UnitConvert::toMPS(xml::getDouble(wind, "Speed", 0));
      double dir = UnitConvert::toRads(xml::getDouble(wind, "Direction", 0) + 180);
      windVel.set1(wspeed * std::cos(dir));
      windVel.set2(wspeed * std::sin(dir));
      windVel.set3(0);
   }

   // general
   xml::Node *tmp = node->getChild("InitialConditions/Position");
   lat = UnitConvert::toRads(xml::getDouble(tmp, "Latitude", 0.0));
   lon = UnitConvert::toRads(xml::getDouble(tmp, "Longitude", 0.0));
   alt = UnitConvert::toMeters(xml::getDouble(tmp, "Altitude", 0.0));

   tmp = node->getChild("InitialConditions/Orientation");
   eulers = Euler(
       UnitConvert::toRads(xml::getDouble(tmp, "Heading", 0.0)),
       UnitConvert::toRads(xml::getDouble(tmp, "Pitch", 0.0)),
       UnitConvert::toRads(xml::getDouble(tmp, "Roll", 0.0)));

   const double speed = UnitConvert::toMPS(xml::getDouble(node, "InitialConditions/Airspeed", 0.0));
   const double mach = getDouble(node, "InitialConditions/Mach", 0);
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

   initialize();
}

void FDMGlobals::update(double timestep)
{
   simTime += timestep;

   for (unsigned int i = 0; i < modules.size(); i++)
   {
      timestep = simTime - modules[i]->lastTime;
      if (timestep >= modules[i]->frameTime)
      {
         modules[i]->lastTime = simTime;
         modules[i]->update(timestep);
      }
   }
   frameNum++;
}

void FDMGlobals::setProperty(std::string tag, double val)
{
   for (int i = 0; i < modules.size(); i++)
   {
      modules[i]->setProperty(tag, val);
   }
}

}
}
