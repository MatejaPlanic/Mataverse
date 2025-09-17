#include "Nebo.h"

static vec3 operator*(mat4x4 mat, vec3 vec)
{
	vec4 v(vec.x, vec.y, vec.z, 1.f);
	v = mat * v;
	return vec3(v.x, v.y, v.z);
}

static vector<vec3> operator*(mat4x4 mat, vector<vec3> vectors)
{
	for (int i = 0; i < (int)vectors.size(); i++)
		vectors[i] = mat * vectors[i];
	return vectors;
}

Nebo::Nebo() {

	const int brojZvezda = 500;
	zvezde.resize(brojZvezda);
	for (int i = 0; i < brojZvezda; ++i) {
		float fi = 2.0f * 3.14f * (rand() / (float)RAND_MAX);
		float teta = 3.14f * (rand() / (float)RAND_MAX);
		float r = 49.5f;
		zvezde[i].x = r * sin(teta) * cos(fi);
		zvezde[i].y = r * sin(teta) * sin(fi);
		zvezde[i].z = r * cos(teta);
	}
}

vector<vec3> Nebo::kreirajKruznicu(vec3 centar, float r, int brojTacaka) const
{
	vector<vec3> kruznica;
	kruznica.resize(brojTacaka);

	float fi = 2.0f * 3.14f / (float)brojTacaka;

	mat4x4 matricaRotacijeY = rotate(fi, vec3(0, 1, 0));
	mat4x4 matricaTranslacije = translate(centar);

	kruznica[0] = vec3(r, 0, 0);
	for (int i = 1; i < brojTacaka; i++)
		kruznica[i] = matricaRotacijeY * kruznica[i - 1];

	for (int i = 0; i < brojTacaka; i++)
		kruznica[i] = matricaTranslacije * kruznica[i];

	return kruznica;
}
vector<vector<vec3>> Nebo::kreirajLoptu(float r, int brojTacakaKruznice, int brojKruznica) const
{
	vector<vector<vec3>> l;

	l.resize(brojKruznica);
	for (int i = 0; i < brojKruznica; i++)
		l[i].resize(brojTacakaKruznice);

	float fi = 2.0f * 3.14f / (float)brojKruznica;
	mat4x4 matricaRotacijeZ = rotate(fi, vec3(0, 0, 1));

	l[0] = kreirajKruznicu(vec3(0, 0, 0), r, brojTacakaKruznice);
	for (int i = 1; i < brojKruznica; i++)
		l[i] = matricaRotacijeZ * l[i - 1];

	return l;
}

void Nebo::nacrtaj(GLenum mode, const vector<vec3>& tacke) const
{
	glBegin(mode);
	for (int i = 0; i < (int)tacke.size(); i++)
		glVertex3f(tacke[i].x, tacke[i].y, tacke[i].z);
	glEnd();
}

void Nebo::spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const
{
	vector<vec3> poligon(4);
	int n = (int)prva.size();
	for (int i = 0; i < n - 1; i++)
	{
		poligon[0] = prva[i];
		poligon[1] = druga[i];
		poligon[2] = druga[i + 1];
		poligon[3] = prva[i + 1];
		nacrtaj(GL_POLYGON, poligon);
	}
	poligon[0] = prva[n - 1];
	poligon[1] = druga[n - 1];
	poligon[2] = druga[0];
	poligon[3] = prva[0];
	nacrtaj(GL_POLYGON, poligon);
}

Nebo::~Nebo() {
}

void Nebo::draw() const {
	glDepthMask(GL_FALSE);
	glPointSize(2.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_POINTS);
	for (const auto& zvezda : zvezde) {
		glVertex3f(zvezda.x, zvezda.y, zvezda.z);
	}
	glEnd();
	glDepthMask(GL_TRUE);

}