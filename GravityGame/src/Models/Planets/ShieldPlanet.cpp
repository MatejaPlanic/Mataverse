#include "ShieldPlanet.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>

ShieldPlanet::ShieldPlanet(const glm::vec3& pos, float rad)
    : Planet(pos,vec3(0,0,0), rad, 0.0f)
{
}

void ShieldPlanet::draw() const {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    GLfloat ambient[] = { 0.05f, 0.05f, 0.10f, 1.0f };
    GLfloat diffuse[] = { 0.10f, 0.10f, 0.25f, 1.0f };
    GLfloat specular[] = { 0.80f, 0.80f, 1.00f, 1.0f };
    GLfloat shininess[] = { 100.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

    glColor3f(0.1f, 0.1f, 0.2f); 

    glutSolidSphere(radius, 40, 40);

    glPopMatrix();

}

bool ShieldPlanet::isHit(const glm::vec3& start, const glm::vec3& end) const {
    glm::vec3 dir = end - start;
    float lenSq = glm::dot(dir, dir);

    if (lenSq < 1e-6f) { 
        return glm::distance(start, position) <= radius;
    }

    glm::vec3 L = position - start; 
    float tca = glm::dot(L, dir) / std::sqrt(lenSq);

    float d2 = glm::dot(L, L) - tca * tca * lenSq; 

    if (d2 > radius * radius) {
        return false;
    }

    float thc = std::sqrt(radius * radius - d2);

    float t0 = tca - thc; 
    float t1 = tca + thc; 

    t0 /= std::sqrt(lenSq);
    t1 /= std::sqrt(lenSq);

    return (t0 >= 0.0f && t0 <= 1.0f) || (t1 >= 0.0f && t1 <= 1.0f);
}