
#include "sflight/xml_bindings/init_InterpAero.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/InterpAero.hpp"

#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"
#include "sflight/mdls/modules/Atmosphere.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_InterpAero(xml::Node* node, mdls::InterpAero* iaero)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Module: InterpAero"        << std::endl;
   std::cout << "-------------------------" << std::endl;

   xml::Node* tmp{node->getChild("Design")};

   iaero->designAlt = mdls::UnitConvert::toMeters(xml::getDouble(tmp, "DesignAltitude", 0.0));
   iaero->designWeight = mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "DesignWeight", 0.0));

   iaero->wingSpan = mdls::UnitConvert::toMeters(xml::getDouble(tmp, "WingSpan", 6.0));
   iaero->wingArea = mdls::UnitConvert::toSqMeters(xml::getDouble(tmp, "WingArea", 6.0));

   iaero->wingEffects = mdls::math::PI * iaero->wingSpan * iaero->wingSpan / iaero->wingArea;
   // wingEffects = 1.0;

   iaero->thrustAngle = mdls::UnitConvert::toRads(xml::getDouble(tmp, "ThrustAngle", 0.0));

   const double thrustRatio{xml::getDouble(tmp, "ThrustToWeight", 0.0) * iaero->designWeight};
   const double speedSound{mdls::Atmosphere::getSpeedSound(mdls::Atmosphere::getTemp(iaero->designAlt))};

   // cruise condition
   double pitch{mdls::UnitConvert::toRads(xml::getDouble(tmp, "CruiseCondition/Pitch", 0.0))};
   double airspeed{mdls::UnitConvert::toMPS(xml::getDouble(tmp, "CruiseCondition/Airspeed", 0.0))};
   double vs{mdls::UnitConvert::FPMtoMPS(xml::getDouble(tmp, "CruiseCondition/VS", 0.0))};
   if (airspeed < 1E-6) {
      airspeed = xml::getDouble(tmp, "CruiseCondition/Mach", 0.0) * speedSound;
   }

   double thrust{mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "CruiseCondition/Thrust", 0.0))};
   if (thrust < 1E-6) {
      thrust = xml::getDouble(tmp, "CruiseCondition/Throttle", 0.0) * thrustRatio;
   }

   iaero->createCoefs(pitch, thrust, vs, airspeed, iaero->cruiseAlpha, iaero->cruiseCL, iaero->cruiseCD);

   double mach{};
   if (iaero->usingMachEffects) {
      mach = airspeed / speedSound;
      const double beta_mach = iaero->getBetaMach(mach);
      iaero->cruiseCL *= beta_mach;
      iaero->cruiseCD *= beta_mach;
   }

   // climb condition
   pitch = mdls::UnitConvert::toRads(xml::getDouble(tmp, "ClimbCondition/Pitch", 0.0));
   airspeed = mdls::UnitConvert::toMPS(xml::getDouble(tmp, "ClimbCondition/Airspeed", 0.0));
   vs = mdls::UnitConvert::FPMtoMPS(xml::getDouble(tmp, "ClimbCondition/VS", 0.0));
   if (airspeed < 1E-6) {
      airspeed = xml::getDouble(tmp, "ClimbCondition/Mach", 0.0) * speedSound;
   }
   mach = airspeed / speedSound;

   thrust = mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "ClimbCondition/Thrust", 0.0));
   if (thrust < 1E-6) {
      thrust = xml::getDouble(tmp, "ClimbCondition/Throttle", 0.0) * thrustRatio;
   }

   iaero->createCoefs(pitch, thrust, vs, airspeed, iaero->climbAlpha, iaero->climbCL, iaero->climbCD);

   if (iaero->usingMachEffects) {
      mach = airspeed / speedSound;
      const double beta_mach = iaero->getBetaMach(mach);
      iaero->climbCL *= beta_mach;
      iaero->climbCD *= beta_mach;
   }

   iaero->a2 = (iaero->cruiseCL - iaero->climbCL) / (iaero->cruiseAlpha - iaero->climbAlpha);
   iaero->a1 = iaero->climbCL - iaero->a2 * iaero->climbAlpha;

   iaero->b2 = (iaero->cruiseCD - iaero->climbCD) /
               (iaero->cruiseCL * iaero->cruiseCL - iaero->climbCL * iaero->climbCL);
   iaero->b1 = iaero->climbCD - iaero->b2 * iaero->climbCL * iaero->climbCL;

   std::cout << "-------------------------" << std::endl;
}
}
}
