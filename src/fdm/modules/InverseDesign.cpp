
#include "sf/fdm/modules/InverseDesign.hpp"

#include "sf/xml/Node.hpp"
#include "sf/xml/node_utils.hpp"

#include "sf/fdm/modules/Atmosphere.hpp"

#include "sf/fdm/FDMGlobals.hpp"
#include "sf/fdm/UnitConvert.hpp"
#include "sf/fdm/WindAxis.hpp"
#include "sf/fdm/Vector3.hpp"

#include <iostream>
#include <vector>
#include <cmath>

namespace sf
{

InverseDesign::InverseDesign(FDMGlobals* globals, double frameRate) : FDMModule(globals, frameRate)
{
}

void InverseDesign::update(double timestep)
{
   if (globals == nullptr)
      return;

   double qbar = 0.5 * globals->vInf * globals->vInf * globals->rho * wingArea;

   //double cl = clo + a * globals->alpha;
   //double cd = cdo + b * cl * cl;
   double sinAlpha = std::sin(globals->alpha);

   double cl = clo + a * sinAlpha * cos(globals->alpha);
   double cd = cdo + b * sinAlpha * sinAlpha;

   if (usingMachEffects)
   {
      double beta_mach = globals->mach > 0.95 ? 1.0 : 1.0 / sqrt(1.0 - globals->mach * globals->mach);
      cl *= beta_mach;
      cd *= beta_mach;
   }

   WindAxis::windToBody(globals->aeroForce, globals->alpha, globals->beta, cl * qbar, cd * qbar, 0);

   double thrust = getThrust(globals->rho, globals->mach, globals->throttle);

   globals->thrust.set1(thrust * cos(thrustAngle));
   globals->thrust.set2(0);
   globals->thrust.set3(-thrust * sin(thrustAngle));

   globals->fuelflow = getFuelFlow(globals->rho, globals->mach, thrust);
   globals->mass -= globals->fuelflow * timestep;
   globals->fuel -= globals->fuelflow * timestep;
}

void InverseDesign::initialize(xml::Node *node)
{
   double thrustRatio {};
   double speedSound {};
   double beta_mach {};
   double pitch {};
   double airspeed {};
   double vs {};
   double thrust {};

   double rho {};

   xml::Node *tmp = node->getChild("Design");

   usingMachEffects = xml::getBool(tmp, "CompressibleFlow", false);

   //get Engine parameters
   thrustAngle = UnitConvert::toRads(xml::getDouble(tmp, "Engine/ThrustAngle", 0.0));
   staticThrust = UnitConvert::toNewtons(xml::getDouble(tmp, "Engine/StaticThrust", 0));

   std::string engineType = xml::get(tmp, "Engine/Type", "");
   if (engineType == "Turbojet")
   {
      dTdM = 0;
      dTdRho = 1;
   }
   else if (engineType == "Turbofan")
   {
      dTdM = -0.2;
      dTdRho = 1;
   }
   else
   {
      dTdM = 0;
      dTdRho = 0;
   }

   // setup flight conditions (2 points expected)
   std::vector<xml::Node*> fcNodes = tmp->getChildren("FlightConditions/FlightCondition");

   // get default values
   designWeight = getDouble(tmp, "FlightConditions/Weight", 0.0);
   wingSpan = UnitConvert::toMeters(getDouble(tmp, "FlightCondiitons/WingSpan", 6.0));
   wingArea = UnitConvert::toSqMeters(getDouble(tmp, "FlightConditions/WingArea", 6.0));
   designAlt = getDouble(tmp, "FlightConditions/Altitude", 0.0);

   const int size = fcNodes.size();

   double *cl = new double[size];
   double *cd = new double[size];
   double *alpha = new double[size];
   double *tsfc = new double[size];
   double *mach = new double[size];

   for (int i = 0; i < size; i++)
   {
      tmp = fcNodes[i];

      pitch = UnitConvert::toRads(getDouble(tmp, "Pitch", 0));
      airspeed = UnitConvert::toMPS(getDouble(tmp, "Airspeed", 0.0));
      vs = UnitConvert::FPMtoMPS(getDouble(tmp, "VS", 0.0));
      double alt = UnitConvert::toMeters(getDouble(tmp, "Altitude", designAlt));
      speedSound = Atmosphere::getSpeedSound(Atmosphere::getTemp(alt));
      rho = Atmosphere::getRho(alt);
      double weight = UnitConvert::toNewtons(getDouble(tmp, "Weight", designWeight));

      if (airspeed < 1E-6)
      {
         mach[i] = getDouble(tmp, "Mach", 0);
         airspeed = mach[i] * speedSound;
      }
      else
      {
         mach[i] = airspeed / speedSound;
      }

      thrust = getThrust(rho, mach[i], getDouble(tmp, "Throttle", 0));

      getAeroCoefs(pitch, airspeed, vs, rho, weight, thrust, alpha[i], cl[i], cd[i]);

      tsfc[i] = UnitConvert::toKilos(getDouble(tmp, "FuelFlow", 0) / 3600.) / thrust;

      // apply compressibility if used.  this finds the incompressible cl and cd values by deviding by the
      // prandtl-glauert denominator.
      if (usingMachEffects)
      {
         beta_mach = mach[i] > 0.95 ? 1 : sqrt(1.0 - mach[i] * mach[i]);
         cl[i] *= beta_mach;
         cd[i] *= beta_mach;
      }
   }

   if (size == 1)
   {
      // find cdo and clo and assume dCLdalpa = 2*PI, dCDdCL = 1/(PI*AR)
      a = 2 * PI;
      b = 1.0 / PI / (wingSpan * wingSpan / wingArea);
      clo = cl[0] - a * alpha[0];
      cdo = cd[0] - b * cl[0] * cl[0];

      dTSFCdM = 0;
      staticTSFC = tsfc[0];
   }
   else if (size > 1)
   {
      //setup cl and cd parameters using the two points

      double alphaLift_0 = sin(alpha[0]) * cos(alpha[0]);
      double alphaLift_1 = sin(alpha[1]) * cos(alpha[1]);

      a = (cl[1] - cl[0]) / (alphaLift_1 - alphaLift_0);
      clo = cl[1] - a * alphaLift_1;

      //b = (cd[1] - cd[0]) / ( cl[1] * cl[1] - cl[0] * cl[0] );
      //cdo = cd[0] - b * cl[0] * cl[0];
      double alphaDrag_0 = sin(alpha[0]) * sin(alpha[0]);
      double alphaDrag_1 = sin(alpha[1]) * sin(alpha[1]);

      b = (cd[1] - cd[0]) / (alphaDrag_1 - alphaDrag_0);
      cdo = cd[0] - b * alphaDrag_0;

      dTSFCdM = (tsfc[1] - tsfc[0]) / (mach[1] - mach[0]);
      staticTSFC = tsfc[0] + dTSFCdM * (0 - mach[0]);
   }

   std::cout << "clo: " << clo << " dCLda: " << a << std::endl;
   std::cout << "cdo: " << cdo << " dCDda: " << b << std::endl;

   // set initial conditions
   globals->throttle = getDouble(node, "InitialConditions/Throttle", 0);
   globals->rpm = globals->throttle;

   delete cl;
   delete cd;
   delete alpha;
   delete tsfc;
   delete mach;
}

void InverseDesign::getAeroCoefs(double theta, double u, double vz, double rho, double weight, double thrust,
                                 double &alpha, double &cl, double &cd)
{

   vz = -vz;

   double w = (vz + u * sin(theta)) / cos(theta);

   double vInf = sqrt(w * w + u * u);

   alpha = atan2(w, u);

   double q = 0.5 * rho * vInf * vInf;

   double zforce = (-thrust * sin(thrustAngle) + weight * cos(theta)) / q;
   double xforce = (thrust * cos(thrustAngle) - weight * sin(theta)) / q;

   Vector3 aero = Vector3();
   WindAxis::bodyToWind(aero, alpha, 0., xforce, 0., zforce);

   cl = -aero.get1() / wingArea;
   cd = -aero.get2() / wingArea;
}

double InverseDesign::getThrust(double rho, double mach, double throttle)
{
   return pow(mach, dTdM) * pow(rho / 1.225, dTdRho) * staticThrust * throttle;
}

double InverseDesign::getFuelFlow(double rho, double mach, double thrust)
{
   return (staticTSFC + dTSFCdM * mach) * thrust;
}

}
