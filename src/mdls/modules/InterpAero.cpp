
#include "sflight/mdls/modules/InterpAero.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/Vector3.hpp"
#include "sflight/mdls/WindAxis.hpp"
#include "sflight/mdls/constants.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

InterpAero::InterpAero(Player* player, const double frameRate) : Module(player, frameRate) {}

void InterpAero::update(const double timestep)
{
   if (player == nullptr)
      return;

   const double qbar = 0.5 * player->vInf * player->vInf * player->rho * wingArea;

   double cl = a1 + a2 * player->alpha;
   double cd = b1 + b2 * cl * cl;
   const double cy = -2.0 * player->beta;

   if (usingMachEffects) {
      const double beta_mach = getBetaMach(player->mach);
      cl /= beta_mach;
      cd /= beta_mach;
   }

   WindAxis::windToBody(player->aeroForce, player->alpha, player->beta, cl * qbar, cd * qbar,
                        cy * qbar);
}

void InterpAero::createCoefs(const double theta, const double thrust, const double vz,
                             const double u, double& alpha, double& cl, double& cd)
{
   // in this version, thrust is always horizontal
   double thrustAngle = 0;

   const double w = (-vz + u * std::sin(theta)) * std::cos(theta);

   const double vInf = std::sqrt(w * w + u * u);

   alpha = std::atan2(w, u);

   const double q = 0.5 * Atmosphere::getRho(designAlt) * vInf * vInf;

   const double zforce = (-thrust * std::sin(thrustAngle) + designWeight * std::cos(theta)) / q;
   const double xforce = (thrust * std::cos(thrustAngle) - designWeight * std::sin(theta)) / q;

   Vector3 aero;
   WindAxis::bodyToWind(aero, alpha, 0., xforce, 0., zforce);

   cl = -aero.get1() / wingArea;
   cd = -aero.get2() / wingArea;
}

double InterpAero::getBetaMach(const double x) const
{
   double mach {x};
   // if (mach > 1.02 ) return 1 + std::sqrt( mach * mach - 1.0);
   if (mach > 1.05)
      mach = 1.0 / mach;
   if (mach > 0.95)
      mach = 0.95;
   return std::sqrt(1.0 - mach * mach);
}
}
}
