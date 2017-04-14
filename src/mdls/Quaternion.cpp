
#include "sflight/mdls/Quaternion.hpp"
#include "sflight/mdls/Euler.hpp"

#include <iostream>
#include <cmath>

namespace sflight {
namespace mdls {

Quaternion::Quaternion()
{
   initialize(0.0, 0.0, 0.0);
}

Quaternion::Quaternion(const double psi, const double theta, const double phi)
{
   initialize(psi, theta, phi);
}

Quaternion::Quaternion(const double eo, const double ex, const double ey, const double ez)
{
   this->eo = eo;
   this->ex = ex;
   this->ey = ey;
   this->ez = ez;
}

Quaternion::Quaternion(Euler &euler)
{
   initialize(euler.getPsi(), euler.getTheta(), euler.getPhi());
}

Quaternion::~Quaternion()
{
}

double Quaternion::getEo() const
{
   return this->eo;
}
double Quaternion::getEx() const
{
   return this->ex;
}
double Quaternion::getEy() const
{
   return this->ey;
}
double Quaternion::getEz() const
{
   return this->ez;
}

double Quaternion::getPsi()
{
   const double m11 = eo * eo + ex * ex - ey * ey - ez * ez;
   return std::atan2(2.0 * (eo * ez + ex * ey), m11);
}

double Quaternion::getTheta()
{
   return std::asin(2 * (eo * ey - ex * ez));
}

double Quaternion::getPhi()
{
   const double m33 = eo * eo + ez * ez - ex * ex - ey * ey;
   return std::atan2(2.0 * (eo * ex + ey * ez), m33);
}

void Quaternion::getEulers(Euler &euler)
{
   euler.setPsi(getPsi());
   euler.setTheta(getTheta());
   euler.setPhi(getPhi());
}

/** sets the value of world axis velocities (dxdydz) based on the passed body-axis (uvw) velocities */
void Quaternion::getDxDyDz(Vector3 &dxdydz, Vector3 &uvw)
{
   const double a11 = ex * ex + eo * eo - ey * ey - ez * ez;
   const double a12 = 2.0 * (ex * ey - ez * eo);
   const double a13 = 2.0 * (ex * ez + ey * eo);
   const double a21 = 2.0 * (ex * ey + ez * eo);
   const double a22 = ey * ey + eo * eo - ex * ex - ez * ez;
   const double a23 = 2.0 * (ey * ez - ex * eo);
   const double a31 = 2.0 * (ex * ez - ey * eo);
   const double a32 = 2.0 * (ey * ez + ex * eo);
   const double a33 = ez * ez + eo * eo - ex * ex - ey * ey;

   dxdydz.set1(a11 * uvw.get1() + a12 * uvw.get2() + a13 * uvw.get3());
   dxdydz.set2(a21 * uvw.get1() + a22 * uvw.get2() + a23 * uvw.get3());
   dxdydz.set3(a31 * uvw.get1() + a32 * uvw.get2() + a33 * uvw.get3());
}

/** sets the value of body axis velocities (uvw) based on the passed world axis velocities (dxdydz) */
void Quaternion::getUVW(Vector3 &uvw, Vector3 &dxdydz)
{
   const double a11 = ex * ex + eo * eo - ey * ey - ez * ez;
   const double a21 = 2.0 * (ex * ey - ez * eo);
   const double a31 = 2.0 * (ex * ez + ey * eo);
   const double a12 = 2.0 * (ex * ey + ez * eo);
   const double a22 = ey * ey + eo * eo - ex * ex - ez * ez;
   const double a32 = 2.0 * (ey * ez - ex * eo);
   const double a13 = 2.0 * (ex * ez - ey * eo);
   const double a23 = 2.0 * (ey * ez + ex * eo);
   const double a33 = ez * ez + eo * eo - ex * ex - ey * ey;

   uvw.set1(a11 * dxdydz.get1() + a12 * dxdydz.get2() + a13 * dxdydz.get3());
   uvw.set2(a21 * dxdydz.get1() + a22 * dxdydz.get2() + a23 * dxdydz.get3());
   uvw.set3(a31 * dxdydz.get1() + a32 * dxdydz.get2() + a33 * dxdydz.get3());
}

void Quaternion::getQdot(Quaternion &toFill, double p, double q, double r)
{

   toFill.eo = 0.5 * (-p * ex - q * ey - r * ez);
   toFill.ex = 0.5 * (p * eo + r * ey - q * ez);
   toFill.ey = 0.5 * (q * eo - r * ex + p * ez);
   toFill.ez = 0.5 * (r * eo + q * ex - p * ey);
}

void Quaternion::initialize(Euler &euler)
{
   initialize(euler.getPsi(), euler.getTheta(), euler.getPhi());
}

void Quaternion::initialize(double psi, double theta, double phi)
{
   const double c_psi = cos(psi / 2.0);
   const double s_psi = sin(psi / 2.0);
   const double c_theta = cos(theta / 2.0);
   const double s_theta = sin(theta / 2.0);
   const double c_phi = cos(phi / 2.0);
   const double s_phi = sin(phi / 2.0);

   eo = c_phi * c_theta * c_psi + s_phi * s_theta * s_psi;
   ex = s_phi * c_theta * c_psi - c_phi * s_theta * s_psi;
   ey = c_phi * s_theta * c_psi + s_phi * c_theta * s_psi;
   ez = c_phi * c_theta * s_psi - s_phi * s_theta * c_psi;
}

void Quaternion::normalize()
{
   const double norm = std::sqrt(eo * eo + ex * ex + ey * ey + ez * ez);
   if (norm > 0)
   {
      eo /= norm;
      ex /= norm;
      ey /= norm;
      ez /= norm;
   }
}

void Quaternion::add(Quaternion q)
{
   eo += q.getEo();
   ex += q.getEx();
   ey += q.getEy();
   ez += q.getEz();

   normalize();
}

void Quaternion::getConjugate(Quaternion q)
{
   q.eo = eo;
   q.ex = -ex;
   q.ey = -ey;
   q.ez = -ez;
}

void Quaternion::multiply(Quaternion q)
{
   const double qeo = q.getEo();
   const double qex = q.getEx();
   const double qey = q.getEy();
   const double qez = q.getEz();

   const double tmp_eo = eo * qeo - ex * qex - ey * qey - ez * qez;
   const double tmp_ex = eo * qex + ex * qeo + ey * qez - ez * qey;
   const double tmp_ey = eo * qey - ex * qez + ey * qeo + ez * qex;
   const double tmp_ez = eo * qez + ex * qey - ey * qex + ez * qeo;

   eo = tmp_eo;
   ex = tmp_ex;
   ey = tmp_ey;
   ez = tmp_ez;
}

void Quaternion::multiply(double d)
{
   eo *= d;
   ex *= d;
   ey *= d;
   ez *= d;
}

void Quaternion::print()
{
   std::cout << "eo: " << eo << " ex: " << ex << " ey: " << ey << " ez: " << ez << std::endl;
}
}
}
