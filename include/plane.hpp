#ifndef PLANE_H
#define PLANE_H

#include "object3d.hpp"
#include <vecmath.h>
#include <cmath>

// TODO: Implement Plane representing an infinite plane
// function: ax+by+cz=d
// choose your representation , add more fields and fill in the functions

class Plane : public Object3D {
public:
    Plane() {
        d = 0;
        normal = Vector3f(0, 0, 1); //xy平面
    }

    Plane(const Vector3f &normal, float d, Material *m) : Object3D(m) {
        this->normal = normal.normalized();
        this->d = d;
        this->material = m;
    }

    ~Plane() override = default;

    bool intersect(const Ray &r, Hit &h, float tmin) override {
        //先判断是否平行
        float denom = Vector3f::dot(normal, r.getDirection());
        //if (fabs(denom) < 1e-6) return false;  // 视为平行

        //不平行
        Vector3f dir = r.getDirection().normalized();
        float t = (d - Vector3f::dot(normal, r.getOrigin())) / Vector3f::dot(normal, dir);
        if(t<tmin || t>h.getT()) return false; //没完成目标

        //能完成更新
        h.set(t, material, normal, r.pointAtParameter(t));
        return true;
    }

private:
    Vector3f normal;
    float d;
    Material *material; 
    
protected:


};

#endif //PLANE_H
		

