

#ifndef WAYPOINT_H
#define WAYPOINT_H

namespace SimpleFlight
{
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
    double lat;
    double lon;
    double alt;
    double speed;
    double bearing;

};

}//namespace SimpleFlight


#endif

