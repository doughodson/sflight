
#ifndef __Earth_H__
#define __Earth_H__

namespace sf {
class Vector3;

//------------------------------------------------------------------------------
// Class: Earth
//------------------------------------------------------------------------------
class Earth
{
public:
   static const double epsilon;
   static const double gravEq;
   static const double radiusEq;
   static const double gravConst;

   static const double metersToRadian;
   static const double radianToMeter;

   static void wgs84LatLon(double &lat, double &lon, double alt, double vn, double ve, double time_diff);

   static void simpleLatLon(double &lat, double &lon, double alt, double vn, double ve, double time_diff);

   static double headingBetween(double lat1, double lon1, double lat2, double lon2);

   static double distance(double lat1, double lon1, double lat2, double lon2);

   static double getG(double lat, double lon, double alt);

   static void getGravForce(Vector3 &v, double theta, double phi, double g);
};

}

#endif
