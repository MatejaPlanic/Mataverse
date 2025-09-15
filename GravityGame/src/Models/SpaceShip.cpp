#include "SpaceShip.h"
using namespace glm;
using namespace std;

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

SpaceShip::SpaceShip()
{
    lopta = kreirajLoptu(0.2f, 64, 64);
    nebo = kreirajLoptu(20.0f, 64, 64);

    const int brojZvezda = 300;
    zvezde.resize(brojZvezda);
    for (int i = 0; i < brojZvezda; ++i) {
        float fi = 2.0f * 3.14f * (rand() / (float)RAND_MAX);
        float teta = 3.14f * (rand() / (float)RAND_MAX);
        float r = 19.5f; 
        zvezde[i].x = r * sin(teta) * cos(fi);
        zvezde[i].y = r * sin(teta) * sin(fi);
        zvezde[i].z = r * cos(teta);
    }
}

SpaceShip::~SpaceShip() {}

void SpaceShip::draw() const
{
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    drawKupola();
    drawPrsten();
    glPopMatrix();
}


vector<vec3> SpaceShip::kreirajKruznicu(vec3 centar, float r, int brojTacaka) const
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

vector<vector<vec3>> SpaceShip::kreirajLoptu(float r, int brojTacakaKruznice, int brojKruznica) const
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

void SpaceShip::nacrtaj(GLenum mode, const vector<vec3>& tacke) const
{
    glBegin(mode);
    for (int i = 0; i < (int)tacke.size(); i++)
        glVertex3f(tacke[i].x, tacke[i].y, tacke[i].z);
    glEnd();
}

void SpaceShip::spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const
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

void SpaceShip::drawKupola() const
{
	glPushMatrix();
    glScalef(1.0f, 0.5f, 1.0f);
    float r = 0.1f, g = 0.1f, b = 0.1f;

    if (!lopta.empty())
        nacrtaj(GL_POLYGON, lopta[0]);

    int n = (int)lopta.size();
    for (int i = 1; i < n; i++)
    {
        glColor3f(r, g, b);
        spojiKruznice(lopta[i - 1], lopta[i]);
        r += 0.001f; g += 0.001f; b += 0.001f;
    }
	glPopMatrix();
}

vector<vector<vec3>> SpaceShip::kreirajTorus(float R, float r, int slicesMajor, int slicesMinor) const
{
    vector<vector<vec3>> torus;
    if (slicesMajor < 3 || slicesMinor < 3) return torus;

    torus.resize(slicesMajor);

    torus[0] = kreirajKruznicu(vec3(R, -0.02, 0), r, slicesMinor);

    float fi = 2.0f * 3.14f / (float)slicesMajor;
    mat4x4 rotY = rotate(fi, vec3(0, 1, 0));
    for (int i = 1; i < slicesMajor; ++i)
        torus[i] = rotY * torus[i - 1];

    return torus;
}


void SpaceShip::drawPrsten() const
{
    const float R = 0.25f;
    const float r = 0.08f;
    const int   M = 48;
    const int   N = 24;
    auto torus = kreirajTorus(R, r, M, N);

    glColor3f(0.15f, 0.95f, 0.40f);
    for (int i = 1; i < M; ++i)
        spojiKruznice(torus[i - 1], torus[i]);

    spojiKruznice(torus[M - 1], torus[0]);

    const int brojLampica = 12; 
    const float lampica_r = 0.015f;

    glColor3f(1.0f, 0.0f, 0.0f);

    glPushMatrix();
    const float lampice_pozicija_R = R + (r/2);

    float ugao_rotacije = 2.0f * 3.14f / brojLampica;

    for (int i = 0; i < brojLampica; i++)
    {
        glPushMatrix();

        mat4x4 rotacija = rotate(i * ugao_rotacije, vec3(0, 1, 0));
        vec3 pozicija = rotacija * vec3(lampice_pozicija_R, 0, 0);

        glTranslatef(pozicija.x, pozicija.y, pozicija.z);

        vector<vec3> lampica = kreirajKruznicu(vec3(0, 0, 0), lampica_r, 16);
        nacrtaj(GL_POLYGON, lampica);

        glPopMatrix(); 
    }

    glPopMatrix(); 
}

void SpaceShip::drawNebo() const
{
    glDisable(GL_DEPTH_TEST);
   

    glColor3f(0.0f, 0.0f, 0.0f); 
    int n = (int)nebo.size();
    for (int i = 1; i < n; i++)
    {
        spojiKruznice(nebo[i - 1], nebo[i]);
    }
    spojiKruznice(nebo[n - 1], nebo[0]);

    glPointSize(2.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POINTS);
    for (const auto& zvezda : zvezde) {
        glVertex3f(zvezda.x, zvezda.y, zvezda.z);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST); 
}
