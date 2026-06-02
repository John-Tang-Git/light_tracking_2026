#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <vecmath.h>

#include "ray.hpp"
#include "hit.hpp"
#include <iostream>

// TODO: Implement Shade function that computes Phong introduced in class.
class Material {
public:

    explicit Material(const Vector3f &d_color, const Vector3f &s_color = Vector3f::ZERO, float s = 0, 
            const Vector3f &r_color = Vector3f::ZERO, const Vector3f &t_color = Vector3f::ZERO, float ior = 1.0f) : 
            diffuseColor(d_color), specularColor(s_color), shininess(s), reflectiveColor(r_color), transmissiveColor(t_color), ior(ior) {

    }

    virtual ~Material() = default;

    virtual Vector3f getDiffuseColor() const {
        return diffuseColor;
    }

    virtual Vector3f getSpecularColor() const {
        return specularColor;
    }

    virtual float getShininess() const {
        return shininess;
    }

    virtual Vector3f getReflectiveColor() const {
        return reflectiveColor;
    }

    virtual Vector3f getTransmissiveColor() const {
        return transmissiveColor;
    }

    virtual float getIor() const {
        return ior;
    }

    float clamp(float num){
        return num>0 ? num : 0;
    }

    Vector3f Shade(const Ray &ray, const Hit &hit,
                const Vector3f &dirToLight, const Vector3f &lightColor) {
        Vector3f shaded = Vector3f::ZERO;
        Vector3f N = hit.getNormal().normalized();
        Vector3f L = dirToLight.normalized();
        Vector3f V = -ray.getDirection().normalized();
        // 标准Phong反射向量R公式不变
        Vector3f R = 2.0f * Vector3f::dot(N, L) * N - L;
        R.normalize();

        // 漫反射：N·L负数直接置0，背光无漫反射
        float NdotL = std::max(Vector3f::dot(N, L), 0.0f);
        Vector3f Diffuse = diffuseColor * NdotL;

        // 高光：背光(NdotL<=0)直接高光归零；R·V负数也归零，避免pow负数异常
        Vector3f Specular = Vector3f::ZERO;
        if(NdotL > 1e-6f)
        {
            float RdotV = std::max(Vector3f::dot(R, V), 0.0f);
            Specular = specularColor * pow(RdotV, shininess);
        }

        shaded += lightColor * (Diffuse + Specular);
        return shaded;
    }

protected:
    Vector3f diffuseColor;
    Vector3f specularColor;
    float shininess;
    Vector3f reflectiveColor; // 反射系数
    Vector3f transmissiveColor; // 折射系数
    float ior; // 折射率
};


#endif // MATERIAL_H
