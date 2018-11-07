
#ifndef __sflight_mdls_FDM_H__
#define __sflight_mdls_FDM_H__

#include "sflight/xml/Node.hpp"

#include "sflight/mdls/AutoPilotCmds.hpp"
#include "sflight/mdls/Euler.hpp"
#include "sflight/mdls/Quaternion.hpp"
#include "sflight/mdls/Vector3.hpp"
#include "sflight/mdls/nav_utils.hpp"

#include "sflight/xml_bindings/init_Player.hpp"

#include <vector>

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Module;

//------------------------------------------------------------------------------
// Class: Player
// Description: Player/Platform/Entity object with an associated
//              flight dynamics model
//------------------------------------------------------------------------------
class Player
{
 public:
   Player();
   ~Player();

   void addModule(Module* module);
   void update(double timestep);

   friend void xml_bindings::init_Player(xml::Node*, Player*);

   // lat, lon (radians) and alt (meters)
   double lat{};
   double lon{};
   double alt{};

   // mass (kg)
   double mass{};

   // air density (kg/m3)
   double rho{1.22};

   // magnitude of vehicle true airspeed (m/s)
   double vInf{1e-32};

   // mach number
   double mach{};

   // angle of attack (radians)
   double alpha{};

   // sideslip angle (radians)
   double beta{};

   // change in alpha with time (rad/sec)
   double alphaDot{};

   // change in beta with time (rads/sec)
   double betaDot{};

   // altitude above ground (meters)
   double altagl{};

   // terrain elevation (meters)
   double terrainElev{};

   // gravitational accel (m/s2)
   double g{};

   // velocity in the body axis [u, v, w] (m/s)
   Vector3 uvw;

   // acceleration in the body axis (m/s2)
   Vector3 uvwdot;

   // angular velocities (rad/s)
   Vector3 pqr;

   // angular accel (rad/s2)
   Vector3 pqrdot;

   // euler angles (psi, theta, phi)
   Euler eulers;

   // thrust in the [x, y, z] directions (newtons)
   Vector3 thrust;

   // propulsion moment around [roll, pitch, yaw] axes (newton-m)
   Vector3 thrustMoment;

   // aero forces in the [x, y, z] directions (newtons)
   Vector3 aeroForce;

   // aero moments around [roll, pitch, yaw] axes (newton-m)
   Vector3 aeroMoment;

   // velocity in the earth plane [vnorth, veast, vdown] (m/s)
   Vector3 nedVel;

   // position in the x-y-z coordinates [north, east, down] from starting point (meters)
   Vector3 xyz;

   // control surface deflections [aileron elevator rudder] (radians)
   Vector3 deflections;

   // Inertia (also a matrix that is the inertia tensor)
   // Inertia inertia = new Inertia();

   // background wind vel [north east down]  without turbulence (m/s)
   Vector3 windVel;

   // wind gust [north east down] in m/s
   Vector3 windGust;

   // engine-related values
   double throttle{};
   double rpm{};
   double fuel{};     // kilos
   double fuelflow{}; // kilos/sec

   // sim related items
   std::size_t frameNum{};
   double simTime{};
   bool paused{true};

   // Autopilot commands
   AutoPilotCmds autoPilotCmds;

   std::vector<Module*> modules{};
};
}
}

#endif
