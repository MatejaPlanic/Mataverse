#pragma once
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

class Satelit {
public:
    Satelit(const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& bodyColor = glm::vec3(0.75f, 0.8f, 0.9f));

    void setPosition(const glm::vec3& p);
    void setRotationY(float degrees);      
    void setScale(float uniformScale);

    void setPanelColor(const glm::vec3& c);
    void setAccentColor(const glm::vec3& c); 

    void addSpin(float degreesDelta);

    void draw() const;

    const vec3& getPosition() const { return m_pos; }
    float getRotationY() const { return m_rotY; }
    float getScale() const { return m_scale; }
    vec3 forward() const {
        float rad = glm::radians(m_rotY);
        return glm::normalize(glm::vec3(std::sin(rad), 0.0f, std::cos(rad)));
    }

private:
    void drawBody()           const;  
    void drawSolarPanel(bool left) const; 
    void drawMounts()         const; 
    void drawAntenna()        const;  

    vec3 m_pos;
    vec3 m_bodyColor;
    vec3 m_panelColor;
    vec3 m_accentColor;

    float m_rotY;   
    float m_scale;  

    float m_bodyW, m_bodyH, m_bodyD;  
    float m_panelW, m_panelH, m_panelT; 
    float m_mountLen, m_mountT;       
};