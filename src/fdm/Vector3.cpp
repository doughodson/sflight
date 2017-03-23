
#include "sflight/fdm/Vector3.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

namespace sflight {
namespace fdm {

Vector3::Vector3(const double a1, const double a2, const double a3)
    : a1(a1), a2(a2), a3(a3)
{
}

void Vector3::cross(Vector3& v)
{
   const double val1 = a2 * v.a3 - a3 * v.a2;
   const double val2 = a1 * v.a3 - a3 * v.a1;
   const double val3 = a1 * v.a2 - a2 * v.a1;
   a1 = val1;
   a2 = val2;
   a3 = val3;
}

void Vector3::cross(Vector3& ret, Vector3& v1, Vector3& v2)
{
   ret.set1(v1.a2 * v2.a3 - v1.a3 * v2.a2);
   ret.set2(v1.a1 * v2.a3 - v1.a3 * v2.a1);
   ret.set3(v1.a1 * v2.a2 - v1.a2 * v2.a1);
}

void Vector3::multiply(const double val)
{
   this->a1 *= val;
   this->a2 *= val;
   this->a3 *= val;
}

void Vector3::multiply(Vector3& ret, Vector3& v, double val)
{
   ret.set1(v.a1 * val);
   ret.set2(v.a2 * val);
   ret.set3(v.a3 * val);
}

void Vector3::dot(Vector3& v)
{
   this->a1 *= v.a1;
   this->a2 *= v.a2;
   this->a3 *= v.a3;
}

void Vector3::dot(Vector3& ret, Vector3& v1, Vector3& v2)
{
   ret.set1(v1.a1 * v2.a1);
   ret.set2(v1.a2 * v2.a2);
   ret.set3(v1.a3 * v2.a3);
}

void Vector3::add(const double val)
{
   this->a1 += val;
   this->a2 += val;
   this->a3 += val;
}

void Vector3::add(Vector3& v)
{
   this->a1 += v.a1;
   this->a2 += v.a2;
   this->a3 += v.a3;
}

void Vector3::add(Vector3& ret, Vector3& v1, double val)
{
   ret.set1(v1.a1 + val);
   ret.set2(v1.a2 + val);
   ret.set3(v1.a3 + val);
}

void Vector3::add(Vector3& ret, Vector3& v1, Vector3& v2)
{
   ret.set1(v1.a1 + v2.a1);
   ret.set2(v1.a2 + v2.a2);
   ret.set3(v1.a3 + v2.a3);
}

void Vector3::subtract(const double val)
{
   this->a1 -= val;
   this->a2 -= val;
   this->a3 -= val;
}

void Vector3::subtract(Vector3& v)
{
   this->a1 -= v.a1;
   this->a2 -= v.a2;
   this->a3 -= v.a3;
}

void Vector3::subtract(Vector3& ret, Vector3& v1, double val)
{
   ret.set1(v1.a1 - val);
   ret.set2(v1.a2 - val);
   ret.set3(v1.a3 - val);
}

void Vector3::subtract(Vector3& ret, Vector3& v1, Vector3& v2)
{
   ret.set1(v1.a1 - v2.a1);
   ret.set2(v1.a2 - v2.a2);
   ret.set3(v1.a3 - v2.a3);
}

double Vector3::magnitude() { return sqrt(a1 * a1 + a2 * a2 + a3 * a3); }

void Vector3::reciprocal(Vector3& ret)
{
   ret.set1(1. / a1);
   ret.set2(1. / a2);
   ret.set3(1. / a3);
}

void Vector3::print()
{
   std::cout << "Vector3: " << get1() << ", " << get2() << ", " << get3()
             << std::endl;
}

std::string Vector3::toString() const
{
   std::ostringstream oss;
   oss << "Vector3: [ " << get1() << ", " << get2() << ", " << get3() << " ]";
   return oss.str();
}
}
}
