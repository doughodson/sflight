
#ifndef __sflight_mdls_UnitConvert_H__
#define __sflight_mdls_UnitConvert_H__

#include "sflight/mdls/constants.hpp"

#include <cmath>

namespace sflight {
namespace mdls {

//------------------------------------------------------------------------------
// basic conversions used in the flight model
//------------------------------------------------------------------------------
class UnitConvert
{
 public:
   static inline double toDegs(const double rad);
   static inline double toRads(const double deg);
   static inline double toMeters(const double feet);
   static inline double toSqMeters(const double sqFeet);
   static inline double toFeet(const double meters);
   static inline double toSqFeet(const double sqMeters);
   static inline double toMPS(const double knots);
   static inline double toKnots(const double metersPerSecond);
   static inline double toKilos(const double lbs);
   static inline double toLbs(const double kilos);
   static inline double toLbsForce(const double newtons);
   static inline double toNewtons(const double lbs);
   static inline double toFPM(const double metersPerSecond);
   static inline double FPMtoMPS(const double feetPerMinute);
   static inline double wrapHeading(double heading, bool isRadians);
   static inline double toHeadingDegrees(const double radHeading);
   static inline double toHeadingRadians(const double degHeading);
   static inline double NMtoEarthRadians(const double distNM);
   static inline double signum(const double input);
};

double UnitConvert::toDegs(const double rad) { return rad * 180.0 / math::PI; }

double UnitConvert::toRads(const double deg) { return deg / 180.0 * math::PI; }

double UnitConvert::toMeters(const double feet) { return feet * 0.3048; }

double UnitConvert::toSqMeters(const double sqFeet) { return sqFeet * 0.3048 * 0.3048; }

double UnitConvert::toFeet(const double meters) { return meters / 0.3048; }

double UnitConvert::toSqFeet(const double sqMeters) { return sqMeters / 0.3048 / 0.3048; }

/** converts knots to Meters per second */
double UnitConvert::toMPS(const double knots) { return knots / 1.9438445; }

double UnitConvert::toKnots(const double metersPerSecond) { return metersPerSecond * 1.9438445; }

double UnitConvert::toKilos(const double lbs) { return lbs / 2.2; }

double UnitConvert::toLbs(const double kilos) { return kilos * 2.2; }

double UnitConvert::toLbsForce(const double newtons) { return toLbs(newtons) / 9.8; }

double UnitConvert::toNewtons(const double lbs) { return toKilos(lbs) * 9.8; }

double UnitConvert::toFPM(const double metersPerSecond) { return toFeet(metersPerSecond) * 60.0; }

double UnitConvert::FPMtoMPS(const double feetPerMinute) { return toMeters(feetPerMinute) / 60.0; }

/** returns the sign of the number */
double UnitConvert::signum(const double input) { return input >= 0.0 ? 1.0 : -1.0; }

/** clamps heading to [-PI..PI] if input is radians, otherwise
  * returns heading in [-180..180]
*/
double UnitConvert::wrapHeading(double heading, const bool isRadians)
{
   if (!isRadians) {
      heading = UnitConvert::toRads(heading);
   }

   heading = heading - std::floor((heading + math::PI) / math::TWO_PI) * math::TWO_PI;

   if (isRadians)
      return heading;
   else
      return toDegs(heading);
}

double UnitConvert::toHeadingDegrees(const double radHeading)
{
   const double heading = UnitConvert::wrapHeading(radHeading, true);
   return (UnitConvert::toDegs(heading));
}

double UnitConvert::toHeadingRadians(const double degHeading)
{
   const double heading = UnitConvert::wrapHeading(degHeading, false);
   return UnitConvert::toRads(heading);
}

double UnitConvert::NMtoEarthRadians(const double distNM) { return distNM / 21600.0 * math::TWO_PI; }
}
}

#endif
