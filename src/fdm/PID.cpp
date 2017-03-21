
#include "sf/fdm/PID.hpp"

#include <cmath>

namespace sf
{

PID::PID(const double minVal, const double maxVal, const double kp)
   : kp(kp), max(maxVal), min(minVal)
{
}

/** set ti to infinity to ignore integration, td to 0 to ignore differentiation */
void PID::setIDTimes(const double ti, const double td)
{
   this->ti = ti + 1E-32;
   this->td = td + 1E-32;
}

/** sets the gain value */
void PID::setGain(const double p)
{
   kp = p;
}

double PID::getP() const
{
   return kp;
}

double PID::getI() const
{
   return ti;
}

double PID::getD() const
{
   return td;
}

/** @param current_pt the current point of the process (resulting action)
 *  @param current_output current point of the effector (such as actuator)
 *  @param desired_pt the point desired for the process
 */
double PID::getOutput(const double timestep, const double desired_pt,
                     const double current_pt, const double current_output)
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
   double tstf = ts / tf;
   edf[0] = edf[1] / (tstf + 1) + ed[0] * tstf / (tstf + 1);

   //System.out.println("edf: " + edf[0]);

   du = kp * (ep[0] - ep[1] + (ts / ti * en) + td / ts * (edf[0] - 2 * edf[1] + edf[2]));
   if (!std::isinf(du) || std::isnan(du))
      du = 0.0;

   // check for max and min conditions (to prevent integrator windup)
   if (u + du > max)
   {
      return max;
   }
   else if (u + du < min)
   {
      return min;
   }

   return u + du;
}

}
