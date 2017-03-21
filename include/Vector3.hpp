
#ifndef VECTOR3_H
#define VECTOR3_H

#include <string>

namespace sf
{

//------------------------------------------------------------------------------
// Class: Vector3
// Description: Defines a 3-D vector
//------------------------------------------------------------------------------
class Vector3
{
 public:
   Vector3();
   Vector3(double a1, double a2, double a3);
   virtual ~Vector3();

   double a1 {};
   double a2 {};
   double a3 {};

   double get1() const { return a1; }
   double get2() const { return a2; }
   double get3() const { return a3; }

   void set1(const double x) { a1 = x; }
   void set2(const double x) { a2 = x; }
   void set3(const double x) { a3 = x; }

   void cross(Vector3 &v);
   static void cross(Vector3 &ret, Vector3 &v1, Vector3 &v2);

   void multiply(double val);
   static void multiply(Vector3 &ret, Vector3 &v, double val);

   void dot(Vector3 &v);
   static void dot(Vector3 &ret, Vector3 &v1, Vector3 &v2);

   void add(double val);
   void add(Vector3 &v);
   static void add(Vector3 &ret, Vector3 &v1, Vector3 &v2);
   static void add(Vector3 &ret, Vector3 &v1, double val);

   void subtract(double val);
   void subtract(Vector3 &v);
   static void subtract(Vector3 &ret, Vector3 &v1, Vector3 &v2);
   static void subtract(Vector3 &ret, Vector3 &v1, double val);

   double magnitude();
   void reciprocal(Vector3 &ret);

   void print();
   std::string toString();
};

}

#endif
