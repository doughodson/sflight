
#include "Waypoint.hpp"

namespace SimpleFlight
{
    Waypoint :: Waypoint()
    {}

    Waypoint :: Waypoint(double lat, double lon, double alt, double speed, double bearing)
    {
        this->lat = lat;
        this->lon = lon;
        this->alt = alt;
        this->speed = speed;
        this->bearing = bearing;
    }
    
    Waypoint :: ~Waypoint()
    {}
    
    double Waypoint :: getLat() {
        return lat;
    }
    
    void Waypoint :: setLat( double radLat ) {
        this->lat = radLat;
    }
    
    double Waypoint :: getLon() {
        return lon;
    }
    
    void Waypoint :: setLon( double radLon ) {
        this->lon = radLon;
    }
    
    double Waypoint :: getAlt() {
        return alt;
    }
    
    void Waypoint :: setAlt( double meterAlt ) {
        this->alt = meterAlt;
    }

    double Waypoint :: getSpeed()
    {
        return speed;
    }

    void Waypoint :: setSpeed( double speed)
    {
        this->speed = speed;
    }
    
    double Waypoint :: getBearing() {
        return bearing;
    }
    
    void Waypoint :: setBearing(double radBearing) {
        this->bearing = radBearing;
    }
}//namespace SimpleFlight
