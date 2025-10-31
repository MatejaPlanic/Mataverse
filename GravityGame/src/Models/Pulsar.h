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

    void setPosition(const glm::vec3& p) { position = p; }
    glm::vec3 getPosition() const { return position; }
    void setRadius(float r) { radius = r; }
    void setBeamLength(float L) { beamLength = L; }
    void setRotSpeed(float degPerSec) { rotSpeedDeg = degPerSec; }

    bool hitsShip(const glm::vec3& shipPos, float shipRadius) const;

    void setOccluderSphere(const glm::vec3& c, float r, bool enabled);

private:
    void drawGlowSphere(int rings = 14, int sectors = 18) const;
    void drawBeamSoft(float length, float widthStart, float widthEnd,
        int sheets = 8, int segments = 28) const;

    glm::vec3 position;
    float radius;
    float beamLength;
    float rotSpeedDeg;
    float angleDeg;

    bool  occEnabled = false;
    glm::vec3 occCenter{ 0.0f };
    float occRadius = 0.0f;

    static bool raySphereIntersect(const glm::vec3& ro, const glm::vec3& rd,
        const glm::vec3& C, float R, float& tHit);
    float truncatedLen(const glm::vec3& ro, const glm::vec3& rd, float baseLen) const;
};
