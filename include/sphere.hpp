#ifndef SPHERE_H
#define SPHERE_H

#include "object3d.hpp"
#include <vecmath.h>
#include <cmath>

using namespace std;

// TODO: Implement functions and add more fields as necessary

class Sphere : public Object3D {
public:
    Sphere() {
        // unit ball at the center
        center = Vector3f(0.0);
        radius = 1;
    }

    Sphere(const Vector3f &center, float radius, Material *material) : Object3D(material) {
        // 
        this->center = center;
        this->radius = radius;
        this->material = material;
    }

    ~Sphere() override = default;

    bool intersect(const Ray &r, Hit &h, float tmin) override {
        Vector3f oc = r.getOrigin() - center;
        Vector3f d = r.getDirection();

        // a*t² + b*t + c = 0
        float a = Vector3f::dot(d, d);
        float b = 2.0f * Vector3f::dot(oc, d);
        float c = Vector3f::dot(oc, oc) - radius * radius;

        float delta = b * b - 4 * a * c;

        if (delta < 0)
            return false;

        float sqrtDelta = sqrt(delta);

        float t0 = (-b - sqrtDelta) / (2 * a);
        float t1 = (-b + sqrtDelta) / (2 * a);

        // 保证 t0 <= t1
        if (t0 > t1)
            std::swap(t0, t1);

        float t;

        // 取最近的合法交点
        if (t0 >= tmin)
            t = t0;
        else if (t1 >= tmin)
            t = t1;
        else
            return false;

        if (t >= h.getT())
            return false;

        Vector3f hitPoint = r.pointAtParameter(t);

        // 几何法线，不要翻转
        Vector3f normal = (hitPoint - center).normalized();

        h.set(t, material, normal, hitPoint);

        return true;
    }

private:
    Vector3f center; //球心
    float radius; //半径
    Material *material; //材质


protected:

};


#endif
