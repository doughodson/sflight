
#include "sflight/mdls/modules/Engine.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Player.hpp"

#include <cmath>

namespace sflight {
namespace mdls {

Engine::Engine(Player* player, const double frameRate) : Module(player, frameRate)
{
   seaLevelTemp = Atmosphere::getTemp(0);
   seaLevelPress = Atmosphere::getPressure(0);
}

Engine::~Engine() {}

void Engine::update(const double timestep)
{
   if (player->fuel <= 0) {
      player->thrust.set1(0);
      player->thrust.set2(0);
      player->thrust.set3(0);
      return;
   }

   double airRatio = (Atmosphere::getTemp(player->alt)) *
                     Atmosphere::getPressure(player->alt) / seaLevelTemp / seaLevelPress;

   // player->rpm = player->rpm + (player->throttle - player->rpm) /
   // timeConst;
   player->rpm = player->throttle;

   // double thrust = player->rpm * ( staticThrust + thrustSlope *
   // player->mach ) * airRatio;
   double thrust = player->rpm * staticThrust * airRatio * player->mach;

   player->thrust.set1(thrust * std::cos(thrustAngle));
   player->thrust.set2(0);
   player->thrust.set3(-thrust * std::sin(thrustAngle));

   player->fuelflow = player->rpm * (staticFF + FFslope * player->mach) * airRatio;
   player->fuelflow = player->rpm * staticFF * airRatio;

   player->fuel -= player->fuelflow * timestep;
   player->mass -= player->fuelflow * timestep;
}
}
}
