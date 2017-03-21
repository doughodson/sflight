
#ifndef __Waypoint_H__
#define __Waypoint_H__

namespace sf
{

//------------------------------------------------------------------------------
// Class: Waypoint
//------------------------------------------------------------------------------
class Waypoint
{
 public:
   Waypoint();
   Waypoint(double lat, double lon, double alt, double speed, double bearing);
   ~Waypoint();

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

#endif
