
#include "sf/fdm/modules/EOMFiveDOF.hpp"

#include "sf/fdm/modules/Atmosphere.hpp"

#include "sf/fdm/Earth.hpp"
#include "sf/fdm/Euler.hpp"
#include "sf/fdm/FDMGlobals.hpp"
#include "sf/fdm/Quaternion.hpp"
#include "sf/fdm/Vector3.hpp"

#include "sf/xml/Node.hpp"
#include "sf/xml/node_utils.hpp"

#include <cmath>
#include <iostream>

namespace sf {
namespace fdm {

EOMFiveDOF::EOMFiveDOF(FDMGlobals* globals, double frameRate)
    : FDMModule(globals, frameRate)
{
}

void EOMFiveDOF::update(const double timestep) { computeEOM(timestep); }

void EOMFiveDOF::initialize(xml::Node* node)
{
   quat = Quaternion(globals->eulers);
   qdot = Quaternion();
   forces = Vector3();

   uvw = Vector3();
   pqr = Vector3();
   xyz = Vector3();
   gravAccel = Vector3();

   gravConst = Earth::getG(0, 0, 0);
   autoRudder = xml::getBool(node, "Control/AutoRudder", true);
}

void EOMFiveDOF::computeEOM(double timestep)
{

   double mass = globals->mass;

   quat.initialize(globals->eulers);

   // quat.getUVW(globals->uvw, globals->nedVel);

   // add aero and propulsion forces
   Vector3::add(forces, globals->aeroForce, globals->thrust);

   // set the accel based on angular rates.
   globals->uvwdot.set1(globals->pqr.get3() * globals->uvw.get2() -
                        globals->pqr.get2() * globals->uvw.get3());
   if (autoRudder) {
      globals->uvwdot.set2(0);
   } else {
      globals->uvwdot.set2(-globals->pqr.get3() * globals->uvw.get1() +
                           globals->pqr.get1() * globals->uvw.get3());
   }
   globals->uvwdot.set3(globals->pqr.get2() * globals->uvw.get1() -
                        globals->pqr.get1() * globals->uvw.get2());

   // get gravity acceleration for current orientation
   Earth::getGravForce(&gravAccel, globals->eulers.getTheta(),
                       globals->eulers.getPhi(),
                       Earth::getG(globals->lat, globals->lon, globals->alt));

   // set the "g" term in the globals
   globals->g = (globals->uvwdot.get3() + gravAccel.get3()) / gravConst;

   // add gravity accel to uvwdot
   globals->uvwdot.add(gravAccel);

   // add aero and propulsion forces to uvwdot
   Vector3::add(forces, globals->aeroForce, globals->thrust);
   forces.multiply(1. / mass);
   globals->uvwdot.add(forces);

   // compute new velocity vector
   Vector3::multiply(uvw, globals->uvwdot, timestep);
   globals->uvw.add(uvw);

   // compute the new aero angles
   globals->vInf = globals->uvw.magnitude();
   globals->alpha = std::atan2(globals->uvw.get3(), globals->uvw.get1());
   globals->beta = std::asin(globals->uvw.get2() / globals->vInf);
   globals->alphaDot = std::atan2(globals->uvwdot.get3(), globals->uvw.get1());
   globals->betaDot = std::asin(globals->uvwdot.get2() / globals->vInf);

   // adjust yaw rate to match change in beta (no-slip condition)
   if (autoRudder) {
      globals->pqr.set3(globals->betaDot);
      globals->beta = 0;
      globals->uvw.set2(0);
   }

   // test for on ground condition and prevent downward accel if true
   if (globals->altagl < 0) {
      if (globals->eulers.getTheta() < 0) {
         globals->pqr.set2(globals->pqr.get2() > 0 ? globals->pqr.get2() : 0);
      }
      globals->uvw.set3(globals->uvw.get3() < 0 ? globals->uvw.get3() : 0);
      globals->uvwdot.set3(globals->uvwdot.get3() < 0 ? globals->uvwdot.get3()
                                                      : 0);
      // globals->alt = globals->altagl;
   }

   // integrate pqrdot and add to pqr
   Vector3::multiply(pqr, globals->pqrdot, timestep);
   globals->pqr.add(pqr);

   // adjust the quaternion based on the body angular rates
   const double psi = globals->eulers.getPsi();
   quat.getQdot(qdot, globals->pqr.get1(), globals->pqr.get2(),
                globals->pqr.get3());
   qdot.multiply(timestep);
   quat.add(qdot);
   quat.getEulers(globals->eulers);

   // compute the north, east, down velocities
   quat.getDxDyDz(globals->nedVel, globals->uvw);

   // add steady-state wind vel to air velocity
   globals->nedVel.add(globals->windVel);

   // std::cout << "speeds : uvw " << globals->uvw.toString() << ", ned: " <<
   // globals->nedVel.toString() << std::endl;

   // integrate velocities to get new lat, lon
   Earth::wgs84LatLon(&globals->lat, &globals->lon, globals->alt,
                      globals->nedVel.get1(), globals->nedVel.get2(), timestep);

   // update the position in x-y-z space
   Vector3::multiply(xyz, globals->nedVel, timestep);
   globals->xyz.add(xyz);

   // set the new alt (positive vel is downward)
   globals->alt = globals->alt - globals->nedVel.get3() * timestep;

   // set the mach number
   globals->mach = globals->vInf /
                   Atmosphere::getSpeedSound(Atmosphere::getTemp(globals->alt));
}
}
}
