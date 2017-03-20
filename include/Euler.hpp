/* Basic euler functionality */

#ifndef EULER
#define EULER


#include "Vector3.hpp"
#include <math.h>

namespace SimpleFlight
{
class Euler : public Vector3
{
public:

    Euler();
    Euler( double psi, double theta, double phi);
    ~Euler();

    void getUVW( Vector3 &uvw, Vector3 &dxdydz);
    void getDxDyDz( Vector3 &dxdydz, Vector3 &uvw);
    void getDeltaEuler( Euler &deltaEuler, double p, double q, double r );
    void getPQR( Vector3 &pqr, Vector3 &eulerDot);
    
    //double a1;
    //double a2;
    //double a3;

    double getPsi()
    {
        return this->a1;
    }

    void setPsi(double psi)
    {
        this->a1 = psi;
    }

    double getTheta()
    {
        return this->a2;
    }

    void setTheta(double theta)
    {
        this->a2 = theta;
    }

    double getPhi()
    {
        return a3;
    }

    void setPhi(double phi)
    {
        this->a3 = phi;
    }

};
} //namespace SimpleFlight
#endif
