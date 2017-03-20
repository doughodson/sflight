
#include "PID.hpp"

#include <cmath>

namespace sf
{

PID :: PID()
{}

PID :: PID( double minVal, double maxVal, double kp)
{

    ts, ti, td, tf = 0.;
    double en = 0.;
    double rn, yn = 0.;

    this->max = maxVal;
    this->min = minVal;
    this->kp = kp;

    double du, u = 0.;

    ep[0] = 0.;
    ep[1] = 0.;

    ed[0] = 0.;
    ed[1] = 0.;
    ed[2] = 0.;

    edf[0] = 0.;
    edf[1] = 0.;
    edf[2] = 0.;
    edf[3] = 0.;

    this->alpha = 0.1;
    this->beta = 1;
    this->gamma = 0;


}

PID :: ~PID()
{}

/** set ti to infinity to ignore integration, td to 0 to ignore differentiation */
void PID :: setIDTimes( double ti, double td)
{
    this->ti = ti + 1E-32;
    this->td = td + 1E-32;
}

/** sets the gain value */
void PID :: setGain( double p)
{
    kp = p;
}

double PID :: getP()
{
    return kp;
}

double PID :: getI()
{
    return ti;
}

double PID :: getD()
{
    return td;
}

/** @param current_pt the current point of the process (resulting action)
 *  @param current_output current point of the effector (such as actuator)
 *  @param desired_pt the point desired for the process
 */
double PID :: getOutput(double timestep, double desired_pt, double current_pt, double current_output)
{
    ts = timestep;
    rn = desired_pt;
    yn = current_pt;
    u = current_output;

    en = rn - yn;
    tf = alpha * td;

    // set the error terms
    ep[1] = ep[0];
    ep[0] = beta * rn - yn;

    ed[2] = ed[1];
    ed[1] = ed[0];
    ed[0] = gamma * rn - yn;

    edf[3] = edf[2];
    edf[2] = edf[1];
    edf[1] = edf[0];
    double tstf = ts/tf;
    edf[0] = edf[1] / (tstf + 1) + ed[0] * tstf / (tstf + 1);

    //System.out.println("edf: " + edf[0]);

    du = kp * ( ep[0] - ep[1] + (ts/ti * en) + td/ts * ( edf[0] - 2 * edf[1] + edf[2] ) );
    if (!std::isinf(du) || std::isnan(du))
       du = 0.0;

    // check for max and min conditions (to prevent integrator windup)
    if (u + du > max )
    {
        return max;
    }
    else if (u + du < min)
    {
        return min;
    }

    return  u + du;

}


} //namespace SimpleFlight
