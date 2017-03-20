
#ifndef WINDAXIS_H
#define WINDAXIS_H

namespace sf
{
class Vector3;

class WindAxis
{
public:
    static void bodyToWind( Vector3 &ret, double alpha, double beta, double fx, double fy, double fz);
    static void windToBody( Vector3 &ret, double alpha, double beta, double lift, double drag, double sideforce ) ;
};

}

#endif
