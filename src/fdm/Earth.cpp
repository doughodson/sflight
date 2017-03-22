
#include "sf/fdm/Vector3.hpp"

#include "sf/fdm/Earth.hpp"
#include "sf/fdm/UnitConvert.hpp"
#include "sf/fdm/constants.hpp"

#include <cmath>

namespace sf {
namespace fdm {

bool Earth::wgs84LatLon(double* const lat, double* const lon, const double alt,
                        const double vn, const double ve, const double time_diff)
{
   // check to ensure we have good pointers
   if (!lat || !lon)
      return false;

   const double divisor =
       std::sqrt(1 - earth::epsilon * earth::epsilon * std::sin(*lat) * std::sin(*lat));

   const double rMeridian =
       earth::radiusEq * (1. - earth::epsilon * earth::epsilon) / std::pow(divisor, 3.0);

   const double rNormal = earth::radiusEq / divisor;

   // double requiv = sqrt(rMeridian * rNormal);

   // double g = gravEq * ( 1 + gravConst * sin(lat) * sin(lat)) / divisor;

   const double dLat = vn / (rMeridian + alt);

   const double dLon = ve / ((rNormal + alt) * std::cos(*lat));

   *lat = *lat + dLat * time_diff;
   *lon = *lon + dLon * time_diff;
   return true;
}

bool Earth::simpleLatLon(double* const lat, double* const lon, const double alt,
                         const double vn, const double ve, const double time_diff)
{
   if (!lat || !lon)
      return false;
   *lat = *lat + vn / (earth::radiusEq + alt) * time_diff;
   *lon = *lon + ve / ((earth::radiusEq + alt) * std::cos(*lat)) * time_diff;
   return true;
}

/** calculates the approx heading from lat1, lon1 to lat2, lon2
 *  based on the openmap great circle computations (http://openmap.bbn.com)
 *  @returns the heading in the [-PI..PI] domain
 */
double Earth::headingBetween(const double lat1, const double lon1, const double lat2,
                             const double lon2)
{
   const double londiff = lon2 - lon1;
   const double coslat = std::cos(lat2);

   return std::atan2(
       coslat * std::sin(londiff),
       (std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * coslat * std::cos(londiff)));
}

/** calculates the distance from lat1, lon1 to lat2, lon2
 *  based on the openmap great circle computations (http://openmap.bbn.com)
 *  @returns the distance in radians
 */
double Earth::distance(const double lat1, const double lon1, const double lat2,
                       const double lon2)
{
   const double latdiff = std::sin(((lat2 - lat1) / 2.));
   const double londiff = std::sin((lon2 - lon1) / 2.);
   const double rval =
       std::sqrt((latdiff * latdiff) + std::cos(lat1) * std::cos(lat2) * (londiff * londiff));

   return 2.0 * std::asin(rval);
}

/** needs update.  Will always return gravity at lat = 0, lon = 0, alt = 0; */
double Earth::getG(const double lat, const double lon, const double alt)
{
   return earth::gravEq;
}

/** fills a Vector with the 3-d components of gravity based on current euler
    angles and grav force
*/
bool Earth::getGravForce(Vector3* const v, const double theta, const double phi,
                         const double g)
{
   if (!v)
      return false;
   v->set1(-g * std::sin(theta));
   v->set2(g * std::sin(phi) * std::cos(theta));
   v->set3(g * std::cos(theta) * std::cos(phi));
   return true;
}
}
}
