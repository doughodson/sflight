
#include "FDMGlobals.hpp"

#include "Vector3.hpp"
#include "Euler.hpp"
#include "Quaternion.hpp"
#include "Earth.hpp"

#include "modules/Atmosphere.hpp"
#include "modules/FDMModule.hpp"

#include <string>

#include "UnitConvert.hpp"
#include "xml/NodeUtil.hpp"
#include "UnitConvert.hpp"
#include "Earth.hpp"
#include "AutoPilotCmds.hpp"
#include "xml/Node.hpp"

using namespace std;

namespace sf
{

FDMGlobals::FDMGlobals()
{

   this->lat = 0;
   this->lon = 0;
   this->alt = 0;

   this->mass = 0.;
   this->rho = 1.22;
   this->vInf = 1E-32;
   this->mach = 0.;
   this->alpha = 0.;
   this->beta = 0.;
   this->alphaDot = 0.;
   this->betaDot = 0.;
   this->altagl = 0.;
   this->g = Earth::getG(0, 0, 0);

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

   this->throttle = 0;
   this->rpm = 0;
   this->fuel = 0;
   this->fuelflow = 0;

   this->simTime = 0;
   this->frameNum = 0;
   this->paused = true;

   this->modules = vector<FDMModule *>();
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

   if (rootNode == 0)
      return;

   for (unsigned int i = 0; i < modules.size(); i++)
   {
      modules[i]->initialize(rootNode);
      modules[i]->lastTime = 0;
   }
}

void FDMGlobals::initialize(Node *node)
{

   rootNode = node;

   mass = (UnitConvert::toKilos(NodeUtil::getDouble(node, "InitialConditions/Weight", 0.0)));

   Node *wind = node->getChild("Wind");
   if (wind != 0)
   {
      double wspeed = UnitConvert::toMPS(NodeUtil::getDouble(wind, "Speed", 0));
      double dir = UnitConvert::toRads(NodeUtil::getDouble(wind, "Direction", 0) + 180);
      windVel.set1(wspeed * cos(dir));
      windVel.set2(wspeed * sin(dir));
      windVel.set3(0);
   }

   //general
   Node *tmp = node->getChild("InitialConditions/Position");
   lat = UnitConvert::toRads(NodeUtil::getDouble(tmp, "Latitude", 0.0));
   lon = UnitConvert::toRads(NodeUtil::getDouble(tmp, "Longitude", 0.0));
   alt = UnitConvert::toMeters(NodeUtil::getDouble(tmp, "Altitude", 0.0));

   tmp = node->getChild("InitialConditions/Orientation");
   eulers = Euler(
       UnitConvert::toRads(NodeUtil::getDouble(tmp, "Heading", 0.0)),
       UnitConvert::toRads(NodeUtil::getDouble(tmp, "Pitch", 0.0)),
       UnitConvert::toRads(NodeUtil::getDouble(tmp, "Roll", 0.0)));

   double speed = UnitConvert::toMPS(NodeUtil::getDouble(node, "InitialConditions/Airspeed", 0.0));
   double mach = NodeUtil::getDouble(node, "InitialConditions/Mach", 0);
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

   fuel = UnitConvert::toKilos(NodeUtil::getDouble(node, "InitialConditions/Fuel", 0.0));

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

void FDMGlobals::setProperty(string tag, double val)
{
   for (int i = 0; i < modules.size(); i++)
   {
      modules[i]->setProperty(tag, val);
   }
}

} //namespace SimpleFlight
