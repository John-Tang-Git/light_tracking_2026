#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <vecmath.h>
#include "object3d.hpp"

// transforms a 3D point using a matrix, returning a 3D point
static Vector3f transformPoint(const Matrix4f &mat, const Vector3f &point) {
    return (mat * Vector4f(point, 1)).xyz();
}

// transform a 3D direction using a matrix, returning a direction
static Vector3f transformDirection(const Matrix4f &mat, const Vector3f &dir) {
    return (mat * Vector4f(dir, 0)).xyz();
}

class Transform : public Object3D {
public:
    Transform() {}

    Transform(const Matrix4f &m, Object3D *obj) : o(obj) {
        transform = m.inverse();
    }

    ~Transform() {
    }

    virtual bool intersect(const Ray &r, Hit &h, float tmin) {
        // 将射线从世界坐标变换到局部坐标
        Vector3f trSource = transformPoint(transform, r.getOrigin());
        Vector3f trDirection = transformDirection(transform, r.getDirection());
        Ray tr(trSource, trDirection);
        
        // 在局部坐标系中进行求交
        bool inter = o->intersect(tr, h, tmin);
        
        if (inter) {
            // 计算局部坐标系中的交点
            Vector3f localPoint = tr.pointAtParameter(h.getT());
            
            // 变换到世界坐标系
            Vector3f worldPoint = transformPoint(transform.inverse(), localPoint);
            
            // 变换法线（逆转置矩阵）
            Vector3f worldNormal = transformDirection(transform.transposed(), h.getNormal()).normalized();
            
            // 存储结果（t 保持不变，因为是射线参数，与坐标系无关）
            h.set(h.getT(), h.getMaterial(), worldNormal, worldPoint);
        }
        return inter;
    }

protected:
    Object3D *o; //un-transformed object
    Matrix4f transform;
};

#endif //TRANSFORM_H
