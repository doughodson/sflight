
#include "sflight/mdls/modules/TableAero.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/Table2D.hpp"
#include "sflight/mdls/Table3D.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/Vector3.hpp"
#include "sflight/mdls/WindAxis.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

TableAero::TableAero(Player* player, const double frameRate) : Module(player, frameRate) {}

void TableAero::update(const double timestep)
{
   if (player == nullptr)
      return;

   double cl = liftTable->interp(player->mach, player->alt, player->alpha);
   double cd = dragTable->interp(player->mach, player->alt, cl);

   double qbarS = 0.5 * player->vInf * player->vInf * player->rho * wingArea;

   WindAxis::windToBody(player->aeroForce, player->alpha, player->beta, cl * qbarS, cd * qbarS,
                        0);

   player->fuelflow = thrustTable->interp(player->mach, player->alt, player->throttle);
   player->fuel = player->fuel - player->fuelflow * timestep;
   player->mass = player->mass - player->fuelflow * timestep;

   double thrust = thrustTable->interp(player->mach, player->alt, player->throttle);
   player->thrust.set1(thrust * std::cos(thrustAngle));
   player->thrust.set2(0);
   player->thrust.set3(-thrust * std::sin(thrustAngle));
}
}
}
