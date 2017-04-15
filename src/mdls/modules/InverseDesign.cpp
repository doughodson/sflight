
#include "sflight/mdls/modules/InverseDesign.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/Vector3.hpp"
#include "sflight/mdls/WindAxis.hpp"

#include <cmath>
#include <iostream>
#include <vector>

namespace sflight {
namespace mdls {

InverseDesign::InverseDesign(Player* player, const double frameRate)
    : Module(player, frameRate)
{
}

void InverseDesign::update(const double timestep)
{
   if (player == nullptr)
      return;

   const double qbar = 0.5 * player->vInf * player->vInf * player->rho * wingArea;

   // double cl = clo + a * player->alpha;
   // double cd = cdo + b * cl * cl;
   const double sinAlpha = std::sin(player->alpha);

   double cl = clo + a * sinAlpha * std::cos(player->alpha);
   double cd = cdo + b * sinAlpha * sinAlpha;

   if (usingMachEffects) {
      double beta_mach =
          player->mach > 0.95 ? 1.0 : 1.0 / std::sqrt(1.0 - player->mach * player->mach);
      cl *= beta_mach;
      cd *= beta_mach;
   }

   WindAxis::windToBody(player->aeroForce, player->alpha, player->beta, cl * qbar, cd * qbar,
                        0);

   const double thrust = getThrust(player->rho, player->mach, player->throttle);

   player->thrust.set1(thrust * std::cos(thrustAngle));
   player->thrust.set2(0);
   player->thrust.set3(-thrust * std::sin(thrustAngle));

   player->fuelflow = getFuelFlow(player->rho, player->mach, thrust);
   player->mass -= player->fuelflow * timestep;
   player->fuel -= player->fuelflow * timestep;
}

void InverseDesign::getAeroCoefs(double theta, double u, double vz, double rho, double weight,
                                 double thrust, double& alpha, double& cl, double& cd)
{
   vz = -vz;

   double w = (vz + u * std::sin(theta)) / std::cos(theta);

   double vInf = std::sqrt(w * w + u * u);

   alpha = std::atan2(w, u);

   double q = 0.5 * rho * vInf * vInf;

   double zforce = (-thrust * std::sin(thrustAngle) + weight * std::cos(theta)) / q;
   double xforce = (thrust * std::cos(thrustAngle) - weight * std::sin(theta)) / q;

   Vector3 aero = Vector3();
   WindAxis::bodyToWind(aero, alpha, 0., xforce, 0., zforce);

   cl = -aero.get1() / wingArea;
   cd = -aero.get2() / wingArea;
}

double InverseDesign::getThrust(double rho, double mach, double throttle)
{
   return std::pow(mach, dTdM) * std::pow(rho / 1.225, dTdRho) * staticThrust * throttle;
}

double InverseDesign::getFuelFlow(double rho, double mach, double thrust)
{
   return (staticTSFC + dTSFCdM * mach) * thrust;
}
}
}
