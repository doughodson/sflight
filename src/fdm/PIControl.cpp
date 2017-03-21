
#include "sf/fdm/PIControl.hpp"

#include <cmath>

namespace sf {
namespace fdm {

PIControl::PIControl(double minVal, double maxVal, double p, double i)
   : kp(p), ti(i), max(maxVal), min(minVal)
{
}

void PIControl::setMax(double maxVal)
{
   this->max = maxVal;
}

double PIControl::getMax()
{
   return max;
}

void PIControl::setMin(double minVal)
{
   this->min = minVal;
}

double PIControl::getMin()
{
   return min;
}

/** sets the gain value */
void PIControl::setP(double p)
{
   kp = p;
}

double PIControl::getP()
{
   return kp;
}

double PIControl::getI()
{
   return ti;
}

void PIControl::setI(double val)
{
   this->ti = val;
}

/** @param current_pt the current point of the process (resulting action)
 *  @param current_output current point of the effector (such as actuator)
 *  @param desired_pt the point desired for the process
 */
double PIControl::getOutput(double timestep, double desired_pt, double current_pt, double current_output)
{
   ts = timestep;
   rn = desired_pt;
   yn = current_pt;
   u = current_output;

   en = rn - yn;
   tf = alpha * td;

   // set the error terms
   ep1 = ep0;
   ep0 = beta * rn - yn;

   //System.out.println("edf: " + edf[0]);

   du = kp * (ep0 - ep1 + ts / ti * en);

   if (!std::isinf(du) || std::isnan(du))
      du = 0.0;

   // check for max and min conditions (to prevent integrator windup)
   if (u + du > max)
   {
      //System.out.println("returning max: " + max);
      return max;
   }
   else if (u + du < min)
   {
      //System.out.println("returning min: " + min);
      return min;
   }

   return u + du;
}

}
}
