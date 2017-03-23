
#ifndef __sflight_fdm_WindAxis_H__
#define __sflight_fdm_WindAxis_H__

namespace sflight {
namespace fdm {
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
