
#include "modules/SimpleEngine.hpp"

#include "modules/Atmosphere.hpp"

#include "FDMGlobals.hpp"
#include "UnitConvert.hpp"
#include "Earth.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"

#include <iostream>
#include <vector>

namespace sf {

SimpleEngine::SimpleEngine(FDMGlobals* globals, double frameRate) : FDMModule(globals, frameRate)
{
   seaLevelTemp = Atmosphere::getTemp(0);
   seaLevelPress = Atmosphere::getPressure(0);
}

SimpleEngine::~SimpleEngine() {}

void SimpleEngine::initialize(Node *node)
{
   Node* tmp = node->getChild("Design");

   double designAlt = UnitConvert::toMeters(NodeUtil::getDouble(tmp, "DesignAltitude", 0));
   designRho = Atmosphere::getRho(designAlt);
   designTemp = Atmosphere::getTemp(designAlt);
   designPress = Atmosphere::getPressure(designAlt);

   double speedSound = Atmosphere::getSpeedSound(designTemp);
   double airRatio = designTemp * designPress / seaLevelPress / seaLevelTemp;

   thrustAngle = UnitConvert::toRads(NodeUtil::getDouble(tmp, "ThrustAngle", 0.0));

   double designThrust = NodeUtil::getDouble(tmp, "ThrustToWeight", 0) * NodeUtil::getDouble(tmp, "DesignWeight", 0);
   designThrust = UnitConvert::toNewtons(designThrust);
   this->designThrust = designThrust;
   designThrust = designThrust / airRatio;
   staticThrust = designThrust;
   thrustSlope = 0;

   std::cout << "design thrust: " << UnitConvert::toLbsForce(designThrust) << std::endl;

   // setup fuel flow slope
   double ff_1 = UnitConvert::toKilos(NodeUtil::getDouble(tmp, "CruiseCondition/FuelFlow", 0) / 3600.);
   double mach_1 = NodeUtil::getDouble(tmp, "CruiseCondition/Mach", 0);
   if (mach_1 == 0)
      mach_1 = UnitConvert::toMPS(NodeUtil::getDouble(tmp, "CruiseCondition/Airspeed", 0)) / speedSound;
   double throttle_1 = NodeUtil::getDouble(tmp, "CruiseCondition/Throttle", 0);
   if (throttle_1 == 0)
      throttle_1 = UnitConvert::toNewtons(NodeUtil::getDouble(tmp, "CruiseCondition/Thrust", 0)) / designThrust;
   ff_1 = ff_1 / throttle_1 / airRatio;

   double thrust_1 = designThrust / throttle_1 / airRatio;

   double ff_2 = UnitConvert::toKilos(NodeUtil::getDouble(tmp, "ClimbCondition/FuelFlow", 0) / 3600.);
   double mach_2 = NodeUtil::getDouble(tmp, "ClimbCondition/Mach", 0);
   if (mach_2 == 0)
      mach_2 = UnitConvert::toMPS(NodeUtil::getDouble(tmp, "ClimbCondition/Airspeed", 0)) / speedSound;
   double throttle_2 = NodeUtil::getDouble(tmp, "ClimbCondition/Throttle", 0);
   if (throttle_2 == 0)
      throttle_2 = UnitConvert::toNewtons(NodeUtil::getDouble(tmp, "ClimbCondition/Thrust", 0)) / designThrust;
   ff_2 = ff_2 / throttle_2 / airRatio;

   FFslope = (ff_2 - ff_1) / (mach_2 - mach_1);

   staticFF = ff_2 - FFslope * mach_2;

   std::cout << "static ff: " << UnitConvert::toLbs(staticFF) * 3600 * airRatio << std::endl;

   // set initial conditions
   globals->throttle = NodeUtil::getDouble(node, "InitialConditions/Throttle", 0);
   globals->rpm = globals->throttle;
}

void SimpleEngine::update(double timestep)
{
   if (globals->fuel <= 0)
   {
      globals->thrust.set1(0);
      globals->thrust.set2(0);
      globals->thrust.set3(0);
      return;
   }

   double airRatio = (Atmosphere::getTemp(globals->alt)) * Atmosphere::getPressure(globals->alt) /
                     seaLevelTemp / seaLevelPress;

   //globals->rpm = globals->rpm + (globals->throttle - globals->rpm) / timeConst;
   globals->rpm = globals->throttle;

   //double thrust = globals->rpm * ( staticThrust + thrustSlope * globals->mach ) * airRatio;
   double thrust = globals->rpm * staticThrust * airRatio * globals->mach;

   globals->thrust.set1(thrust * cos(thrustAngle));
   globals->thrust.set2(0);
   globals->thrust.set3(-thrust * sin(thrustAngle));

   globals->fuelflow = globals->rpm * (staticFF + FFslope * globals->mach) * airRatio;
   globals->fuelflow = globals->rpm * staticFF * airRatio;

   globals->fuel -= globals->fuelflow * timestep;
   globals->mass -= globals->fuelflow * timestep;
}

}
