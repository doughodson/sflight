
#include "sflight/xml_bindings/init_Engine.hpp"

#include "sflight/mdls/modules/Engine.hpp"
#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_Engine(xml::Node* node, mdls::Engine* engine)
{
   xml::Node* tmp = node->getChild("Design");

   const double designAlt =
       mdls::UnitConvert::toMeters(xml::getDouble(tmp, "DesignAltitude", 0.0));
   engine->designRho = mdls::Atmosphere::getRho(designAlt);
   engine->designTemp = mdls::Atmosphere::getTemp(designAlt);
   engine->designPress = mdls::Atmosphere::getPressure(designAlt);

   double speedSound = mdls::Atmosphere::getSpeedSound(engine->designTemp);
   double airRatio =
       engine->designTemp * engine->designPress / engine->seaLevelPress / engine->seaLevelTemp;

   engine->thrustAngle = mdls::UnitConvert::toRads(xml::getDouble(tmp, "ThrustAngle", 0.0));

   double designThrust =
       xml::getDouble(tmp, "ThrustToWeight", 0.0) * xml::getDouble(tmp, "DesignWeight", 0.0);
   engine->designThrust = mdls::UnitConvert::toNewtons(designThrust);
   //   this->designThrust = designThrust;
   designThrust = designThrust / airRatio;
   engine->staticThrust = designThrust;
   engine->thrustSlope = 0.0;

   std::cout << "design thrust: " << mdls::UnitConvert::toLbsForce(designThrust) << std::endl;

   // setup fuel flow slope
   double ff_1 =
       mdls::UnitConvert::toKilos(xml::getDouble(tmp, "CruiseCondition/FuelFlow", 0) / 3600.0);
   double mach_1 = xml::getDouble(tmp, "CruiseCondition/Mach", 0.0);
   if (mach_1 == 0)
      mach_1 = mdls::UnitConvert::toMPS(xml::getDouble(tmp, "CruiseCondition/Airspeed", 0.0)) /
               speedSound;
   double throttle_1 = xml::getDouble(tmp, "CruiseCondition/Throttle", 0.0);
   if (throttle_1 == 0)
      throttle_1 =
          mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "CruiseCondition/Thrust", 0.0)) /
          designThrust;
   ff_1 = ff_1 / throttle_1 / airRatio;

   double thrust_1 = designThrust / throttle_1 / airRatio;

   double ff_2 = mdls::UnitConvert::toKilos(
       xml::getDouble(tmp, "ClimbCondition/FuelFlow", 0.0) / 3600.0);
   double mach_2 = xml::getDouble(tmp, "ClimbCondition/Mach", 0);
   if (mach_2 == 0)
      mach_2 = mdls::UnitConvert::toMPS(xml::getDouble(tmp, "ClimbCondition/Airspeed", 0.0)) /
               speedSound;
   double throttle_2 = xml::getDouble(tmp, "ClimbCondition/Throttle", 0.0);
   if (throttle_2 == 0)
      throttle_2 =
          mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "ClimbCondition/Thrust", 0.0)) /
          designThrust;
   ff_2 = ff_2 / throttle_2 / airRatio;

   engine->FFslope = (ff_2 - ff_1) / (mach_2 - mach_1);

   engine->staticFF = ff_2 - engine->FFslope * mach_2;

   std::cout << "static ff: " << mdls::UnitConvert::toLbs(engine->staticFF) * 3600 * airRatio
             << std::endl;

   // set initial conditions
   engine->player->throttle = xml::getDouble(node, "InitialConditions/Throttle", 0);
   engine->player->rpm = engine->player->throttle;
}
}
}
