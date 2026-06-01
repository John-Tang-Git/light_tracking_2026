#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "object3d.hpp"
#include <vecmath.h>
#include <cmath>
#include <iostream>
using namespace std;

// TODO: implement this class and add more fields as necessary,
class Triangle: public Object3D {

public:
	Triangle() = delete;

    // a b c are three vertex positions of the triangle
	Triangle( const Vector3f& a, const Vector3f& b, const Vector3f& c, Material* m) : Object3D(m) {
		this->vertices[0] = a;
		this->vertices[1] = b;
		this->vertices[2] = c;
		this->material = m;

		// 补全法线计算
        Vector3f E1 = vertices[0] - vertices[1];
        Vector3f E2 = vertices[0] - vertices[2];
        normal = Vector3f::cross(E1, E2).normalized();
	}

	bool intersect( const Ray& ray,  Hit& hit , float tmin) override {
        Vector3f E1 = vertices[0] - vertices[1];
		Vector3f E2 = vertices[0] - vertices[2];
		Vector3f S = vertices[0] - ray.getOrigin();
		Vector3f Rd = ray.getDirection();

		Matrix3f M0(Rd, E1, E2, false);
		Matrix3f M1(S, E1, E2, false);
		Matrix3f M2(Rd, S, E2, false);
		Matrix3f M3(Rd, E1, S, false);	

		float det0 = M0.determinant();
		float det1 = M1.determinant();
		float det2 = M2.determinant();
		float det3 = M3.determinant();

		float t = det1 / det0;
		float beta = det2 / det0;
		float gamma = det3 / det0;

		if(t>0 && beta+gamma<=1 && 0<=beta && beta<=1 && 0<=gamma && gamma<=1){
			if(t<tmin || t>hit.getT()) return false;
			 // 关键修改：根据射线方向决定法线方向
			Vector3f finalNormal = normal.normalized();
			// 如果射线从背面击中，翻转法线
			if (Vector3f::dot(finalNormal, Rd) > 0) {
				finalNormal = -finalNormal;
			}
			Vector3f hitPoint = ray.pointAtParameter(t);
			hit.set(t, material, finalNormal, hitPoint);
			return true;
		}else{
			return false;
		}
	}
	Vector3f normal;
	Vector3f vertices[3];
	Material *material;
protected:

};

#endif //TRIANGLE_H
