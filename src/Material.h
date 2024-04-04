#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

struct Material {
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float s;

    Material(glm::vec3 ka, glm::vec3 kd, glm::vec3 ks, float s) {
        this->ka = ka;
        this->kd = kd;
        this->ks = ks;
        this->s = s;
    }

};

#endif