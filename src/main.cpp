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

const int MAX_DEPTH = 10;  // 通常 3-5 就足够
const float EPSILON = 1e-4;  // 偏移量，防止自交

// 折射方向计算函数
Vector3f refract(const Vector3f& V, const Vector3f& N, float eta) {
    float cosI = Vector3f::dot(V, N);
    float cosT2 = 1 - eta * eta * (1 - cosI * cosI);
    if (cosT2 < 0) {
        return Vector3f::ZERO;  // 全反射
    }
    float cosT = sqrt(cosT2);
    return eta * (-V) + (eta * cosI - cosT) * N;
}

Vector3f IntersectColor(Ray ray, int depth) {
    Hit hit;
    bool isIntersect = baseGroup->intersect(ray, hit, EPSILON);
    
    if (isIntersect) {
        Vector3f finalColor = Vector3f::ZERO;
        
        // 直接光照
        for (int li = 0; li < sceneParser->getNumLights(); ++li) {
            Light* light = sceneParser->getLight(li);
            Vector3f L, lightColor;
            light->getIllumination(ray.pointAtParameter(hit.getT()), L, lightColor);
            finalColor += hit.getMaterial()->Shade(ray, hit, L, lightColor);
        }
        
        if (depth >= MAX_DEPTH) {
            return finalColor;
        }

        Vector3f hitPoint = hit.getIntersectionPoint();
        Vector3f N = hit.getNormal().normalized();
        Vector3f V = ray.getDirection().normalized();

        // ========== 反射 ==========
        Vector3f reflectiveColor = hit.getMaterial()->getReflectiveColor();
        if (reflectiveColor.length() > 1e-3) {
            Vector3f reflectDir = V - 2 * Vector3f::dot(V, N) * N;
            reflectDir.normalize();  // 确保归一化
            
            // 沿反射方向偏移，防止自交
            Ray reflectRay(hitPoint + reflectDir * EPSILON, reflectDir);
            Vector3f reflectCol = IntersectColor(reflectRay, depth + 1);
            finalColor += reflectiveColor * reflectCol;
        }

        // ========== 折射（修复版） ==========
        Vector3f transmissiveColor = hit.getMaterial()->getTransmissiveColor();
        if (transmissiveColor.length() > 1e-3) {
            float ior = hit.getMaterial()->getIor();
            Vector3f normal = N;
            float eta = 1.0f / ior;  // 空气到物体
            
            // 判断是从空气进入物体，还是从物体射出
            if (Vector3f::dot(V, N) > 0) {
                // 光线从内部射出
                eta = ior;
                normal = -N;
            }
            
            Vector3f refractDir = refract(V, normal, eta);
            if (refractDir.length() > 1e-3) {
                refractDir.normalize();
                
                // 【关键修复】沿折射方向偏移，而不是沿法线
                Ray refractRay(hitPoint + refractDir * EPSILON, refractDir);
                Vector3f refractCol = IntersectColor(refractRay, depth + 1);
                finalColor += transmissiveColor * refractCol;
            }
            // 如果全反射，不添加折射贡献
        }

        return finalColor;
    } else {
        return sceneParser->getBackgroundColor();
    }
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

