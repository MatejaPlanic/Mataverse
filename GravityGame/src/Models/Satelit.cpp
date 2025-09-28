#include "Satelit.h"
#include <GL/glut.h>
#include <GL/glu.h>
#include <cmath>

using glm::vec3;

Satelit::Satelit(const vec3& position, const vec3& bodyColor)
    : m_pos(position),
    m_bodyColor(bodyColor),
    m_panelColor(0.20f, 0.55f, 1.0f),   
    m_accentColor(0.90f, 0.90f, 0.95f), 
    m_rotY(0.0f),
    m_scale(1.0f),
    m_bodyW(0.9f), m_bodyH(0.6f), m_bodyD(0.6f),
    m_panelW(1.6f), m_panelH(0.55f), m_panelT(0.04f),
    m_mountLen(0.35f), m_mountT(0.04f)
{
}

void Satelit::setPosition(const vec3& p) { m_pos = p; }
void Satelit::setRotationY(float degrees) { m_rotY = degrees; }
void Satelit::setScale(float s) { m_scale = s; }
void Satelit::setPanelColor(const vec3& c) { m_panelColor = c; }
void Satelit::setAccentColor(const vec3& c) { m_accentColor = c; }
void Satelit::addSpin(float d) { m_rotY += d; }

static void drawBox(float w, float h, float d)
{
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0);
    glPopMatrix();
}

static void drawFilledDisk(float radius, int segments = 48)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.f, 0.f, 0.f);
    for (int i = 0; i <= segments; ++i) {
        float a = (2.0f * 3.14159265f * i) / segments;
        glVertex3f(radius * cosf(a), radius * sinf(a), 0.f);
    }
    glEnd();
}

void Satelit::drawBody() const
{
    glColor3f(m_bodyColor.r, m_bodyColor.g, m_bodyColor.b);
    drawBox(m_bodyW, m_bodyH, m_bodyD);

    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    glPushMatrix();
    glScalef(m_bodyW * 1.01f, m_bodyH * 1.01f, m_bodyD * 1.01f);
    glutWireCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, m_bodyD * 0.5f + 0.0005f);

    float R = 0.35f * m_bodyH;

    glColor3f(0.90f, 0.10f, 0.10f);
    drawFilledDisk(R, 64);

    glColor3f(0.f, 0.f, 0.f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 64; ++i) {
        float a = (2.0f * 3.14159265f * i) / 64.0f;
        glVertex3f(R * cosf(a), R * sinf(a), 0.f);
    }
    glEnd();

    glPopMatrix();
}

void Satelit::drawSolarPanel(bool left) const
{
    float side = left ? -1.0f : 1.0f;
    float offsetX = side * (m_bodyW * 0.5f + m_mountLen + m_panelW * 0.5f);

    glPushMatrix();
    glTranslatef(offsetX, 0.0f, 0.0f);
    glColor3f(m_panelColor.r, m_panelColor.g, m_panelColor.b);
    drawBox(m_panelW, m_panelH, m_panelT);

    glPushMatrix();

    glTranslatef(0.0f, 0.0f, m_panelT * 0.501f);


    float halfW = m_panelW * 0.5f * 0.98f;
    float halfH = m_panelH * 0.5f * 0.98f;

    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);

    for (int i = -4; i <= 4; ++i) {
        float t = i / 4.0f;           
        float x = halfW * t;
        glVertex3f(x, -halfH, 0.0f);
        glVertex3f(x, +halfH, 0.0f);
    }

    for (int j = -2; j <= 2; ++j) {
        float t = j / 2.0f;               
        float y = halfH * t;
        glVertex3f(-halfW, y, 0.0f);
        glVertex3f(+halfW, y, 0.0f);
    }
    glEnd();
    glPopMatrix();   
    glPopMatrix();   
}

void Satelit::drawMounts() const
{
    glColor3f(m_accentColor.r, m_accentColor.g, m_accentColor.b);

    glPushMatrix();
    glTranslatef(0.0f, m_bodyH * 0.5f + m_mountT * 0.5f, 0.0f);
    drawBox(m_bodyW * 0.9f, m_mountT, m_mountT);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -m_bodyH * 0.5f - m_mountT * 0.5f, 0.0f);
    drawBox(m_bodyW * 0.9f, m_mountT, m_mountT);
    glPopMatrix();
}

void Satelit::drawAntenna() const
{
    glPushMatrix();
    glTranslatef(0.0f, m_bodyH * 0.5f + 0.02f, 0.0f);

    glColor3f(m_accentColor.r, m_accentColor.g, m_accentColor.b);
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glRotatef(-90.0f, 1, 0, 0); 
    gluCylinder(q, 0.03, 0.03, 0.35, 10, 1);
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.0f, 0.35f, 0.0f);
    glRotatef(-90.0f, 1, 0, 0);
    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidCone(0.08, 0.14, 12, 1);
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.0f, 0.35f + 0.14f, 0.0f);
    glColor3f(1.0f, 0.95f, 0.85f);
    glutSolidSphere(0.05, 12, 12);
    glPopMatrix();

    gluDeleteQuadric(q);
    glPopMatrix();
}

void Satelit::draw() const
{
    glPushMatrix();
    glTranslatef(m_pos.x, m_pos.y, m_pos.z);
    glRotatef(m_rotY, 0, 1, 0);
    glScalef(m_scale, m_scale, m_scale);

    drawBody();
    drawMounts();
    drawSolarPanel(true);  
    drawSolarPanel(false);  
    drawAntenna();

    glPopMatrix();
}
