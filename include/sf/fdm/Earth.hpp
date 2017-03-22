
#ifndef __sf_fdm_Earth_H__
#define __sf_fdm_Earth_H__

namespace sf {
namespace fdm {
class Vector3;

//------------------------------------------------------------------------------
// Class: Earth
//------------------------------------------------------------------------------
class Earth
{
 public:
   static bool wgs84LatLon(double* const lat, double* const lon, const double alt,
                           const double vn, const double ve, const double time_diff);

   static bool simpleLatLon(double* const lat, double* const lon, const double alt,
                            const double vn, const double ve, const double time_diff);

   static double headingBetween(const double lat1, const double lon1, const double lat2,
                                const double lon2);

   static double distance(const double lat1, const double lon1, const double lat2,
                          const double lon2);

   static double getG(const double lat, const double lon, const double alt);

   static bool getGravForce(Vector3* const v, const double theta, const double phi,
                            const double g);
};
}
}

#endif
