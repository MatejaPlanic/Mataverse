#include "SpaceShip.h"
#include "WormHole.h"
#include "Satelit.h" 
#include <unordered_map>

static std::unordered_map<const Planet*, float> g_jamTimers;

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

static bool planetJammed(const Planet* p, const Satelit* sat, float dt)
{
    if (!sat || !p) return false;

    const float coneDeg = 22.0f;
    const float range = 20.0f;
    const float cosHalf = std::cos(glm::radians(coneDeg * 0.5f));

    glm::vec3 S = sat->getPosition();
    glm::vec3 F = glm::normalize(sat->forward());
    glm::vec3 to = p->getPosition() - S;
    float dist = glm::length(to);

    if (dist > 0.0f && dist <= range) {
        glm::vec3 dir = to / dist;
        float cosang = glm::dot(F, dir);
        if (cosang >= cosHalf) {
            g_jamTimers[p] = 2.0f;
        }
    }

    auto it = g_jamTimers.find(p);
    if (it != g_jamTimers.end()) {
        it->second -= dt;
        if (it->second > 0.0f) return true; 
        else g_jamTimers.erase(it);         
    }

    return false;
}

SpaceShip::SpaceShip()
{
    lopta = kreirajLoptu(0.2f, 64, 64);
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

    if (!lopta.empty())
        nacrtaj(GL_POLYGON, lopta[0]);

    int n = (int)lopta.size();
    glColor3f(0.2f, 0.2f, 0.7f);
    for (int i = 1; i < n; i++)
    {
        
        spojiKruznice(lopta[i - 1], lopta[i]);
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

    glColor3f(0.4f, 0.4f, 0.4f);
    for (int i = 1; i < M; ++i)
        spojiKruznice(torus[i - 1], torus[i]);

    spojiKruznice(torus[M - 1], torus[0]);

    const int brojLampica = 12; 
    const float lampica_r = 0.015f;

    glColor3f(0.0f, 0.0f, 1.0f);

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

void SpaceShip::update(float dt, const std::vector<Planet*>& planets, const WormHole* wormhole, const Satelit* sat)
{
    const float G = 1.0f;              
    const float softening2 = 0.25f;       
    const float maxAccel = 50.0f;        

    vec3 a(0.0f);
    for (auto* p : planets)
    {
        vec3 r = p->getPosition() - position; 
        float dist2 = glm::dot(r, r) + softening2; 
        float invDist = 1.0f / sqrtf(dist2);
        float invDist3 = invDist * invDist * invDist;   
        float mass = p->getMass();

        if (planetJammed(p, sat, dt)) {
            mass = 0.0f;
        }
        a += (G * mass) * r * invDist3;
    }

    float aLen = glm::length(a);
    if (aLen > maxAccel) a *= (maxAccel / aLen);

    if (wormhole)
    {
        vec3 rw = wormhole->getPosition() - position;
        float dw = glm::length(rw);
        float R = wormhole->getRadius();

        const float outer = 3.0f * R;   
        const float inner = 0.60f * R;  
        if (dw < outer)
        {
            vec3 dir = rw / dw; 

            float t = 1.0f - clamp((dw - inner) / (outer - inner), 0.0f, 1.0f);

            float suctionAccel = 120.0f * (0.2f + 0.8f * t); 

            a += dir * suctionAccel;
        }

        if (dw < inner)
        {
            position = wormhole->getPosition();
            velocity = vec3(0.0f);
			shipCaptured = true;
        }
    }

    const float maxAccelWithSuction = 150.0f;
    aLen = glm::length(a);
    if (aLen > maxAccelWithSuction) a *= (maxAccelWithSuction / aLen);

    velocity += a * dt;
    position += velocity * dt;

    for (auto* p : planets) {
        
        glm::vec3 r = position - p->getPosition();
        float d = glm::length(r);
        if (d < p->getRadius()) {
            glm::vec3 n = r / d;
            position = p->getPosition() + n * p->getRadius();
            velocity = glm::vec3(0); 
        }
    }

}


