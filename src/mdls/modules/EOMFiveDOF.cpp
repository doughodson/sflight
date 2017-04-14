
#include "sflight/mdls/modules/EOMFiveDOF.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Euler.hpp"
#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/Quaternion.hpp"
#include "sflight/mdls/Vector3.hpp"

#include "sflight/mdls/nav_utils.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

EOMFiveDOF::EOMFiveDOF(Player* player, const double frameRate) : Module(player, frameRate) {}

void EOMFiveDOF::update(const double timestep) { computeEOM(timestep); }

void EOMFiveDOF::computeEOM(double timestep)
{
   const double mass = player->mass;

   quat.initialize(player->eulers);

   // quat.getUVW(player->uvw, player->nedVel);

   // add aero and propulsion forces
   Vector3::add(forces, player->aeroForce, player->thrust);

   // set the accel based on angular rates.
   player->uvwdot.set1(player->pqr.get3() * player->uvw.get2() -
                        player->pqr.get2() * player->uvw.get3());
   if (autoRudder) {
      player->uvwdot.set2(0);
   } else {
      player->uvwdot.set2(-player->pqr.get3() * player->uvw.get1() +
                           player->pqr.get1() * player->uvw.get3());
   }
   player->uvwdot.set3(player->pqr.get2() * player->uvw.get1() -
                        player->pqr.get1() * player->uvw.get2());

   // get gravity acceleration for current orientation
   nav::getGravForce(&gravAccel, player->eulers.getTheta(), player->eulers.getPhi(),
                     nav::getG(player->lat, player->lon, player->alt));

   // set the "g" term in the player
   player->g = (player->uvwdot.get3() + gravAccel.get3()) / gravConst;

   // add gravity accel to uvwdot
   player->uvwdot.add(gravAccel);

   // add aero and propulsion forces to uvwdot
   Vector3::add(forces, player->aeroForce, player->thrust);
   forces.multiply(1. / mass);
   player->uvwdot.add(forces);

   // compute new velocity vector
   Vector3::multiply(uvw, player->uvwdot, timestep);
   player->uvw.add(uvw);

   // compute the new aero angles
   player->vInf = player->uvw.magnitude();
   player->alpha = std::atan2(player->uvw.get3(), player->uvw.get1());
   player->beta = std::asin(player->uvw.get2() / player->vInf);
   player->alphaDot = std::atan2(player->uvwdot.get3(), player->uvw.get1());
   player->betaDot = std::asin(player->uvwdot.get2() / player->vInf);

   // adjust yaw rate to match change in beta (no-slip condition)
   if (autoRudder) {
      player->pqr.set3(player->betaDot);
      player->beta = 0;
      player->uvw.set2(0);
   }

   // test for on ground condition and prevent downward accel if true
   if (player->altagl < 0) {
      if (player->eulers.getTheta() < 0) {
         player->pqr.set2(player->pqr.get2() > 0 ? player->pqr.get2() : 0);
      }
      player->uvw.set3(player->uvw.get3() < 0 ? player->uvw.get3() : 0);
      player->uvwdot.set3(player->uvwdot.get3() < 0 ? player->uvwdot.get3() : 0);
      // player->alt = player->altagl;
   }

   // integrate pqrdot and add to pqr
   Vector3::multiply(pqr, player->pqrdot, timestep);
   player->pqr.add(pqr);

   // adjust the quaternion based on the body angular rates
   const double psi = player->eulers.getPsi();
   quat.getQdot(qdot, player->pqr.get1(), player->pqr.get2(), player->pqr.get3());
   qdot.multiply(timestep);
   quat.add(qdot);
   quat.getEulers(player->eulers);

   // compute the north, east, down velocities
   quat.getDxDyDz(player->nedVel, player->uvw);

   // add steady-state wind vel to air velocity
   player->nedVel.add(player->windVel);

   // std::cout << "speeds : uvw " << player->uvw.toString() << ", ned: " <<
   // player->nedVel.toString() << std::endl;

   // integrate velocities to get new lat, lon
   nav::wgs84LatLon(&player->lat, &player->lon, player->alt, player->nedVel.get1(),
                    player->nedVel.get2(), timestep);

   // update the position in x-y-z space
   Vector3::multiply(xyz, player->nedVel, timestep);
   player->xyz.add(xyz);

   // set the new alt (positive vel is downward)
   player->alt = player->alt - player->nedVel.get3() * timestep;

   // set the mach number
   player->mach =
       player->vInf / Atmosphere::getSpeedSound(Atmosphere::getTemp(player->alt));
}
}
}
