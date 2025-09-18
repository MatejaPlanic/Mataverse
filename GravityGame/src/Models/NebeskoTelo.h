#pragma once
#include <glm/vec3.hpp>
#include<vector>
class NebeskoTelo {
public:
    NebeskoTelo(const glm::vec3& pos, float rad, float mas);
    virtual ~NebeskoTelo() = default;

    virtual void draw() const = 0;

    const glm::vec3& getPosition() const { return position; }
    float getRadius()  const { return radius; }
    float getMass()    const { return masa; }
    void setPosition(const glm::vec3& p) { position = p; }

protected:
    glm::vec3 position{};
    float     radius{};   
    float     masa{};          

};
