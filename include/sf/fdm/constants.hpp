
#ifndef __sf_fdm_constants_H__
#define __sf_fdm_constants_H__

namespace sf {
namespace fdm {

namespace math {
const double PI = 3.14159265358979323846;
const double TWO_PI = 6.28318530717958647692;
}

namespace earth {
const double epsilon = 0.0818191908426;
const double gravEq = 9.7803267714;
const double radiusEq = 6378137.0;
const double gravConst = 0.00193185138639;
const double metersToRadian = 2.0 * math::PI / 6378137.0;
const double radianToMeter = 6378137.0 / 2.0 * math::PI;
}
}
}

#endif
