/* class that defines a 3-d vector */


#include <math.h>
#include "Vector3.hpp"
#include <iostream>
#include <sstream>

using namespace std;

namespace SimpleFlight
{
Vector3 :: Vector3() {
	this->a1 = 0.0;
	this->a2 = 0.0;
	this->a3 = 0.0;

}

Vector3 :: Vector3( double a1, double a2, double a3) {
	this->a1 = a1;
	this->a2 = a2;
	this->a3 = a3;
}

Vector3 :: ~Vector3() {
}


void Vector3 :: cross(Vector3 &v) {

    double val1 = a2 * v.a3 - a3 * v.a2;
    double val2 = a1 * v.a3 - a3 * v.a1;
    double val3 = a1 * v.a2 - a2 * v.a1;
    this->a1 = val1;
    this->a2 = val2;
    this->a3 = val3;

}

void Vector3 :: cross(Vector3 &ret, Vector3 &v1, Vector3 &v2) {

    ret.set1( v1.a2 * v2.a3 - v1.a3 * v2.a2 );
    ret.set2( v1.a1 * v2.a3 - v1.a3 * v2.a1 );
    ret.set3( v1.a1 * v2.a2 - v1.a2 * v2.a1 );

}

void Vector3 :: multiply(double val) {
    this->a1 *= val;
    this->a2 *= val;
    this->a3 *= val;
}

void Vector3 :: multiply(Vector3 &ret, Vector3 &v, double val) {
    ret.set1( v.a1 * val);
    ret.set2( v.a2 * val);
    ret.set3( v.a3 * val);
}

void Vector3 :: dot(Vector3 &v) {
    this->a1 *= v.a1;
    this->a2 *= v.a2;
    this->a3 *= v.a3;
}

void Vector3 :: dot(Vector3 &ret, Vector3 &v1, Vector3 &v2) {
    ret.set1( v1.a1 * v2.a1);
    ret.set2( v1.a2 * v2.a2);
    ret.set3( v1.a3 * v2.a3);
}

void Vector3 :: add(double val) {
    this->a1 += val;
    this->a2 += val;
    this->a3 += val;
}

void Vector3 :: add(Vector3 &v) {
    this->a1 += v.a1;
    this->a2 += v.a2;
    this->a3 += v.a3;
}

void Vector3 :: add(Vector3 &ret, Vector3 &v1, double val) {
    ret.set1( v1.a1 + val);
    ret.set2( v1.a2 + val);
    ret.set3( v1.a3 + val);
}

void Vector3 :: add(Vector3 &ret, Vector3 &v1, Vector3 &v2) {
    ret.set1( v1.a1 + v2.a1);
    ret.set2( v1.a2 + v2.a2);
    ret.set3( v1.a3 + v2.a3);
}

void Vector3::subtract(double val) {
    this->a1 -= val;
    this->a2 -= val;
    this->a3 -= val;

}

void Vector3 :: subtract(Vector3 &v) {
    this->a1 -= v.a1;
    this->a2 -= v.a2;
    this->a3 -= v.a3;
}

void Vector3 :: subtract(Vector3 &ret, Vector3 &v1, double val) {
    ret.set1( v1.a1 - val);
    ret.set2( v1.a2 - val);
    ret.set3( v1.a3 - val);
}

void Vector3 :: subtract(Vector3 &ret, Vector3 &v1, Vector3 &v2) {
    ret.set1( v1.a1 - v2.a1);
    ret.set2( v1.a2 - v2.a2);
    ret.set3( v1.a3 - v2.a3);
}


double Vector3 :: magnitude() {
    return sqrt( a1 * a1 + a2 * a2 + a3 * a3 );
}

void Vector3 :: reciprocal( Vector3 &ret) {
    ret.set1( 1./a1);
    ret.set2( 1./a2);
    ret.set3( 1./a3);
}

void Vector3 :: print() {

    cout << "Vector3: " << get1() << ", " << get2() << ", " << get3() << endl;

}

string Vector3 :: toString() {
    ostringstream oss;
    oss << "Vector3: [ " << get1() << ", " << get2() << ", " << get3() << " ]";
    return oss.str();
}



}//namespace SimpleFlight