
#ifndef __sf_fdm_Waypoint_H__
#define __sf_fdm_Waypoint_H__

namespace sf {
namespace fdm {

//------------------------------------------------------------------------------
// Class: Waypoint
//------------------------------------------------------------------------------
class Waypoint
{
 public:
   Waypoint() = default;
   Waypoint(double lat, double lon, double alt, double speed, double bearing);
   virtual ~Waypoint() = default;

   double getSpeed();
   void setSpeed(double speed);

   double getLat();
   void setLat(double radLat);

   double getLon();
   void setLon(double radLon);

   double getAlt();
   void setAlt(double meterAlt);

   double getBearing();
   void setBearing(double radBearing);

 protected:
   double lat {};
   double lon {};
   double alt {};
   double speed {};
   double bearing {};
};

}
}

#endif
