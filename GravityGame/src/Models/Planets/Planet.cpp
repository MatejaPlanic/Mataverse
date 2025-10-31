#include "Planet.h"
#include <glm/gtc/type_ptr.hpp>

static inline void emitVertexWithNormal(const glm::vec3& v) {
	glm::vec3 n = glm::normalize(v);          
	glNormal3f(n.x, n.y, n.z);
	glVertex3f(v.x, v.y, v.z);
}

static inline glm::vec3 faceNormal(const glm::vec3& a,
	const glm::vec3& b,
	const glm::vec3& c) {
	return glm::normalize(glm::cross(b - a, c - a));
}

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

Planet::Planet(vec3 pos, vec3 col, float rad, float mas) : NebeskoTelo(pos, rad, mas), color(col) {

	lopta = kreirajLoptu(rad, 48, 48);

	dodajKratere(0.7f);

	recomputeNormals();
}

Planet::~Planet() {}


vector<vec3> Planet::kreirajKruznicu(vec3 centar, float r, int brojTacaka) const
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
vector<vector<vec3>> Planet::kreirajLoptu(float r, int brojTacakaKruznice, int brojKruznica) const
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

void Planet::nacrtaj(GLenum mode, const vector<vec3>& tacke) const
{
	glBegin(mode);
	for (int i = 0; i < (int)tacke.size(); i++)
		glVertex3f(tacke[i].x, tacke[i].y, tacke[i].z);
	glEnd();
}
void Planet::draw() const
{
	glPushMatrix();
	glTranslatef(position.x, position.y, position.z);

	if (lopta.empty()) { glPopMatrix(); return; }

	glShadeModel(GL_SMOOTH);

	// Sačuvaj COLOR_MATERIAL pa privremeno podesi naš materijal
	GLboolean wasColorMat = glIsEnabled(GL_COLOR_MATERIAL);

	// Koristimo ColorMaterial za ambient+diffuse => dovoljno je glColor3f
	if (!wasColorMat) glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// Bazna boja planete
	glColor3f(color.x, color.y, color.z);

	// Mekši specular da ne naglašava facete
	GLfloat ks[] = { 0.35f, 0.35f, 0.35f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ks);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);

	const int R = (int)lopta.size();
	const int C = (int)lopta[0].size();

	// Svaki pojas crtamo kao triangle strip sa glatkim normalama
	for (int i = 0; i < R - 1; ++i)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int j = 0; j <= C; ++j) // <=C da spojimo poslednji sa prvim
		{
			int jj = j % C;

			const vec3& v0 = lopta[i][jj];
			const vec3& v1 = lopta[i + 1][jj];

			const vec3& n0 = normals[i][jj];
			const vec3& n1 = normals[i + 1][jj];

			glNormal3f(n0.x, n0.y, n0.z);
			glVertex3f(v0.x, v0.y, v0.z);

			glNormal3f(n1.x, n1.y, n1.z);
			glVertex3f(v1.x, v1.y, v1.z);
		}
		glEnd();
	}

	// Vrati stanje
	if (!wasColorMat) glDisable(GL_COLOR_MATERIAL);

	glPopMatrix();
}




void Planet::dodajKratere(float udubljenje) {
	const int brojKratera = 20;
	for (int k = 0; k < brojKratera; k++) {
		float fi_kratera = 2.0f * 3.14f * (rand() / (float)RAND_MAX);
		float teta_kratera = 3.14f * (rand() / (float)RAND_MAX);

		vec3 centar_kratera;
		centar_kratera.x = radius * sin(teta_kratera) * cos(fi_kratera);
		centar_kratera.y = radius * sin(teta_kratera) * sin(fi_kratera);
		centar_kratera.z = radius * cos(teta_kratera);
		const float radijus_kratera = 0.7f;

		Krater kr{ centar_kratera, radijus_kratera, udubljenje };
		kratere.push_back(kr);

		for (int i = 0; i < (int)lopta.size(); i++) {
			for (int j = 0; j < (int)lopta[i].size(); j++) {
				vec3* tacka = &lopta[i][j];
				float distanca = length(centar_kratera - (*tacka));
				if (distanca < radijus_kratera) {
					float faktor = 1.0f - (distanca / radijus_kratera);
					vec3 normala = normalize(*tacka);
					*tacka -= normala * udubljenje * faktor * faktor;
				}
			}
		}
	}
}

float Planet::shadeFactor(const vec3& p) const {
	float f = 1.0f;

	for (const auto& kr : kratere) {
		float d = length(p - kr.center);
		if (d < kr.radius) {
			float t = d / kr.radius;

			float lokalno = 0.55f + 0.45f * t;      

			if (lokalno < f) f = lokalno;
		}
	}
	return f;
}

void Planet::recomputeNormals() {
	int R = (int)lopta.size();
	if (R == 0) return;
	int C = (int)lopta[0].size();
	normals.assign(R, std::vector<vec3>(C, vec3(0)));

	auto add = [&](int i, int j, const vec3& n) { normals[i][j] += n; };

	for (int i = 0; i < R - 1; i++) {
		for (int j = 0; j < C; j++) {
			int jn = (j + 1) % C;

			vec3 v00 = lopta[i][j], v01 = lopta[i][jn];
			vec3 v11 = lopta[i + 1][jn], v10 = lopta[i + 1][j];

			vec3 nA = glm::normalize(glm::cross(v01 - v00, v10 - v00));
			vec3 nB = glm::normalize(glm::cross(v11 - v01, v10 - v01));

			add(i, j, nA); add(i, jn, nA); add(i + 1, j, nA);
			add(i, jn, nB); add(i + 1, jn, nB); add(i + 1, j, nB);
		}
	}
	for (int i = 0; i < R; i++)
		for (int j = 0; j < C; j++)
			normals[i][j] = glm::normalize(normals[i][j]);
}

