
#include "sflight/xml_bindings/init_InverseDesign.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/Atmosphere.hpp"
#include "sflight/mdls/modules/InverseDesign.hpp"

#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace sflight {
namespace xml_bindings {

void init_InverseDesign(xml::Node* node, mdls::InverseDesign* invDsg)
{
   xml::Node* tmp = node->getChild("Design");

   invDsg->usingMachEffects = xml::getBool(tmp, "CompressibleFlow", false);

   // get Engine parameters
   invDsg->thrustAngle =
       mdls::UnitConvert::toRads(xml::getDouble(tmp, "Engine/ThrustAngle", 0.0));
   invDsg->staticThrust =
       mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "Engine/StaticThrust", 0.0));

   std::string engineType = xml::get(tmp, "Engine/Type", "");
   if (engineType == "Turbojet") {
      invDsg->dTdM = 0;
      invDsg->dTdRho = 1;
   } else if (engineType == "Turbofan") {
      invDsg->dTdM = -0.2;
      invDsg->dTdRho = 1;
   } else {
      invDsg->dTdM = 0;
      invDsg->dTdRho = 0;
   }

   // setup flight conditions (2 points expected)
   std::vector<xml::Node*> fcNodes = tmp->getChildren("FlightConditions/FlightCondition");

   // get default values
   invDsg->designWeight = xml::getDouble(tmp, "FlightConditions/Weight", 0.0);
   invDsg->wingSpan =
       mdls::UnitConvert::toMeters(xml::getDouble(tmp, "FlightCondiitons/WingSpan", 6.0));
   invDsg->wingArea =
       mdls::UnitConvert::toSqMeters(xml::getDouble(tmp, "FlightConditions/WingArea", 6.0));
   invDsg->designAlt = xml::getDouble(tmp, "FlightConditions/Altitude", 0.0);

   const int size = fcNodes.size();

   auto cl = std::unique_ptr<double[]>{new double[size]};
   auto cd = std::unique_ptr<double[]>{new double[size]};
   auto alpha = std::unique_ptr<double[]>{new double[size]};
   auto tsfc = std::unique_ptr<double[]>{new double[size]};
   auto mach = std::unique_ptr<double[]>{new double[size]};

   for (int i = 0; i < size; i++) {
      tmp = fcNodes[i];

      const double pitch = mdls::UnitConvert::toRads(xml::getDouble(tmp, "Pitch", 0.0));
      double airspeed = mdls::UnitConvert::toMPS(xml::getDouble(tmp, "Airspeed", 0.0));
      const double vs = mdls::UnitConvert::FPMtoMPS(xml::getDouble(tmp, "VS", 0.0));
      const double alt =
          mdls::UnitConvert::toMeters(xml::getDouble(tmp, "Altitude", invDsg->designAlt));
      const double speedSound =
          mdls::Atmosphere::getSpeedSound(mdls::Atmosphere::getTemp(alt));
      const double rho = mdls::Atmosphere::getRho(alt);
      const double weight =
          mdls::UnitConvert::toNewtons(xml::getDouble(tmp, "Weight", invDsg->designWeight));

      if (airspeed < 1E-6) {
         mach[i] = xml::getDouble(tmp, "Mach", 0.0);
         airspeed = mach[i] * speedSound;
      } else {
         mach[i] = airspeed / speedSound;
      }

      const double thrust =
          invDsg->getThrust(rho, mach[i], xml::getDouble(tmp, "Throttle", 0.0));

      invDsg->getAeroCoefs(pitch, airspeed, vs, rho, weight, thrust, alpha[i], cl[i], cd[i]);

      tsfc[i] =
          mdls::UnitConvert::toKilos(xml::getDouble(tmp, "FuelFlow", 0.0) / 3600.0) / thrust;

      // apply compressibility if used.  this finds the incompressible cl and cd
      // values by deviding by the
      // prandtl-glauert denominator.
      if (invDsg->usingMachEffects) {
         const double beta_mach = mach[i] > 0.95 ? 1 : std::sqrt(1.0 - mach[i] * mach[i]);
         cl[i] *= beta_mach;
         cd[i] *= beta_mach;
      }
   }

   if (size == 1) {
      // find cdo and clo and assume dCLdalpa = 2*PI, dCDdCL = 1/(PI*AR)
      invDsg->a = 2.0 * mdls::math::PI;
      invDsg->b =
          1.0 / mdls::math::PI / (invDsg->wingSpan * invDsg->wingSpan / invDsg->wingArea);
      invDsg->clo = cl[0] - invDsg->a * alpha[0];
      invDsg->cdo = cd[0] - invDsg->b * cl[0] * cl[0];

      invDsg->dTSFCdM = 0.0;
      invDsg->staticTSFC = tsfc[0];
   } else if (size > 1) {
      // setup cl and cd parameters using the two points

      const double alphaLift_0 = std::sin(alpha[0]) * std::cos(alpha[0]);
      const double alphaLift_1 = std::sin(alpha[1]) * std::cos(alpha[1]);

      invDsg->a = (cl[1] - cl[0]) / (alphaLift_1 - alphaLift_0);
      invDsg->clo = cl[1] - invDsg->a * alphaLift_1;

      // b = (cd[1] - cd[0]) / ( cl[1] * cl[1] - cl[0] * cl[0] );
      // cdo = cd[0] - b * cl[0] * cl[0];
      const double alphaDrag_0 = std::sin(alpha[0]) * std::sin(alpha[0]);
      const double alphaDrag_1 = std::sin(alpha[1]) * std::sin(alpha[1]);

      invDsg->b = (cd[1] - cd[0]) / (alphaDrag_1 - alphaDrag_0);
      invDsg->cdo = cd[0] - invDsg->b * alphaDrag_0;

      invDsg->dTSFCdM = (tsfc[1] - tsfc[0]) / (mach[1] - mach[0]);
      invDsg->staticTSFC = tsfc[0] + invDsg->dTSFCdM * (0 - mach[0]);
   }

   std::cout << "clo: " << invDsg->clo << " dCLda: " << invDsg->a << std::endl;
   std::cout << "cdo: " << invDsg->cdo << " dCDda: " << invDsg->b << std::endl;

   // set initial conditions
   invDsg->player->throttle = xml::getDouble(node, "InitialConditions/Throttle", 0.0);
   invDsg->player->rpm = invDsg->player->throttle;
}
}
}
