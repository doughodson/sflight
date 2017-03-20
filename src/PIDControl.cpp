// this is taken from a Flightgear help page as a simple PIDControl algorithm. The original source is not cited.
// it is identical to the PID controller with differential control removed

#include "PIDControl.hpp"

#include <cmath>

namespace sf {

PIDControl :: PIDControl() : PIControl() {}
PIDControl :: ~PIDControl() {}

    PIDControl :: PIDControl(double minVal, double maxVal, double p, double i, double d)
: PIControl(minVal, maxVal, p, i) {

       edf0 = 0.;
       edf1 = 0.;
       edf2 = 0.;

        this->td = d;

    }

    double PIDControl :: getD() {
        return td;
    }

    void PIDControl :: setD( double val) {
        this->td = val;
    }



    /** @param current_pt the current point of the process (resulting action)
     *  @param current_output current point of the effector (such as actuator)
     *  @param desired_pt the point desired for the process
     */
    double PIDControl :: getOutput(double timestep, double desired_pt, double current_pt, double current_output) {
        ts = timestep;
        rn = desired_pt;
        yn = current_pt;
        u = current_output;

        en = rn - yn;
        tf = alpha * td;

        // set the error terms
        ep1 = ep0;
        ep0 = beta * rn - yn;

        edf2 = edf1;
        edf1 = edf0;

        edf0 = edf1 / (tstf + 1) + ed * tstf / (tstf + 1);

        du = kp * ( ep0 - ep1 + ts/ti * en + td/ts * ( edf0 - 2 * edf1 + edf2 ) );

        if (!std::isfinite(du) || std::isnan(du))
            du = 0.0;

        // check for max and min conditions (to prevent integrator windup)
        if (u + du > max ) {
            //System.out.println("returning max: " + max);
            return max;
        }
        else if (u + du < min) {
            //System.out.println("returning min: " + min);
            return min;
        }

        return  u + du;

    }

}
