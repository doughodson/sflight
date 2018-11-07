
#ifndef __sflight_mdls_WindAxis_HPP__
#define __sflight_mdls_WindAxis_HPP__

namespace sflight {
namespace mdls {
class Vector3;

//------------------------------------------------------------------------------
// Class: WindAxis
//------------------------------------------------------------------------------
class WindAxis
{
 public:
   static void bodyToWind(Vector3 &ret, double alpha, double beta, double fx, double fy, double fz);
   static void windToBody(Vector3 &ret, double alpha, double beta, double lift, double drag, double sideforce);
};
}
}

#endif
