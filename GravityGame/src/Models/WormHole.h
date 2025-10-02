#pragma once
#include "NebeskoTelo.h"
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

class WormHole :public NebeskoTelo {
public:
	WormHole(vec3 pos, float rad,float mass, int brojPrstenova, float rastojanjePrstenova);
	~WormHole();
	void draw() const override;
	void orientTowardOnce(const glm::vec3& targetPos);
private:
	mat4 m_rotation = glm::mat4(1.0f);
	bool m_hasRotation = false;
	int brojPrstenova;
	float rastojanjePrstenova;
	void drawPrsten() const;
	vector<vec3> kreirajKruznicu(vec3 centar, float r, int brojTacaka) const;

	void nacrtaj(GLenum mode, const vector<vec3>& tacke) const;
	void spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const;
	vector<vector<vec3>> kreirajTorus(float R, float r, int slicesMajor, int slicesMinor) const;
	vector<vector<vector<vec3>>> nizPrstenova;

};
