#include "Planet.h"


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

	lopta = kreirajLoptu(rad, 32, 32);

	dodajKratere(0.7f);
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
	glShadeModel(GL_SMOOTH); 

	if (lopta.empty()) { glPopMatrix(); return; }

	const int prstenova = (int)lopta.size();
	const int poPrstenu = (int)lopta[0].size();

	for (int i = 0; i < prstenova - 1; i++) {
		glBegin(GL_QUADS);
		for (int j = 0; j < poPrstenu; j++) {
			int jn = (j + 1) % poPrstenu;

			const vec3& v00 = lopta[i][j];
			const vec3& v01 = lopta[i][jn];
			const vec3& v11 = lopta[i + 1][jn];
			const vec3& v10 = lopta[i + 1][j];

			float f00 = shadeFactor(v00);
			float f01 = shadeFactor(v01);
			float f11 = shadeFactor(v11);
			float f10 = shadeFactor(v10);

			glColor3f(color.x * f00, color.y * f00, color.z * f00);
			glVertex3f(v00.x, v00.y, v00.z);

			glColor3f(color.x * f01, color.y * f01, color.z * f01);
			glVertex3f(v01.x, v01.y, v01.z);

			glColor3f(color.x * f11, color.y * f11, color.z * f11);
			glVertex3f(v11.x, v11.y, v11.z);

			glColor3f(color.x * f10, color.y * f10, color.z * f10);
			glVertex3f(v10.x, v10.y, v10.z);
		}
		glEnd();
	}

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

