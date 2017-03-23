
#include "sflt/fdm/modules/Atmosphere.hpp"

#include "sflt/fdm/FDMGlobals.hpp"

#include <cmath>

namespace sflt {
namespace fdm {

const double Atmosphere::alt[] = {
    0,     1000,  2000,  3000,  4000,  5000,  6000,   7000,  8000,  9000,
    10000, 11000, 12000, 13000, 14000, 15000, 16000,  17000, 18000, 19000,
    20000, 21000, 22000, 23000, 24000, 25000, 26000,  27000, 28000, 29000,
    30000, 31000, 32000, 33000, 34000, 35000, 1000000};

const double Atmosphere::temp[] = {
    15,    8.5,   2,     -4.5,  -11,   -17.5, -24,          -30.5, -37,
    -43.5, -50,   -56.5, -56.5, -56.5, -56.5, -56.5,        -56.5, -56.5,
    -56.5, -56.5, -56.5, -55.5, -54.5, -53.5, -52.5 - 51.5, -50.5, -49.5,
    -48.5, -47.5, -46.5, -45.5, -44.5, -41.7, -38.9,        -36.1, -250.0};
const double Atmosphere::press[] = {
    1013, 900, 800, 700, 620, 540, 470, 410, 360, 310, 260, 230, 190,
    170,  140, 120, 100, 90,  75,  65,  55,  47,  40,  34,  29,  25,
    22,   18,  16,  14,  12,  10,  8.7, 7.5, 6.5, 5.6, 1E-6};
const double Atmosphere::dens[] = {
    1.2,   1.1,   1.0,   0.91,  0.82,   0.74,   0.66,  0.59,  0.53,  0.47,
    0.41,  0.36,  0.31,  0.27,  0.23,   0.19,   0.17,  0.14,  0.12,  0.1,
    0.088, 0.075, 0.064, 0.054, 0.046,  0.039,  0.034, 0.029, 0.025, 0.021,
    0.018, 0.015, 0.013, 0.011, 0.0096, 0.0082, 1E-6};

Atmosphere::Atmosphere(FDMGlobals* globals, double frameRate)
    : FDMModule(globals, frameRate)
{
}

void Atmosphere::update(const double timestep)
{
   globals->rho = getRho(globals->alt);
}

int Atmosphere::getIndex(double metersAlt)
{
   if (metersAlt < 0)
      return 0;

   int index = 0;
   for (index = 1; index < maxIndex - 1; index++) {
      if (alt[index] > metersAlt)
         return index - 1;
   }
   return index - 1;
}

double Atmosphere::getRemainder(double alt)
{
   return alt / 1000. - std::floor(alt / 1000.);
}

// returns density in kg/m^3
double Atmosphere::getRho(double alt_meters)
{
   const int index = getIndex(alt_meters);

   if (index >= 0) {
      return dens[index] +
             (dens[index + 1] - dens[index]) * getRemainder(alt_meters);
   } else {
      return dens[0];
   }
}

// returns pressure in kPa
double Atmosphere::getPressure(double alt_meters)
{
   const int index = getIndex(alt_meters);

   if (index >= 0) {
      return press[index] +
             (press[index + 1] - press[index]) * getRemainder(alt_meters);
   } else {
      return press[0];
   }
}

// returns temperature in Kelvin
double Atmosphere::getTemp(double alt_meters)
{
   const int index = getIndex(alt_meters);

   if (index >= 0) {
      return temp[index] +
             (temp[index + 1] - temp[index]) * getRemainder(alt_meters) +
             273.15;
   }
   return temp[0] + 273.15;
}

// returns speed of sound in mps
double Atmosphere::getSpeedSound(double tempK)
{
   return std::sqrt(1.4 * 287.01 * tempK);
}
}
}
