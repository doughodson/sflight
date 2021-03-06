
#ifndef __sflight_mdls_Atmosphere_HPP__
#define __sflight_mdls_Atmosphere_HPP__

#include "sflight/mdls/modules/Module.hpp"

namespace sflight {
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: Atmosphere
// Description: Implements a model of the 1976 standard atmosphere, based on
//              values from 0 to 35,000 meters.
//
// Table Used
// Standard Atmosphere 1976
// Height Temp Press  Density
// m	    C	    hPa    kg/m3
// 0     15	   1013      1.2
// 1000   8.5   900	    1.1
// 2000   2     800	    1
// 3000  -4.5   700	    0.91
// 4000  -11    620	    0.82
// 5000  -17.5  540	    0.74
// 6000	-24    470	    0.66
// 7000	-30.5  410	    0.59
// 8000	-37    360	    0.53
// 9000	-43.5  310	    0.47
// 10000	-50    260	    0.41
// 11000	-56.5	 230	    0.36
// 12000	-56.5	 190	    0.31
// 13000	-56.5	 170	    0.27
// 14000	-56.5	 140	    0.23
// 15000	-56.5	 120	    0.19
// 16000	-56.5	 100	    0.17
// 17000	-56.5	  90	    0.14
// 18000	-56.5	  75	    0.12
// 19000	-56.5	  65	    0.1
// 20000	-56.5	  55	    0.088
// 21000	-55.5	  47	    0.075
// 22000	-54.5	  40	    0.064
// 23000	-53.5	  34	    0.054
// 24000	-52.5	  29	    0.046
// 25000	-51.5	  25	    0.039
// 26000	-50.5	  22	    0.034
// 27000	-49.5	  18	    0.029
// 28000	-48.5	  16	    0.025
// 29000	-47.5	  14	    0.021
// 30000	-46.5	  12	    0.018
// 31000	-45.5	  10	    0.015
// 32000	-44.5	   8.7    0.013
// 33000	-41.7	   7.5    0.011
// 34000	-38.9	   6.5    0.0096
// 35000	-36.1	   5.6    0.0082
//
//------------------------------------------------------------------------------
class Atmosphere : public Module
{
 public:
   Atmosphere(Player*, const double frameRate);

   // module interface
   virtual void update(const double timestep) override;

   // returns density in kg/m^3
   static double getRho(const double alt_meters);

   // returns pressure in kPa
   static double getPressure(const double alt_meters);

   // returns temperature in Kelvin
   static double getTemp(const double alt_meters);

   static double getRemainder(const double alt);

   static int getIndex(const double alt);

   // returns speed of sound in mps
   static double getSpeedSound(const double tempK);

 private:
   static const double alt[37];
   static const double temp[37];
   static const double press[37];
   static const double dens[37];

   static int const maxIndex = 36;
};
}
}

#endif
