
#include "sf/fdm/Vector3.hpp"

#include "sf/fdm/Earth.hpp"
#include "sf/fdm/UnitConvert.hpp"

namespace sf
{

const double Earth :: epsilon = 0.0818191908426;
const double Earth :: gravEq = 9.7803267714;
const double Earth :: radiusEq = 6378137.0;
const double Earth :: gravConst = 0.00193185138639;
const double Earth :: metersToRadian = 2.0 * PI / 6378137.0;
const double Earth :: radianToMeter = 6378137.0 / 2.0 * PI;

void Earth :: wgs84LatLon( double &lat, double &lon, double alt, double vn, double ve, double time_diff)
{

    double divisor = sqrt( 1 - epsilon * epsilon * sin( lat ) * sin( lat ) );

    double rMeridian = radiusEq * ( 1.- epsilon * epsilon ) / pow( divisor, 3.0);

    double rNormal = radiusEq / divisor;

    //double requiv = sqrt(rMeridian * rNormal);

    //double g = gravEq * ( 1 + gravConst * sin(lat) * sin(lat)) / divisor;

    double dLat = vn / ( rMeridian + alt );

    double dLon = ve / ( (rNormal + alt ) * cos( lat ) );

    lat =  lat + dLat * time_diff;
    lon =  lon + dLon * time_diff;


}

void Earth :: simpleLatLon(double &lat, double &lon, double alt, double vn, double ve, double time_diff )
{

    lat =  lat + vn / (radiusEq + alt ) * time_diff;
    lon =  lon + ve / ( (radiusEq + alt ) * cos( lat ) ) * time_diff;

}

/** calculates the approx heading from lat1, lon1 to lat2, lon2
 *  based on the openmap great circle computations (http://openmap.bbn.com)
 *  @returns the heading in the [-PI..PI] domain
 */
double Earth :: headingBetween( double lat1, double lon1, double lat2, double lon2 )
{
    double londiff = lon2 - lon1;
    double coslat = cos(lat2);

    return atan2(coslat * sin(londiff),  ( cos(lat1) *  sin(lat2) - sin(lat1) * coslat * cos(londiff) ) );
}

/** calculates the distance from lat1, lon1 to lat2, lon2
 *  based on the openmap great circle computations (http://openmap.bbn.com)
 *  @returns the distance in radians
 */
double Earth :: distance( double lat1, double lon1, double lat2, double lon2 )
{
    double latdiff = sin( ((lat2 - lat1) / 2.) );
    double londiff = sin( (lon2 - lon1) / 2. );
    double rval = sqrt( (latdiff * latdiff) + cos(lat1) * cos(lat2) * (londiff * londiff) );

    return 2.0 * asin( rval );
}



/** needs update.  Will always return gravity at lat = 0, lon = 0, alt = 0; */
double Earth :: getG( double lat, double lon, double alt)
{
    return gravEq;
}

/** fills a Vector with the 3-d components of gravity based on current euler
    angles and grav force
*/
void Earth :: getGravForce( Vector3 &v, double theta, double phi, double g )
{

    v.set1(-g * sin(theta) );
    v.set2( g * sin(phi) * cos(theta) );
    v.set3( g * cos(theta) * cos(phi) );

}

}
