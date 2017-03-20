
#include <math.h>
#include "Vector3.hpp"
#include "WindAxis.hpp"

namespace SimpleFlight
{
/** takes an input of drag, sideforce, and lift and returns [fx, fy, fz] in the body axis
     *  @param lift force 90 degrees to Vinf ( -z direction for alpha = 0)
     *  @param drag force in the direction of Vinf (-x direction for alpha, beta = 0)
     *  @param sideforce force in 90 deg to Vinf (y direction for beta = 0)
     *  @param Vector3 ret a Vector3 to fill
     *  @returns void
     */
void WindAxis :: windToBody ( Vector3& ret, double alpha, double beta, double lift, double drag, double sideforce )
{
    lift = -lift;
    drag = -drag;

    ret.set1(cos(alpha)*cos(beta) * drag - cos(alpha)*sin(beta)*sideforce - sin(alpha) * lift );
    ret.set2( sin(beta)*drag + cos(beta)*sideforce );
    ret.set3 ( sin(alpha)*cos(beta)*drag - sin(alpha)*sin(beta)*sideforce + cos(alpha)*lift );

}

/** takes inputs of forces in the negative body axis directions and returns the lift, drag, and sideforce */
void WindAxis :: bodyToWind ( Vector3& ret, double alpha, double beta, double fx, double fy, double fz)
{
    ret.set1( fx * sin(alpha)*cos(beta) - fy*sin(alpha)*sin(beta) - fz*cos(alpha) );
    ret.set2(-fx * cos(alpha)*cos(beta) + fy*cos(alpha)*sin(beta) - fz*sin(alpha) );
    ret.set3( fx * sin(beta) - fy * cos(beta) );
}
} //namespace SimpleFlight

