
#ifndef __UnitConvert_H__
#define __UnitConvert_H__

#include "sf/fdm/constants.hpp"
#include <cmath>

namespace sf {
namespace fdm {

//------------------------------------------------------------------------------
// basic conversions used in the flight model
//------------------------------------------------------------------------------
class UnitConvert
{
public:
    static inline double toDegs(double rad);
    static inline double toRads(double deg);
    static inline double toMeters(double feet);
    static inline double toSqMeters(double sqFeet);
    static inline double toFeet(double meters);
    static inline double toSqFeet(double sqMeters);
    static inline double toMPS(double knots);
    static inline double toKnots(double metersPerSecond);
    static inline double toKilos( double lbs);
    static inline double toLbs( double kilos);
    static inline double toLbsForce( double newtons);
    static inline double toNewtons( double lbs);
    static inline double toFPM( double metersPerSecond);
    static inline double FPMtoMPS(double feetPerMinute);
    static inline double wrapHeading(double heading, bool isRadians);
    static inline double toHeadingDegrees(double radHeading);
    static inline double toHeadingRadians(double degHeading);
    static inline double NMtoEarthRadians(double distNM);
    static inline double signum( double input);
};

double UnitConvert :: toDegs(double rad)
{
    return rad * 180. / PI;
}


double UnitConvert :: toRads(double deg)
{
    return deg / 180. * PI;
}

double UnitConvert :: toMeters(double feet)
{
	return feet*0.3048;
}

double UnitConvert :: toSqMeters(double sqFeet)
{
    return sqFeet * 0.3048f * 0.3048f;
}


double UnitConvert :: toFeet(double meters)
{
    return  meters/0.3048f;
}

double UnitConvert :: toSqFeet(double sqMeters)
{
    return sqMeters / 0.3048f / 0.3048f;
}

/** converts knots to Meters per second */
double UnitConvert :: toMPS(double knots)
{
    return  knots/1.9438445;
}


double UnitConvert :: toKnots(double metersPerSecond)
{
    return  metersPerSecond*1.9438445f;
}


double UnitConvert :: toKilos( double lbs)
{
    return lbs / 2.2;
}


double UnitConvert :: toLbs( double kilos)
{
    return kilos * 2.2;
}


double UnitConvert :: toLbsForce( double newtons)
{
    return toLbs(newtons) / 9.8;
}


double UnitConvert :: toNewtons( double lbs)
{
    return toKilos(lbs) * 9.8;
}


double UnitConvert :: toFPM( double metersPerSecond)
{
    return toFeet(metersPerSecond) * 60.0;
}


double UnitConvert :: FPMtoMPS(double feetPerMinute)
{
    return toMeters(feetPerMinute) / 60.0;
}

/** returns the sign of the number */
double UnitConvert :: signum( double input)
{
    return input >= 0 ? 1. : -1.;
}

/** clamps heading to [-PI..PI] if input is radians, otherwise
  * returns heading in [-180..180]
*/
double UnitConvert :: wrapHeading(double heading, bool isRadians)
{
    if (!isRadians)
    {
        heading = UnitConvert :: toRads(heading);
    }

    heading = heading - std::floor( ( heading + PI ) / TWO_PI) * TWO_PI;

    if (isRadians)
        return  heading;
    else
        return  toDegs(heading);
}

double UnitConvert :: toHeadingDegrees(double radHeading)
{
    double heading = UnitConvert :: wrapHeading(radHeading, true);
    return (  UnitConvert :: toDegs(heading));
}

double UnitConvert :: toHeadingRadians(double degHeading)
{
    double heading = UnitConvert :: wrapHeading(degHeading, false);
    return  UnitConvert :: toRads(heading);
}

double UnitConvert :: NMtoEarthRadians(double distNM) {
    return distNM / 21600.0 * TWO_PI;
}

}
}

#endif
