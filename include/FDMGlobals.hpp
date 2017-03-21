
#ifndef __FDMGlobals_H__
#define __FDMGlobals_H__

#include <vector>

#include "Earth.hpp"
#include "Euler.hpp"
#include "Quaternion.hpp"
#include "Vector3.hpp"
#include "AutoPilotCmds.hpp"
#include "xml/Node.hpp"

namespace sf {
class FDMModule;

//------------------------------------------------------------------------------
// Class: FDMGlobals
// Description: Global variables for use by the flight model
//------------------------------------------------------------------------------
class FDMGlobals
{
public:
    FDMGlobals();
    ~FDMGlobals();

    void addModule(FDMModule* module);
    void initialize();
    void initialize(Node* node);
    void update(double timestep);
    void setProperty(std::string tag, double val);

    // lat, lon (radians) and alt (meters)
    double lat {};
    double lon {};
    double alt {};

    // mass (kg)
    double mass {};

    // air density (kg/m3)
    double rho {1.22};

    // magnitude of vehicle true airspeed (m/s)
    double vInf {1e-32};

    // mach number
    double mach {};

    // angle of attack (radians)
    double alpha {};

    // sideslip angle (radians)
    double beta {};

    // change in alpha with time (rad/sec)
    double alphaDot {};

    // change in beta with time (rads/sec)
    double betaDot {};

    // altitude above ground (meters)
    double altagl {};

    // terrain elevation (meters)
    double terrainElev {};

    // gravitational accel (m/s2)
    double g {};

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
    double throttle {};
	 double rpm {};
	 double fuel {};	    // kilos
	 double fuelflow {};  // kilos/sec

    // sim related items
    unsigned int frameNum {};
    double simTime {};
    bool paused {true};

    // Autopilot commands
    AutoPilotCmds autoPilotCmds;

    Node* rootNode {};
    std::vector <FDMModule*> modules {};
};

}

#endif
