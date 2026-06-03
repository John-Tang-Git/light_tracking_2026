#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>

#include "scene_parser.hpp"
#include "image.hpp"
#include "camera.hpp"
#include "group.hpp"
#include "light.hpp"
#include <random>
#include <chrono>

#include <string>

using namespace std;

SceneParser *sceneParser = nullptr;
Group* baseGroup = nullptr;

// 随机数生成器
static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

const int MAX_DEPTH = 5;  // 通常 3-5 就足够
const float EPSILON = 1e-4;  // 偏移量，防止自交


// ========== Cook-Torrance BRDF for Glossy Materials ==========

// GGX 法线分布函数 D
float GGX_Distribution(const Vector3f& N, const Vector3f& H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = std::max(0.0f, Vector3f::dot(N, H));
    float NdotH2 = NdotH * NdotH;
    
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = M_PI * denom * denom;
    
    return a2 / denom;
}

// 几何遮蔽函数 G (Smith-GGX)
float GGX_Smith_G(const Vector3f& N, const Vector3f& V, const Vector3f& L, float roughness) {
    float a = roughness;
    float a2 = a * a;
    
    float NdotV = std::max(0.0f, Vector3f::dot(N, V));
    float NdotL = std::max(0.0f, Vector3f::dot(N, L));
    
    float G1_V = 2.0f * NdotV / (NdotV + sqrt(a2 + (1.0f - a2) * NdotV * NdotV));
    float G1_L = 2.0f * NdotL / (NdotL + sqrt(a2 + (1.0f - a2) * NdotL * NdotL));
    
    return G1_V * G1_L;
}

// 菲涅尔项 F (Schlick 近似)
Vector3f Schlick_Fresnel(const Vector3f& F0, float VdotH) {
    return F0 + (Vector3f(1.0f, 1.0f, 1.0f) - F0) * pow(1.0f - VdotH, 5.0f);
}

// Cook-Torrance BRDF 评估
Vector3f evaluateCookTorrance(const Vector3f& N, const Vector3f& V, const Vector3f& L, 
                               const Vector3f& F0, float roughness) {
    Vector3f H = (V + L).normalized();
    
    float NdotL = std::max(0.0f, Vector3f::dot(N, L));
    float NdotV = std::max(0.0f, Vector3f::dot(N, V));
    float VdotH = std::max(0.0f, Vector3f::dot(V, H));
    
    if (NdotL <= 0.0f || NdotV <= 0.0f) return Vector3f::ZERO;
    
    float D = GGX_Distribution(N, H, roughness);
    float G = GGX_Smith_G(N, V, L, roughness);
    Vector3f F = Schlick_Fresnel(F0, VdotH);
    
    float denom = 4.0f * NdotV * NdotL;
    if (denom < 1e-6f) return Vector3f::ZERO;
    
    return F * D * G / denom;
}

// 重要性采样 glossy 方向 (基于 GGX)
Vector3f sampleGlossyDirection(const Vector3f& N, const Vector3f& V, float roughness, Vector3f& F0, float& pdf) {
    // 局部坐标系
    Vector3f w = N;
    Vector3f u = Vector3f::cross((fabs(w.x()) > 0.9f ? Vector3f(0, 1, 0) : Vector3f(1, 0, 0)), w).normalized();
    Vector3f v = Vector3f::cross(w, u);
    
    // GGX 重要性采样微表面法线 H
    float r1 = dist(gen);
    float r2 = dist(gen);
    
    float a = roughness;
    float a2 = a * a;
    
    float phi = 2.0f * M_PI * r1;
    float cosTheta = sqrt((1.0f - r2) / (1.0f + (a2 - 1.0f) * r2));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    
    // 局部 H 方向
    Vector3f H_local(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
    Vector3f H = (H_local.x() * u + H_local.y() * w + H_local.z() * v).normalized();
    
    // 反射方向 L = 2*(V·H)H - V
    float VdotH = std::max(0.0f, Vector3f::dot(V, H));
    Vector3f L = (2.0f * VdotH * H - V).normalized();
    
    // 计算 PDF
    float NdotH = std::max(0.0f, Vector3f::dot(N, H));
    float D = GGX_Distribution(N, H, roughness);
    pdf = D * NdotH / (4.0f * VdotH);
    
    if (pdf < 1e-6f) pdf = 1e-6f;
    
    return L;
}

Vector3f sampleDiffuseDirection(const Vector3f& N, float& pdf) {
    // 局部坐标系
    Vector3f w = N;
    Vector3f u = Vector3f::cross((fabs(w.x()) > 0.9f ? Vector3f(0, 1, 0) : Vector3f(1, 0, 0)), w).normalized();
    Vector3f v = Vector3f::cross(w, u);
    
    // 余弦加权采样 (cosθ = sqrt(1 - r2))
    float r1 = dist(gen);
    float r2 = dist(gen);
    float phi = 2.0f * M_PI * r1;
    float cosTheta = sqrt(1.0f - r2);
    float sinTheta = sqrt(r2);
    
    // 局部方向转世界方向
    Vector3f localDir(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
    Vector3f worldDir = (localDir.x() * u + localDir.y() * w + localDir.z() * v).normalized();
    
    pdf = cosTheta / M_PI;  // 余弦加权采样的PDF
    return worldDir;
}

// 采样漫反射或 glossy 方向（根据材质）
Vector3f sampleDirectionByMaterial(Material* mat, const Vector3f& N, const Vector3f& V, float& pdf, bool& isGlossy) {
    Vector3f F0 = mat->getSpecularColor();
    if (F0.length() < 1e-3f) {
        F0 = Vector3f(0.04f, 0.04f, 0.04f);  // 非金属默认 F0
    }
    
    float roughness = mat->getRoughness();
    bool glossy = mat->isGlossy();
    
    if (glossy && roughness > 0.01f) {
        isGlossy = true;
        return sampleGlossyDirection(N, V, roughness, F0, pdf);
    } else {
        isGlossy = false;
        return sampleDiffuseDirection(N, pdf);
    }
}


// 漫反射材质：余弦加权采样


// 漫反射BRDF计算
Vector3f computeDiffuseBRDF(const Vector3f& albedo) {
    return albedo / M_PI;
}


// 折射方向计算函数
Vector3f refract(const Vector3f& I, const Vector3f& N, float ior) {

    float cosi = std::max(-1.0f, std::min(1.0f, Vector3f::dot(I, N)));

    float etai = 1.0f;
    float etat = ior;

    Vector3f n = N;

    if (cosi < 0) {
        // 从空气进入物体
        cosi = -cosi;
    } else {
        // 从物体离开空气
        std::swap(etai, etat);
        n = -N;
    }
    float eta = etai / etat;
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
    if (k < 0.0f)
    {
        // 全反射
        return Vector3f::ZERO;
    }
    return eta * I + (eta * cosi - sqrt(k)) * n;
}

Vector3f pathTracing(Ray ray, int depth) {
    if (depth >= MAX_DEPTH) {
        return Vector3f::ZERO;
    }

    Hit hit;
    if (!baseGroup->intersect(ray, hit, EPSILON)) {
        return Vector3f::ZERO;
    }
    
    Vector3f hitPoint = hit.getIntersectionPoint();
    Vector3f N = hit.getNormal().normalized();
    Vector3f V = -ray.getDirection().normalized();  // 视线方向（指向相机）
    Material* mat = hit.getMaterial();
    
    // ========== 击中光源 ==========
    if (mat->isLight()) {
        return mat->getEmission();
    }
    
    Vector3f albedo = mat->getDiffuseColor();
    Vector3f reflectiveColor = mat->getReflectiveColor();
    Vector3f transmissiveColor = mat->getTransmissiveColor();
    Vector3f specularColor = mat->getSpecularColor();
    float roughness = mat->getRoughness();
    
    // 菲涅尔基准色 F0（非金属默认 0.04）
    Vector3f F0 = specularColor;
    if (F0.length() < 1e-3f) {
        F0 = Vector3f(0.04f, 0.04f, 0.04f);
    }
    
    // 判断材质类型
    bool isPerfectReflect = reflectiveColor.length() > 0.9f && roughness <= 0.01f;
    bool isPerfectRefract = transmissiveColor.length() > 0.9f;
    bool isGlossy = !isPerfectReflect && !isPerfectRefract && reflectiveColor.length() > 1e-3f && roughness > 0.01f && roughness < 0.99f;
    bool isDiffuse = !isPerfectReflect && !isPerfectRefract && !isGlossy;
    
    // ========== 完美镜面反射 ==========
    if (isPerfectReflect) {
        Vector3f I = ray.getDirection().normalized();
        Vector3f reflectDir = I - 2 * Vector3f::dot(I, N) * N;
        reflectDir.normalize();
        Ray reflectRay(hitPoint + reflectDir * EPSILON, reflectDir);
        Vector3f reflectCol = pathTracing(reflectRay, depth + 1);
        return reflectiveColor * reflectCol;
    }
    
    // ========== 完美折射 ==========
    if (isPerfectRefract) {
        float ior = mat->getIor();
        Vector3f I = ray.getDirection().normalized();
        Vector3f refractDir = refract(I, N, ior);
        
        // 全反射时当作反射处理
        if (refractDir.squaredLength() < 1e-4) {
            Vector3f reflectDir = I - 2 * Vector3f::dot(I, N) * N;
            reflectDir.normalize();
            Ray reflectRay(hitPoint + reflectDir * EPSILON, reflectDir);
            Vector3f reflectCol = pathTracing(reflectRay, depth + 1);
            Vector3f fallbackReflect = reflectiveColor.length() > 1e-3 ? reflectiveColor : Vector3f(1.0f, 1.0f, 1.0f);
            return fallbackReflect * reflectCol;
        }
        
        refractDir.normalize();
        Ray refractRay(hitPoint + refractDir * EPSILON, refractDir);
        Vector3f refractCol = pathTracing(refractRay, depth + 1);
        return transmissiveColor * refractCol;
    }
    
    // ========== Direct Lighting (NEE - Next Event Estimation) ==========
    Vector3f directLight = Vector3f::ZERO;
    
    for (int li = 0; li < sceneParser->getNumLights(); ++li) {
        Light* light = sceneParser->getLight(li);
        AreaLight* areaLight = dynamic_cast<AreaLight*>(light);
        
        if (areaLight) {
            float pdf_area;
            Vector3f lightPoint = areaLight->samplePoint(pdf_area);
            Vector3f wi = (lightPoint - hitPoint).normalized();
            float dist2 = (lightPoint - hitPoint).squaredLength();
            float dist = sqrt(dist2);
            
            float cosTheta = std::max(0.0f, Vector3f::dot(wi, N));
            float cosThetaLight = std::max(0.0f, Vector3f::dot(-wi, areaLight->getNormal()));
            
            if (cosTheta <= 0 || cosThetaLight <= 0) continue;
            
            // 阴影测试（支持半透明材质）
            Vector3f transmittance(1.0f, 1.0f, 1.0f);
            Vector3f currentOrigin = hitPoint + wi * EPSILON;
            float totalDist = 0.0f;
            bool fullyOccluded = false;
            
            while (totalDist < dist - EPSILON) {
                Ray shadowRay(currentOrigin, wi);
                Hit shadowHit;
                if (!baseGroup->intersect(shadowRay, shadowHit, EPSILON)) {
                    break;
                }
                
                float hitDist = shadowHit.getT();
                totalDist += hitDist;
                
                if (totalDist > dist - EPSILON) break;
                
                Material* hitMat = shadowHit.getMaterial();
                
                // 碰到光源，停止
                if (hitMat->isLight()) {
                    break;
                }
                
                Vector3f trans = hitMat->getTransmissiveColor();
                if (trans.length() > 1e-3) {
                    // 半透明材质：累乘透射率
                    transmittance = transmittance * trans;
                    currentOrigin = shadowHit.getIntersectionPoint() + wi * EPSILON;
                } else {
                    // 不透明材质：完全遮挡
                    fullyOccluded = true;
                    break;
                }
            }
            
            if (!fullyOccluded && transmittance.length() > 1e-3) {
                Vector3f Le = areaLight->getColor();
                float pdf_w = pdf_area * dist2 / cosThetaLight;
                
                // 根据材质类型选择 BRDF
                Vector3f brdfValue;
                if (isGlossy) {
                    brdfValue = evaluateCookTorrance(N, V, wi, F0, roughness);
                } else {
                    // 漫反射 BRDF
                    brdfValue = albedo / M_PI;
                }
                
                directLight += brdfValue * Le * cosTheta / pdf_w * transmittance;
            }
        }
    }
    
    // ========== 俄罗斯轮盘赌 ==========
    float survivalProb = 0.8f;
    if (depth > 3 && dist(gen) > survivalProb) {
        return directLight;
    }
    float invSurvival = (depth > 3) ? (1.0f / survivalProb) : 1.0f;
    
    // ========== Indirect Lighting（根据材质类型采样方向） ==========
    float pdf;
    Vector3f wi;
    
    if (isGlossy) {
        // Glossy 材质：GGX 重要性采样
        wi = sampleGlossyDirection(N, V, roughness, F0, pdf);
    } else {
        // 漫反射材质：余弦加权采样
        wi = sampleDiffuseDirection(N, pdf);
    }
    
    Ray nextRay(hitPoint + wi * EPSILON, wi);
    Vector3f Li_in = pathTracing(nextRay, depth + 1);
    float cosTheta = std::max(0.0f, Vector3f::dot(wi, N));
    
    // 计算 BRDF 值
    Vector3f brdfValue;
    if (isGlossy) {
        brdfValue = evaluateCookTorrance(N, V, wi, F0, roughness);
    } else {
        brdfValue = albedo / M_PI;
    }
    
    Vector3f indirectLight = brdfValue * Li_in * cosTheta / pdf * invSurvival;
    
    return directLight + indirectLight;
}


Vector3f IntersectColor(Ray ray, int depth) {
    Hit hit;
    bool isIntersect = baseGroup->intersect(ray, hit, EPSILON);
    
    if (!isIntersect) {
        return sceneParser->getBackgroundColor();
    }
    
    Vector3f finalColor = Vector3f::ZERO;
    Vector3f hitPoint = hit.getIntersectionPoint();
    Vector3f N = hit.getNormal().normalized();
    Vector3f V = ray.getDirection().normalized();

    Vector3f ambient(0.15f,0.15f,0.15f);
    finalColor += ambient * hit.getMaterial()->getDiffuseColor();
    
    // ========== 1. 直接光照 + 正确阴影（修复版） ==========
    for (int li = 0; li < sceneParser->getNumLights(); ++li) {
        Light* light = sceneParser->getLight(li);
        
        Vector3f L, lightColor;
        light->getIllumination(hitPoint, L, lightColor);

        // 阴影方向
        Vector3f shadowDir = L.normalized();
        Ray shadowRay(hitPoint + shadowDir * EPSILON, shadowDir);

        // 光源距离
        float lightDist = 1e9;
        PointLight* pointLight = dynamic_cast<PointLight*>(light);
        if (pointLight) {
            lightDist = (pointLight->getPosition() - hitPoint).length();
        }

        // 阴影衰减：初始 = 完全不遮挡
        Vector3f shadowAtten(1.0f);
        Vector3f currentOrigin = shadowRay.getOrigin();
        float totalT = 0.0f;

        // 循环追踪所有透明遮挡物
        while (true) {
            Hit tmpHit;
            bool hasInter = baseGroup->intersect(Ray(currentOrigin, shadowDir), tmpHit, EPSILON);
            if (!hasInter) break;

            float t = tmpHit.getT();
            totalT += t;

            // 超过光源 → 不再计算
            if (totalT > lightDist - EPSILON) break;

            Material* mat = tmpHit.getMaterial();
            Vector3f trans = mat->getTransmissiveColor();

            // 完全不透明 → 直接全黑
            if (trans.x() < 1e-3 && trans.y() < 1e-3 && trans.z() < 1e-3) {
                shadowAtten = Vector3f::ZERO;
                break;
            }

            // 透明：累乘衰减（越叠越暗）
            shadowAtten = shadowAtten * trans;

            // 前进到下一个点
            currentOrigin = tmpHit.getIntersectionPoint() + shadowDir * EPSILON;
        }

        // 应用光照 + RGB阴影（彩色透明阴影更自然）
        finalColor += hit.getMaterial()->Shade(ray, hit, L, lightColor) * shadowAtten;
    }

    
    // 递归深度终止
    if (depth >= MAX_DEPTH) {
        return finalColor;
    }
    
    // ========== 2. 反射 ==========
    Vector3f reflectiveColor = hit.getMaterial()->getReflectiveColor();
    if (reflectiveColor.length() > 1e-3) {
        Vector3f reflectDir = V - 2 * Vector3f::dot(V, N) * N;
        reflectDir.normalize();
        Ray reflectRay(hitPoint + reflectDir * EPSILON, reflectDir);
        Vector3f reflectCol = IntersectColor(reflectRay, depth + 1);
        finalColor += reflectiveColor * reflectCol;
    }
    
    // ========== 3. 折射（修复全反射黑块） ==========
    Vector3f transmissiveColor = hit.getMaterial()->getTransmissiveColor();
    if (transmissiveColor.length() > 1e-3) {
        float ior = hit.getMaterial()->getIor();
        Vector3f refractDir = refract(V, N, ior);
        
        // 只有有效折射方向才继续（避免全反射发黑）
        if (refractDir.squaredLength() > 1e-4) {
            refractDir.normalize();
            Ray refractRay(hitPoint + refractDir * EPSILON, refractDir);
            Vector3f refractCol = IntersectColor(refractRay, depth + 1);
            finalColor += transmissiveColor * refractCol;
        }
    }
    
    return finalColor;
}


int main(int argc, char *argv[]) {
    for (int argNum = 1; argNum < argc; ++argNum) {
        std::cout << "Argument " << argNum << " is: " << argv[argNum] << std::endl;
    }

    if (argc != 3) {
        std::cout << "Usage: ./bin/PA1 <input scene file> <output bmp file>" << endl;
        return 1;
    }
    string inputFile = argv[1];
    string outputFile = argv[2];  // only bmp is allowed.

    // TODO: Main RayCasting Logic
    // First, parse the scene using SceneParser.
    // Then loop over each pixel in the image, shooting a ray
    // through that pixel and finding its intersection with
    // the scene.  Write the color at the intersection to that
    // pixel in your output image.
    sceneParser =  new SceneParser(inputFile.c_str());

    Camera *camera = sceneParser->getCamera();
    Image renderedImg(camera->getWidth(), camera->getHeight());
    baseGroup = sceneParser->getGroup();

    // 循 环 屏 幕 空 间 的 像 素
    // for ( int x = 0; x<camera->getWidth(); ++x) {
    //     for ( int y = 0; y<camera->getHeight(); ++y) {
    //         // 计 算 当 前 像 素 (x , y) 处 相 机 出 射 光 线camRay
    //         Ray camRay = sceneParser->getCamera()->generateRay(Vector2f(x, y));
    //         // 计 算 光 线 与 场 景 中 物 体 的 相 交 点
    //         Vector3f color = IntersectColor(camRay, 0);
    //         renderedImg.SetPixel(x, y, color);
    //     }
    // }

    int spp = 200;  // 每像素采样数
    
    for (int x = 0; x < camera->getWidth(); ++x) {
        for (int y = 0; y < camera->getHeight(); ++y) {
            Vector3f color(0, 0, 0);
            
            for (int s = 0; s < spp; ++s) {
                // 随机抖动，抗锯齿
                float u = (x + dist(gen));
                float v = (y + dist(gen));
                Ray camRay = camera->generateRay(Vector2f(u, v));
                color += pathTracing(camRay, 0);
            }
            
            color = color / spp;
            renderedImg.SetPixel(x, y, color);
        }
    }


    //保存图片
    renderedImg.SaveImage(argv[2]);


    std::cout << "Hello! Computer Graphics!" << endl;
    return 0;
}

