
#ifndef __sf_fdm_Quaternion_H__
#define __sf_fdm_Quaternion_H__

namespace sf {
namespace fdm {
class Euler;
class Vector3;

//------------------------------------------------------------------------------
// Class: Quaternion
//------------------------------------------------------------------------------
class Quaternion
{
 public:
   double getEo() const;
   double getEx() const;
   double getEy() const;
   double getEz() const;

   Quaternion();
   Quaternion(const double psi, const double theta, const double phi);
   Quaternion(const double eo, const double ex, const double ey, const double ez);
   Quaternion(Euler &euler);
   virtual ~Quaternion();

   double getPsi();
   double getTheta();
   double getPhi();

   void getEulers(Euler &euler);
   void getDxDyDz(Vector3 &dxdydz, Vector3 &uvw);
   void getUVW(Vector3 &uvw, Vector3 &dxdydz);
   void getQdot(Quaternion &toFill, double p, double q, double r);

   void initialize(Euler &euler);
   void initialize(double psi, double theta, double phi);
   void normalize();
   void add(Quaternion q);

   void getConjugate(Quaternion q);
   void multiply(Quaternion q);
   void multiply(double d);

   void print();

 protected:
   double eo {};
   double ex {};
   double ey {};
   double ez {};
};

}
}

#endif
