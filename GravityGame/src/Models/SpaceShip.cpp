#include "SpaceShip.h"
#include "WormHole.h"
#include "Satelit.h" 
#include <unordered_map>

static std::unordered_map<const Planet*, float> g_jamTimers;

using namespace glm;
using namespace std;


static inline void vertexN(const glm::vec3& p) {
    glm::vec3 n = glm::normalize(p);      
    glNormal3f(n.x, n.y, n.z);
    glVertex3f(p.x, p.y, p.z);
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

static bool planetJammed(const Planet* p, const Satelit* sat, float dt)
{
    if (!sat || !p) return false;

    const float range = 20.0f;   
    const float R = p->getRadius();

    glm::vec3 S = sat->getPosition();
    glm::vec3 F = glm::normalize(sat->forward()); 
    glm::vec3 to = p->getPosition() - S;          

    float dist2 = glm::dot(to, to);             
    if (dist2 > (range + R) * (range + R)) {       
        auto it = g_jamTimers.find(p);
        if (it != g_jamTimers.end()) {
            it->second -= dt;
            if (it->second > 0.0f) return true;
            g_jamTimers.erase(it);
        }
        return false;
    }

    const float coneDeg = 20.0f;                 
    const float cosHalf = std::cos(glm::radians(coneDeg));

    float dist = std::sqrt(dist2);
    if (dist <= range) {
        glm::vec3 dir = to / std::max(dist, 1e-6f);  
        if (glm::dot(F, dir) >= cosHalf) {
            g_jamTimers[p] = 2.0f;                   
        }
    }

    auto it = g_jamTimers.find(p);
    if (it != g_jamTimers.end()) {
        it->second -= dt;
        if (it->second > 0.0f) return true;
        g_jamTimers.erase(it);
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
    glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT);

    GLboolean wasColorMat = glIsEnabled(GL_COLOR_MATERIAL);
    if (wasColorMat) glDisable(GL_COLOR_MATERIAL);

    GLfloat kup_diff[] = { 0.22f, 0.32f, 0.75f, 1.0f };
    GLfloat kup_spec[] = { 0.55f, 0.60f, 0.80f, 1.0f };
    GLfloat kup_emit[] = { 0.01f, 0.01f, 0.03f, 1.0f }; 
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, kup_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, kup_spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, kup_emit);

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    drawKupola();

    GLfloat ring_diff[] = { 0.45f, 0.45f, 0.47f, 1.0f };
    GLfloat ring_spec[] = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat no_emit[] = { 0,0,0,1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ring_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ring_spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 80.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, no_emit);
    drawPrsten();
    glPopMatrix();

    if (wasColorMat) glEnable(GL_COLOR_MATERIAL);
    glPopAttrib();
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

void SpaceShip::spojiKruznice(const std::vector<glm::vec3>& A,
    const std::vector<glm::vec3>& B) const
{
    const int n = (int)A.size();
    for (int i = 0; i < n; ++i) {
        const glm::vec3& a = A[i];
        const glm::vec3& b = B[i];
        const glm::vec3& c = B[(i + 1) % n];
        const glm::vec3& d = A[(i + 1) % n];

        glm::vec3 nrm = glm::normalize(glm::cross(b - a, d - a)); 

        glBegin(GL_TRIANGLES);
        glNormal3f(nrm.x, nrm.y, nrm.z); glVertex3f(a.x, a.y, a.z);
        glNormal3f(nrm.x, nrm.y, nrm.z); glVertex3f(b.x, b.y, b.z);
        glNormal3f(nrm.x, nrm.y, nrm.z); glVertex3f(c.x, c.y, c.z);

        glNormal3f(nrm.x, nrm.y, nrm.z); glVertex3f(a.x, a.y, a.z);
        glNormal3f(nrm.x, nrm.y, nrm.z); glVertex3f(c.x, c.y, c.z);
        glNormal3f(nrm.x, nrm.y, nrm.z); glVertex3f(d.x, d.y, d.z);
        glEnd();
    }
}

void SpaceShip::drawKupola() const
{
    GLfloat diff[] = { 0.22f, 0.32f, 0.75f, 1.0f };
    GLfloat spec[] = { 0.65f, 0.70f, 0.85f, 1.0f };
    GLfloat emis[] = { 0.00f, 0.00f, 0.02f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emis);

    const float a = 1.0f, b = 0.5f, c = 1.0f;

    if (lopta.size() < 2) return;

    const int rings = (int)lopta.size();
    const int segs = (int)lopta[0].size();
    const int halfR = rings / 2;

    auto emitEllipsoidVertex = [&](const glm::vec3& ps)
        {
            vec3 p(ps.x * a, ps.y * b, ps.z * c);
            vec3 n(ps.x / (a * a), ps.y / (b * b), ps.z / (c * c));
            n = normalize(n);

            glNormal3f(n.x, n.y, n.z);
            glVertex3f(p.x, p.y, p.z);
        };

    for (int i = 1; i <= halfR; ++i) {
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= segs; ++j) {
            const glm::vec3& a0 = lopta[i - 1][j % segs];
            const glm::vec3& b0 = lopta[i][j % segs];
            emitEllipsoidVertex(a0);
            emitEllipsoidVertex(b0);
        }
        glEnd();
    }

    const int jCap = 0;
    glBegin(GL_TRIANGLE_FAN);
    {
        glm::vec3 ps = lopta[halfR][jCap];
        glm::vec3 p = { ps.x * a, ps.y * b, ps.z * c };
        glm::vec3 n = glm::normalize(glm::vec3(ps.x / (a * a), ps.y / (b * b), ps.z / (c * c)));
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(p.x, p.y, p.z);
    }
    for (int j = 0; j <= segs; ++j) {
        glm::vec3 ps = lopta[halfR - 1][j % segs];
        glm::vec3 n = glm::normalize(glm::vec3(ps.x / (a * a), ps.y / (b * b), ps.z / (c * c)));
        glm::vec3 p = { ps.x * a, ps.y * b, ps.z * c };
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();

    GLfloat zero[] = { 0,0,0,1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
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
    const float R = 0.25f, r = 0.08f;
    const int   M = 48, N = 24;
    auto torus = kreirajTorus(R, r, M, N);

    GLfloat diff[] = { 0.55f, 0.55f, 0.58f, 1.0f };
    GLfloat spec[] = { 1.0f,  1.0f,  1.0f,  1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 96.0f);

    for (int i = 1; i < M; ++i) spojiKruznice(torus[i - 1], torus[i]);
    spojiKruznice(torus[M - 1], torus[0]);

    const int brojLampica = 12;
    const float lampica_r = 0.015f;
    const float lampice_pozicija_R = R + (r / 2);

    GLfloat emitBlue[] = { 0.08f, 0.10f, 0.9f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emitBlue);

    float ugao_rotacije = 2.0f * 3.14159265f / brojLampica;
    for (int i = 0; i < brojLampica; ++i)
    {
        glm::mat4 rot = glm::rotate(i * ugao_rotacije, glm::vec3(0, 1, 0));
        glm::vec3 pos = rot * glm::vec3(lampice_pozicija_R, 0, 0);

        glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);

        auto lamp = kreirajKruznicu(glm::vec3(0, 0, 0), lampica_r, 16);
        glBegin(GL_TRIANGLE_FAN);
        for (auto& v : lamp) vertexN(v);
        glEnd();

        glPopMatrix();
    }

    GLfloat zero[] = { 0,0,0,1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
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

float Jam_GetRemainingFor(const Planet* p) {
    auto it = g_jamTimers.find(p);
    return (it != g_jamTimers.end()) ? it->second : 0.0f;
}

bool SpaceShip::hitsPlanet(const Planet* p) const {
    if (!p) return false;
    const glm::vec3 S = getPosition();
    const glm::vec3 C = p->getPosition();
    const float sumR = p->getRadius() + COLLISION_RADIUS;

    const float dist2 = glm::length(S - C);
    return dist2 <= sumR;
}


bool SpaceShip::hitsAny(const std::vector<Planet*>& planets) const {
    for (const Planet* p : planets) {
        if (hitsPlanet(p)) return true;
    }
    return false;
}


