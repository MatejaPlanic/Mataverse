#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include <GL/freeglut.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>
using namespace glm;
using namespace std;

class Nebo {
public:
	Nebo();
	~Nebo();
	void draw() const;

private:
	vector<vec3> zvezde;
	vector<vec3> kreirajKruznicu(vec3 centar, float r, int brojTacaka) const;
	vector<vector<vec3>> kreirajLoptu(float r, int brojTacakaKruznice, int brojKruznica) const;
	void nacrtaj(GLenum mode, const vector<vec3>& tacke) const;
	void spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const;

};