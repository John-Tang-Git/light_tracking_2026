#ifndef GROUP_H
#define GROUP_H


#include "object3d.hpp"
#include "ray.hpp"
#include "hit.hpp"
#include <iostream>
#include <vector>

using namespace std;


// TODO: Implement Group - add data structure to store a list of Object*
class Group : public Object3D {

public:

    Group() {
        //默认构造什么也不用做
    }

    explicit Group (int num_objects) {
        grp.resize(num_objects, nullptr);
    }

    ~Group() override {
        for (auto obj : grp){
            delete obj;
        }
    }

    bool intersect(const Ray &r, Hit &h, float tmin) override {
        bool flag = false;
        for (auto obj : grp){
            if(obj && obj->intersect(r, h, tmin)) flag = true;
        }
        return flag;
    }

    void addObject(int index, Object3D *obj) {
        grp.insert(grp.begin()+index, obj);
    }

    int getGroupSize() {
        return grp.size();
    }

private:
    vector<Object3D*> grp; //存储列表
};

#endif
	
