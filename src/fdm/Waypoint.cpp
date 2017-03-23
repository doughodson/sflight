
#include "sflt/fdm/Waypoint.hpp"

namespace sflt {
namespace fdm {

Waypoint::Waypoint(double lat, double lon, double alt, double speed, double bearing)
    : lat(lat), lon(lon), alt(alt), speed(speed), bearing(bearing)
{
}

double Waypoint::getLat()
{
   return lat;
}

void Waypoint::setLat(double radLat)
{
   lat = radLat;
}

double Waypoint::getLon()
{
   return lon;
}

void Waypoint::setLon(double radLon)
{
   lon = radLon;
}

double Waypoint::getAlt()
{
   return alt;
}

void Waypoint::setAlt(double meterAlt)
{
   this->alt = meterAlt;
}

double Waypoint::getSpeed()
{
   return speed;
}

void Waypoint::setSpeed(double speed)
{
   this->speed = speed;
}

double Waypoint::getBearing()
{
   return bearing;
}

void Waypoint::setBearing(double radBearing)
{
   this->bearing = radBearing;
}
}
}
