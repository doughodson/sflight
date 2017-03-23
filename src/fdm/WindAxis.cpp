

#include "sflt/fdm/Vector3.hpp"
#include "sflt/fdm/WindAxis.hpp"

#include <cmath>

namespace sflt {
namespace fdm {

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

    ret.set1(std::cos(alpha) * std::cos(beta) * drag - std::cos(alpha) * std::sin(beta)*sideforce - std::sin(alpha) * lift );
    ret.set2( std::sin(beta) * drag + std::cos(beta) * sideforce );
    ret.set3 ( std::sin(alpha) * std::cos(beta)*drag - std::sin(alpha) * std::sin(beta) * sideforce + std::cos(alpha) * lift );
}

/** takes inputs of forces in the negative body axis directions and returns the lift, drag, and sideforce */
void WindAxis :: bodyToWind ( Vector3& ret, double alpha, double beta, double fx, double fy, double fz)
{
    ret.set1( fx * std::sin(alpha) * std::cos(beta) - fy * std::sin(alpha) * std::sin(beta) - fz * std::cos(alpha) );
    ret.set2(-fx * std::cos(alpha) * std::cos(beta) + fy * std::cos(alpha) * std::sin(beta) - fz * std::sin(alpha) );
    ret.set3( fx * std::sin(beta) - fy * std::cos(beta) );
}
}
}
