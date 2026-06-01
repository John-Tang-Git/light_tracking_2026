#ifndef HIT_H
#define HIT_H

#include <vecmath.h>
#include "ray.hpp"

class Material;

class Hit {
public:

    // constructors
    Hit() {
        material = nullptr;
        t = 1e38;
        intersectionPoint = Vector3f::ZERO;
        normal = Vector3f::ZERO;
    }

    Hit(float _t, Material *m, const Vector3f &n, const Vector3f &p) {
        t = _t;
        material = m;
        normal = n;
        intersectionPoint = p;
    }

    Hit(const Hit &h) {
        t = h.t;
        material = h.material;
        normal = h.normal;
        intersectionPoint = h.intersectionPoint;
    }

    // destructor
    ~Hit() = default;

    float getT() const {
        return t;
    }

    Material *getMaterial() const {
        return material;
    }

    const Vector3f &getNormal() const {
        return normal;
    }

    const Vector3f &getIntersectionPoint() const {
        return intersectionPoint;
    }

    void set(float _t, Material *m, const Vector3f &n, const Vector3f &p) {
        t = _t;
        material = m;
        normal = n;
        intersectionPoint = p;
    }

private:
    float t;
    Material *material;
    Vector3f normal;
    Vector3f intersectionPoint;

};

inline std::ostream &operator<<(std::ostream &os, const Hit &h) {
    os << "Hit <" << h.getT() << ", " << h.getNormal() << ", " << h.getIntersectionPoint() << ">";
    return os;
}

#endif // HIT_H
