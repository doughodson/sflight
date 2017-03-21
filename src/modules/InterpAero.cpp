
#include "modules/InterpAero.hpp"

#include "modules/Atmosphere.hpp"

#include "FDMGlobals.hpp"
#include "UnitConvert.hpp"
#include "WindAxis.hpp"
#include "Vector3.hpp"
#include "xml/Node.hpp"
#include "xml/NodeUtil.hpp"
#include "constants.hpp"

#include <iostream>
#include <cmath>

using namespace std;

namespace sf
{

InterpAero::InterpAero(FDMGlobals *globals, double frameRate) : FDMModule(globals, frameRate)
{}

void InterpAero::update(double timestep)
{
   if (globals == nullptr)
      return;

   double qbar = 0.5 * globals->vInf * globals->vInf * globals->rho * wingArea;

   double cl = a1 + a2 * globals->alpha;
   double cd = b1 + b2 * cl * cl;
   double cy = -2.0 * globals->beta;

   if (usingMachEffects)
   {
      const double beta_mach = getBetaMach(globals->mach);
      cl /= beta_mach;
      cd /= beta_mach;
   }

   WindAxis::windToBody(globals->aeroForce, globals->alpha, globals->beta, cl * qbar, cd * qbar, cy * qbar);
}

void InterpAero::initialize(Node *node)
{
   double thrustRatio{};
   double speedSound{};
   double beta_mach{};
   double pitch{};
   double airspeed{};
   double vs{};
   double mach{};
   double thrust{};

   Node *tmp = node->getChild("Design");

   designAlt = UnitConvert::toMeters(NodeUtil::getDouble(tmp, "DesignAltitude", 0.0));

   designWeight = UnitConvert::toNewtons(NodeUtil::getDouble(tmp, "DesignWeight", 0.0));

   wingSpan = UnitConvert::toMeters(NodeUtil::getDouble(tmp, "WingSpan", 6.0));
   wingArea = UnitConvert::toSqMeters(NodeUtil::getDouble(tmp, "WingArea", 6.0));

   wingEffects = PI * wingSpan * wingSpan / wingArea;
   //wingEffects = 1.0;

   thrustAngle = UnitConvert::toRads(NodeUtil::getDouble(tmp, "ThrustAngle", 0.0));

   thrustRatio = NodeUtil::getDouble(tmp, "ThrustToWeight", 0.0) * designWeight;
   speedSound = Atmosphere::getSpeedSound(Atmosphere::getTemp(designAlt));

   // cruise condition
   pitch = UnitConvert::toRads(NodeUtil::getDouble(tmp, "CruiseCondition/Pitch", 0));
   airspeed = UnitConvert::toMPS(NodeUtil::getDouble(tmp, "CruiseCondition/Airspeed", 0.0));
   vs = UnitConvert::FPMtoMPS(NodeUtil::getDouble(tmp, "CruiseCondition/VS", 0.0));
   if (airspeed < 1E-6)
   {
      airspeed = NodeUtil::getDouble(tmp, "CruiseCondition/Mach", 0) * speedSound;
   }

   thrust = UnitConvert::toNewtons(NodeUtil::getDouble(tmp, "CruiseCondition/Thrust", 0));
   if (thrust < 1E-6)
      thrust = NodeUtil::getDouble(tmp, "CruiseCondition/Throttle", 0) * thrustRatio;

   createCoefs(pitch, thrust, vs, airspeed, cruiseAlpha, cruiseCL, cruiseCD);

   if (usingMachEffects)
   {
      mach = airspeed / speedSound;
      beta_mach = getBetaMach(mach);
      cruiseCL *= beta_mach;
      cruiseCD *= beta_mach;
   }

   // climb condition
   pitch = UnitConvert::toRads(NodeUtil::getDouble(tmp, "ClimbCondition/Pitch", 0));
   airspeed = UnitConvert::toMPS(NodeUtil::getDouble(tmp, "ClimbCondition/Airspeed", 0.0));
   vs = UnitConvert::FPMtoMPS(NodeUtil::getDouble(tmp, "ClimbCondition/VS", 0.0));
   if (airspeed < 1E-6)
   {
      airspeed = NodeUtil::getDouble(tmp, "ClimbCondition/Mach", 0) * speedSound;
   }
   mach = airspeed / speedSound;

   thrust = UnitConvert::toNewtons(NodeUtil::getDouble(tmp, "ClimbCondition/Thrust", 0));
   if (thrust < 1E-6)
      thrust = NodeUtil::getDouble(tmp, "ClimbCondition/Throttle", 0) * thrustRatio;

   createCoefs(pitch, thrust, vs, airspeed, climbAlpha, climbCL, climbCD);

   if (usingMachEffects)
   {
      mach = airspeed / speedSound;
      beta_mach = getBetaMach(mach);
      climbCL *= beta_mach;
      climbCD *= beta_mach;
   }

   a2 = (cruiseCL - climbCL) / (cruiseAlpha - climbAlpha);
   a1 = climbCL - a2 * climbAlpha;

   b2 = (cruiseCD - climbCD) / (cruiseCL * cruiseCL - climbCL * climbCL);
   b1 = climbCD - b2 * climbCL * climbCL;
}

void InterpAero::createCoefs(double theta, double thrust, double vz, double u,
                             double &alpha, double &cl, double &cd)
{
   vz = -vz;

   // in this version, thrust is always horizontal
   double thrustAngle = 0;

   double w = (vz + u * sin(theta)) * cos(theta);

   double vInf = sqrt(w * w + u * u);

   alpha = std::atan2(w, u);

   double q = 0.5 * Atmosphere::getRho(designAlt) * vInf * vInf;

   double zforce = (-thrust * sin(thrustAngle) + designWeight * cos(theta)) / q;
   double xforce = (thrust * cos(thrustAngle) - designWeight * sin(theta)) / q;

   Vector3 aero = Vector3();
   WindAxis::bodyToWind(aero, alpha, 0., xforce, 0., zforce);

   cl = -aero.get1() / wingArea;
   cd = -aero.get2() / wingArea;
}

double InterpAero::getBetaMach(double mach)
{
   //if (mach > 1.02 ) return 1 + sqrt( mach * mach - 1.0);
   if (mach > 1.05)
      mach = 1.0 / mach;
   if (mach > 0.95)
      mach = 0.95;
   return std::sqrt(1.0 - mach * mach);
}
}
