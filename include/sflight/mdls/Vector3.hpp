
#ifndef __sflight_mdls_Vector3_H__
#define __sflight_mdls_Vector3_H__

#include <string>

namespace sflight {
namespace mdls {

//------------------------------------------------------------------------------
// Class: Vector3
// Description: Defines a 3-D vector
//------------------------------------------------------------------------------
class Vector3 {
 public:
   Vector3() = default;
   Vector3(const double a1, const double a2, const double a3);
   virtual ~Vector3() = default;

   double a1{};
   double a2{};
   double a3{};

   double get1() const { return a1; }
   double get2() const { return a2; }
   double get3() const { return a3; }

   void set1(const double x) { a1 = x; }
   void set2(const double x) { a2 = x; }
   void set3(const double x) { a3 = x; }

   void cross(Vector3& v);
   static void cross(Vector3& ret, Vector3& v1, Vector3& v2);

   void multiply(const double val);
   static void multiply(Vector3& ret, Vector3& v, double val);

   void dot(Vector3& v);
   static void dot(Vector3& ret, Vector3& v1, Vector3& v2);

   void add(const double val);
   void add(Vector3& v);
   static void add(Vector3& ret, Vector3& v1, Vector3& v2);
   static void add(Vector3& ret, Vector3& v1, double val);

   void subtract(const double val);
   void subtract(Vector3& v);
   static void subtract(Vector3& ret, Vector3& v1, Vector3& v2);
   static void subtract(Vector3& ret, Vector3& v1, double val);

   double magnitude();
   void reciprocal(Vector3& ret);

   void print();
   std::string toString() const;
};
}
}

#endif
