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
        //
        float distance; //测量出的球心到射线距离
        Vector3f l(center.x()-r.getOrigin().x(), center.y()-r.getOrigin().y(), center.z()-r.getOrigin().z());
        Vector3f dir = r.getDirection().normalized();
        float tp = Vector3f::dot(l, dir);

        if (tp<0) return false; //默认在球体外面，而且发射光向量和l夹角大于90度，没有交点

        float d_2 = l.squaredLength() - tp*tp;
        if (d_2 > radius*radius) return false; //不相交

        //确认相交
        float t_2 = radius*radius - d_2;
        float t_1 = sqrt(t_2);
        float t = tp - t_1;
        if(t>h.getT() || t<tmin) return false; //能相交，但是不改变什么

        //确认相交且需要更新相交数据
        Vector3f hitPoint = r.getOrigin() + t * dir;
        Vector3f normal((hitPoint - center).normalized());
        Vector3f finalNormal = normal;
        if (Vector3f::dot(finalNormal, dir) > 0) {
            finalNormal = -finalNormal;  // 让法线指向射线来的方向
        }
        h.set(t, material, finalNormal);

        return true;
    }

private:
    Vector3f center; //球心
    float radius; //半径
    Material *material; //材质


protected:

};


#endif
