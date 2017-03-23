
#ifndef __sflight_fdm_nav_utils_H__
#define __sflight_fdm_nav_utils_H__

#include "sflight/fdm/constants.hpp"

namespace sflight {
namespace fdm {
class Vector3;
namespace nav {

// earth related constants
const double epsilon = 0.0818191908426;
const double gravEq = 9.7803267714;
const double radiusEq = 6378137.0;
const double gravConst = 0.00193185138639;
const double metersToRadian = 2.0 * math::PI / 6378137.0;
const double radianToMeter = 6378137.0 / 2.0 * math::PI;

// navigation oriented functions
bool wgs84LatLon(double* const lat, double* const lon, const double alt, const double vn,
                 const double ve, const double time_diff);

bool simpleLatLon(double* const lat, double* const lon, const double alt, const double vn,
                  const double ve, const double time_diff);

double headingBetween(const double lat1, const double lon1, const double lat2,
                      const double lon2);

double distance(const double lat1, const double lon1, const double lat2, const double lon2);

double getG(const double lat, const double lon, const double alt);

bool getGravForce(Vector3* const v, const double theta, const double phi, const double g);
}
}
}
#endif
