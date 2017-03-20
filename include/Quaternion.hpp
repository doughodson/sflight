/* Implements quaternion math */

#ifndef QUATERNION
#define QUATERNION

namespace SimpleFlight
{
class Euler;
class Vector3;

class Quaternion
{

public:
    double getEo();
    double getEx();
    double getEy();
    double getEz();

    Quaternion();
    Quaternion(double psi, double theta, double phi);
    Quaternion(double eo, double ex, double ey, double ez);
    Quaternion(Euler &euler);
    ~Quaternion();

    double getPsi();
    double getTheta();
    double getPhi();

    void getEulers( Euler &euler );
    void getDxDyDz( Vector3 &dxdydz, Vector3 &uvw);
    void getUVW( Vector3 &uvw,  Vector3 &dxdydz);
    void getQdot( Quaternion& toFill, double p, double q, double r );

    void initialize( Euler &euler);
    void initialize( double psi, double theta, double phi);
    void normalize();
    void add( Quaternion q);

    void getConjugate( Quaternion q);
    void multiply( Quaternion q);
    void multiply( double d );

    void print();

protected:
    double eo;
    double ex;
    double ey;
    double ez;

};

}//namespace SimpleFlight
#endif




