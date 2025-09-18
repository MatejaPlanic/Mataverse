#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <GL/freeglut.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>
#include "../NebeskoTelo.h"
using namespace glm;
using namespace std;

class Planet: public NebeskoTelo{
public:
    Planet(vec3 pos, vec3 col, float rad, float mas);
    ~Planet();

    void draw() const override;
    void dodajKratere(float udubljenje);

private:
    struct Krater {
        vec3 center;
        float radius;
        float depth;
    };

    vec3 color;

    vector<vector<vec3>> lopta;
    vector<Krater> kratere;          

    vector<vec3> kreirajKruznicu(vec3 centar, float r, int brojTacaka) const;
    vector<vector<vec3>> kreirajLoptu(float r, int brojTacakaKruznice, int brojKruznica) const;

    void nacrtaj(GLenum mode, const vector<vec3>& tacke) const;

    float shadeFactor(const vec3& p) const;
};
