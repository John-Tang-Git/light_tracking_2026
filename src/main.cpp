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

#include <string>

using namespace std;

SceneParser *sceneParser = nullptr;
Group* baseGroup = nullptr;

const int MAX_DEPTH = 5;  // 通常 3-5 就足够
const float EPSILON = 1e-4;  // 偏移量，防止自交

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
    for ( int x = 0; x<camera->getWidth(); ++x) {
        for ( int y = 0; y<camera->getHeight(); ++y) {
            // 计 算 当 前 像 素 (x , y) 处 相 机 出 射 光 线camRay
            Ray camRay = sceneParser->getCamera()->generateRay(Vector2f(x, y));
            // 计 算 光 线 与 场 景 中 物 体 的 相 交 点
            Vector3f color = IntersectColor(camRay, 0);
            renderedImg.SetPixel(x, y, color);
        }
    }

    //保存图片
    renderedImg.SaveImage(argv[2]);


    std::cout << "Hello! Computer Graphics!" << endl;
    return 0;
}

