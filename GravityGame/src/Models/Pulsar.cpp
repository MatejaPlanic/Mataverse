#include "Pulsar.h"
#include <cmath>
#include <GL/glut.h>    
#include <glm/gtc/constants.hpp>

using glm::vec3;

Pulsar::Pulsar(const vec3& pos, float r, float beamLen, float rotSpeed)
    : position(pos),
    radius(r),
    beamLength(beamLen),
    rotSpeedDeg(rotSpeed),
    angleDeg(0.0f)
{
}

void Pulsar::update(float dt) {
    angleDeg += rotSpeedDeg * dt;
    if (angleDeg >= 360.0f) angleDeg -= 360.0f;
    if (angleDeg < 0.0f)    angleDeg += 360.0f;
}

void Pulsar::draw() const
{
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    glColor4f(0.6f, 0.8f, 1.0f, 0.06f);
    glPushMatrix(); glScalef(radius * 1.9f, radius * 1.9f, radius * 1.9f); drawGlowSphere(); glPopMatrix();

    glColor4f(0.7f, 0.85f, 1.0f, 0.10f);
    glPushMatrix(); glScalef(radius * 1.5f, radius * 1.5f, radius * 1.5f); drawGlowSphere(); glPopMatrix();

    glColor4f(0.95f, 0.98f, 1.0f, 0.25f);
    glPushMatrix(); glScalef(radius, radius, radius); drawGlowSphere(); glPopMatrix();

    glPushMatrix();
    glRotatef(angleDeg, 0.f, 1.f, 0.f);

    float wStart = radius * 0.45f; 
    float wEnd = radius * 0.04f;   

    glPushMatrix();
    glTranslatef(0.f, 0.f, radius * 0.15f);
    drawBeamSoft(beamLength, wStart, wEnd,10,32);
    glPopMatrix();

    glPushMatrix();
    glRotatef(180.f, 1.f, 0.f, 0.f);
    glTranslatef(0.f, 0.f, radius * 0.15f);
    drawBeamSoft(beamLength, wStart, wEnd, 10, 32);
    glPopMatrix();

    glPopMatrix(); 

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glColor4f(1, 1, 1, 1);

    glPopAttrib();
    glPopMatrix();
}

void Pulsar::drawGlowSphere(int rings, int sectors) const
{
    const float PI = glm::pi<float>();
    const float TWO_PI = 2.f * PI;

    for (int r = 0; r < rings; ++r) {
        float v0 = float(r) / float(rings);
        float v1 = float(r + 1) / float(rings);
        float phi0 = v0 * PI;
        float phi1 = v1 * PI;

        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= sectors; ++s) {
            float u = float(s) / float(sectors);
            float theta = u * TWO_PI;

            float x0 = std::sin(phi0) * std::cos(theta);
            float y0 = std::cos(phi0);
            float z0 = std::sin(phi0) * std::sin(theta);

            float x1 = std::sin(phi1) * std::cos(theta);
            float y1 = std::cos(phi1);
            float z1 = std::sin(phi1) * std::sin(theta);

            glVertex3f(x0, y0, z0);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }
}

void Pulsar::drawBeamSoft(float length, float widthStart, float widthEnd,
    int sheets, int segments) const
{
    if (sheets < 2) sheets = 2;
    if (segments < 2) segments = 2;

    const float edgeFade = 0.12f;
    for (int s = 0; s < sheets; ++s) {
        float ang = (360.0f / sheets) * s;

        glPushMatrix();
        glRotatef(ang, 0.f, 0.f, 1.f); 

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float t = float(i) / float(segments);    
            float z = t * length;
            float hw = 0.5f * (widthStart + (widthEnd - widthStart) * t); 
            float aLine = 0.28f * (1.0f - t);

            glColor4f(0.55f, 0.85f, 1.0f, aLine * edgeFade);
            glVertex3f(-hw, 0.f, z);

            glColor4f(0.55f, 0.85f, 1.0f, aLine * edgeFade);
            glVertex3f(hw, 0.f, z);
        }
        glEnd();

        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float t = float(i) / float(segments);
            float z = t * length;
            float aLine = 0.35f * (1.0f - t);
            glColor4f(0.85f, 0.95f, 1.0f, aLine);
            glVertex3f(0.f, 0.f, z);
        }
        glEnd();

        glPopMatrix();
    }
}