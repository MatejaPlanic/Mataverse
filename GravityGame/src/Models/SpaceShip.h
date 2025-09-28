#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "./Planets/Planet.h"
#include <GL/freeglut.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

using namespace glm;
using namespace std;

class WormHole;
class Satelit;


class SpaceShip {
public:
    SpaceShip();
    ~SpaceShip();
    void draw() const;

    vec3 position{ 0.0f, 0.0f, 0.0f };
    vec3 baseColor{ 0.75f, 0.78f, 0.82f };
    vec3 velocity{ 0,0,0 };
    float mass = 1.0f;
    void update(float dt, const vector<Planet*>& planets, const WormHole* wormhole, const Satelit* sat = nullptr);
	bool shipCaptured = false;

private:
    void drawKupola() const;
    void drawPrsten() const;

    vector<vec3> kreirajKruznicu(vec3 centar, float r, int brojTacaka) const;
    vector<vector<vec3>> kreirajLoptu(float r, int brojTacakaKruznice, int brojKruznica) const;
    void nacrtaj(GLenum mode, const vector<vec3>& tacke) const;
    void spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const;
    vector<vector<vec3>> kreirajTorus(float R, float r, int slicesMajor, int slicesMinor) const;
    vector<vector<vec3>> lopta;
};
