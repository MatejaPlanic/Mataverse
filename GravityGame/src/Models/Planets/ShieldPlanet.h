#pragma once

#include "./Planet.h" 
#include <GL/glut.h>
#include <glm/vec3.hpp>


class ShieldPlanet : public Planet {
public:

    ShieldPlanet(const glm::vec3& pos, float rad);


    void draw() const override;

    bool isHit(const glm::vec3& start, const glm::vec3& end) const;
    
};
