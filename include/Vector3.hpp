/* class that defines a 3-d vector */

#ifndef VECTOR3_H
#define VECTOR3_H

#include <string>

using std::string;

namespace SimpleFlight
{
class Vector3
{

public:
	double a1;
	double a2;
	double a3;

	double get1() const { return this->a1; }
	double get2() const { return this->a2; }
	double get3() const { return this->a3; }

	void set1( double val) { this->a1 = val; }
	void set2( double val) { this->a2 = val; }
	void set3( double val) { this->a3 = val; }

	Vector3();
	Vector3(double a1, double a2, double a3);
	~Vector3();

	void cross( Vector3 &v);
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
	string toString();


};


}//namespace SimpleFlight
#endif

