#include <cmath>

#include "Sphere.h"

using namespace std;


//bool Sphere::intersectLocal(const ray& r, isect &i) const
//{
//
//}


bool Sphere::intersectLocal( const ray& r, isect& i ) const
{
    // YOUR CODE HERE:
    // Add sphere intersection code here.
    // it currently ignores all spheres and just return false.
	Vec3d P = r.getPosition();
	Vec3d D = r.getDirection();
	//D.normalize();
	//double a = dot(D, D);
	double a = D * D;
	double b = 2 * (P *D);
	//double b = 2 * dot(P, D);
	double c = (P * P) - 1;
	//double c = dot(P, P) - 1;
	double delta = b * b - 4 * a * c;
	if (delta < 0)
		return false;
	if (delta == 0) {
		double t = -b / 2 * a;
		Vec3d Q = P + t * D;
		Vec3d N = Q;
		N.normalize();
		i.setT(t);
		i.setN(N);
		i.setObject(this);
		return true;
	}
	if (delta > 0) {
		double t1 = (-b - sqrt(delta)) / 2 * a;
		double t2 = (-b + sqrt(delta)) / 2 * a;
		double t;
		if (t1 > 0) t = t1;
		else if (t2 > 0) t = t2;
		else return false;
		Vec3d N = P + t * D;
		N.normalize();
		i.setT(t);
		i.setN(N);
		i.setObject(this);
		return true;
	}
	return false;
}

