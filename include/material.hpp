#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <vecmath.h>

#include "ray.hpp"
#include "hit.hpp"
#include <iostream>

// Material class for Path Tracing
class Material {
public:

    explicit Material(const Vector3f &d_color, const Vector3f &s_color = Vector3f::ZERO, float s = 0, 
            const Vector3f &r_color = Vector3f::ZERO, const Vector3f &t_color = Vector3f::ZERO, float ior = 1.0f,
            const Vector3f &em = Vector3f::ZERO, float roughness = 0.5f) : 
            diffuseColor(d_color), specularColor(s_color), shininess(s), 
            reflectiveColor(r_color), transmissiveColor(t_color), ior(ior), emission(em), roughness(roughness) {

    }

    virtual ~Material() = default;

    // Basic getters
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

    virtual Vector3f getEmission() const {
        return emission;
    }
    
    virtual float getRoughness() const {
        return roughness;
    }
    
    virtual void setRoughness(float r) {
        roughness = r;
    }

    // 判断材质类型
    virtual bool isLight() const {
        return emission.length() > 1e-3;
    }
    
    virtual bool isGlossy() const {
        return reflectiveColor.length() > 1e-3 && roughness > 0.01f && roughness < 0.99f;
    }
    
    virtual bool isPerfectReflect() const {
        return reflectiveColor.length() > 1e-3 && roughness <= 0.01f;
    }

    // For Whitted-Style (keep for compatibility)
    float clamp(float num) {
        return num > 0 ? num : 0;
    }

    Vector3f Shade(const Ray &ray, const Hit &hit,
                   const Vector3f &dirToLight, const Vector3f &lightColor) {
        Vector3f shaded = Vector3f::ZERO;
        Vector3f N = hit.getNormal().normalized();
        Vector3f L = dirToLight.normalized();
        Vector3f R = 2 * (Vector3f::dot(N, L)) * N - L;
        Vector3f V = -ray.getDirection().normalized();
        Vector3f Diffuse = diffuseColor * clamp(Vector3f::dot(L, N));
        Vector3f Specular = specularColor * pow(clamp(Vector3f::dot(V, R)), shininess);
        shaded += lightColor * (Diffuse + Specular);
        return shaded;
    }

protected:
    Vector3f diffuseColor;
    Vector3f specularColor;
    float shininess;
    Vector3f reflectiveColor;   // 反射系数
    Vector3f transmissiveColor; // 折射系数
    float ior;                   // 折射率
    Vector3f emission;          // 自发光（用于面光源）
    float roughness;            // 粗糙度: 0=完美镜面, 1=完全粗糙
};

#endif // MATERIAL_H