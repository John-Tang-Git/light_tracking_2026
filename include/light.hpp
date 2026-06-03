#ifndef LIGHT_H
#define LIGHT_H

#include <Vector3f.h>
#include "object3d.hpp"

class Light {
public:
    Light() = default;

    virtual ~Light() = default;

    virtual void getIllumination(const Vector3f &p, Vector3f &dir, Vector3f &col) const = 0;
};


class DirectionalLight : public Light {
public:
    DirectionalLight() = delete;

    DirectionalLight(const Vector3f &d, const Vector3f &c) {
        direction = d.normalized();
        color = c;
    }

    ~DirectionalLight() override = default;

    ///@param p unsed in this function
    ///@param distanceToLight not well defined because it's not a point light
    void getIllumination(const Vector3f &p, Vector3f &dir, Vector3f &col) const override {
        // the direction to the light is the opposite of the
        // direction of the directional light source
        dir = -direction;
        col = color;
    }

private:

    Vector3f direction;
    Vector3f color;

};

class PointLight : public Light {
public:
    PointLight() = delete;

    PointLight(const Vector3f &p, const Vector3f &c) {
        position = p;
        color = c;
    }

    ~PointLight() override = default;

    void getIllumination(const Vector3f &p, Vector3f &dir, Vector3f &col) const override {
        // the direction to the light is the opposite of the
        // direction of the directional light source
        dir = (position - p);
        dir = dir / dir.length();
        col = color;
    }

    Vector3f &getPosition() {
        return position;
    }

private:

    Vector3f position;
    Vector3f color;

};


// 面光源
class AreaLight : public Light {
public:
    AreaLight() = delete;
    
    // 矩形面光源：中心点 + 两条边向量
    AreaLight(const Vector3f& c, const Vector3f& u, const Vector3f& v, const Vector3f& col) 
        : center(c), u_axis(u), v_axis(v), color(col) {
        area = Vector3f::cross(u, v).length();
        normal = Vector3f::cross(u, v).normalized();
    }
    
    ~AreaLight() override = default;
    
    void getIllumination(const Vector3f &p, Vector3f &dir, Vector3f &col) const override {
        // 在光源表面随机采样一点
        float su = (float)rand() / RAND_MAX;
        float sv = (float)rand() / RAND_MAX;
        Vector3f samplePoint = center + u_axis * su + v_axis * sv;
        
        dir = samplePoint - p;
        float dist2 = dir.squaredLength();
        dir = dir.normalized();
        
        // 计算几何衰减（避免 cos 项为负）
        float cosTheta = std::max(0.0f, Vector3f::dot(-dir, normal));
        col = color * cosTheta / (dist2 + 1e-4f);  // 加小量避免除零
    }
    
    // 用于 NEE：直接采样光源上的点
    Vector3f samplePoint(float& pdf) const {
        float su = (float)rand() / RAND_MAX;
        float sv = (float)rand() / RAND_MAX;
        pdf = 1.0f / area;
        return center + u_axis * su + v_axis * sv;
    }
    
    float getArea() const { return area; }
    Vector3f getNormal() const { return normal; }
    Vector3f getColor() const { return color; }

private:
    Vector3f center;
    Vector3f u_axis, v_axis;
    Vector3f normal;
    Vector3f color;
    float area;
};

#endif // LIGHT_H
