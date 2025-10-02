#pragma once
#include <glm/glm.hpp>

class Pulsar {
public:
    Pulsar(const glm::vec3& pos = glm::vec3(0),
        float radius = 0.5f,
        float beamLen = 2.5f,
        float rotSpeedDeg = 40.0f);

    void update(float dt);

    void draw() const;

    inline void setPosition(const glm::vec3& p) { position = p; }
    inline glm::vec3 getPosition() const { return position; }
    inline void setRadius(float r) { radius = r; }
    inline void setBeamLength(float L) { beamLength = L; }
    inline void setRotSpeed(float degPerSec) { rotSpeedDeg = degPerSec; }

private:
    void drawGlowSphere(int rings = 14, int sectors = 18) const;
    void drawBeamSoft(float length, float widthStart, float widthEnd,int sheets = 8, int segments = 28) const;

private:
    glm::vec3 position;
    float radius;
    float beamLength;
    float rotSpeedDeg; 
    float angleDeg;   
};
