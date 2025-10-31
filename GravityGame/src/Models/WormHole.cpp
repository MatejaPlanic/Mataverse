#include "WormHole.h"

#include <vector>
#include <cmath>
#include <algorithm>

#include <GL/glut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp> 
using std::vector;
using glm::vec3;
using glm::mat4;


static inline glm::vec3 triNormal(const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c) {
    return glm::normalize(glm::cross(b - a, c - a));
}

static void setWormholeMaterial()
{
    // Mek, „svemirski“ materijal sa malo emisije
    GLfloat diff[] = { 0.30f, 0.45f, 0.90f, 1.0f };
    GLfloat spec[] = { 0.80f, 0.85f, 0.95f, 1.0f };
    GLfloat emis[] = { 0.03f, 0.04f, 0.08f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emis);
}


static vec3 operator*(mat4 mat, vec3 v) {
    glm::vec4 w(v, 1.f);
    w = mat * w;
    return vec3(w);
}
static vector<vec3> operator*(mat4 mat, vector<vec3> vectors) {
    for (int i = 0; i < (int)vectors.size(); i++)
        vectors[i] = mat * vectors[i];
    return vectors;
}

WormHole::WormHole(vec3 pos, float rad, float mass, int brojPrstenova, float rastojanjePrstenova)
    : NebeskoTelo(pos, rad, mass),
    brojPrstenova(brojPrstenova),
    rastojanjePrstenova(rastojanjePrstenova) {
}

WormHole::~WormHole() {}

void WormHole::nacrtaj(GLenum mode, const vector<vec3>& tacke) const {
    glBegin(mode);
    for (int i = 0; i < (int)tacke.size(); i++)
        glVertex3f(tacke[i].x, tacke[i].y, tacke[i].z);
    glEnd();
}
void WormHole::spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const {
    vector<vec3> poligon(4);
    int n = (int)prva.size();
    for (int i = 0; i < n - 1; i++) {
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
vector<vec3> WormHole::kreirajKruznicu(vec3 centar, float r, int brojTacaka) const {
    vector<vec3> kruznica;
    kruznica.resize(brojTacaka);

    float fi = 2.0f * 3.14159265f / (float)brojTacaka;

    mat4 rotY = glm::rotate(mat4(1.0f), fi, vec3(0, 1, 0));
    mat4 T = glm::translate(mat4(1.0f), centar);

    kruznica[0] = vec3(r, 0, 0);
    for (int i = 1; i < brojTacaka; i++)
        kruznica[i] = rotY * kruznica[i - 1];

    for (int i = 0; i < brojTacaka; i++)
        kruznica[i] = T * kruznica[i];

    return kruznica;
}


static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

static vector<vec3> makeRingLocal(float ringRadius, float z, int segments) {
    vector<vec3> ring(segments);
    const float dA = 2.0f * 3.14159265f / float(segments);
    float a = 0.0f;
    for (int i = 0; i < segments; ++i, a += dA) {
        ring[i] = vec3(std::cos(a) * ringRadius, std::sin(a) * ringRadius, z);
    }
    return ring;
}

static void connectRingsLit(const std::vector<glm::vec3>& A,
    const std::vector<glm::vec3>& B)
{
    int N = (int)A.size();
    if (N < 3 || (int)B.size() != N) return;

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < N; ++i) {
        int j = (i + 1) % N;

        // dva trougla po segmentu: (A[i], B[i], B[j]) i (A[i], B[j], A[j])
        glm::vec3 n1 = triNormal(A[i], B[i], B[j]);
        glNormal3f(n1.x, n1.y, n1.z);
        glVertex3f(A[i].x, A[i].y, A[i].z);
        glVertex3f(B[i].x, B[i].y, B[i].z);
        glVertex3f(B[j].x, B[j].y, B[j].z);

        glm::vec3 n2 = triNormal(A[i], B[j], A[j]);
        glNormal3f(n2.x, n2.y, n2.z);
        glVertex3f(A[i].x, A[i].y, A[i].z);
        glVertex3f(B[j].x, B[j].y, B[j].z);
        glVertex3f(A[j].x, A[j].y, A[j].z);
    }
    glEnd();
}

static inline glm::mat4 lookRotation(const glm::vec3& dirIn,
    const glm::vec3& upHint = glm::vec3(0, 1, 0))
{
    glm::vec3 f = glm::normalize(dirIn);
    glm::vec3 up = upHint;

    if (std::abs(glm::dot(f, up)) > 0.999f) {
        up = std::abs(f.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    }

    glm::vec3 r = glm::normalize(glm::cross(up, f)); 
    up = glm::normalize(glm::cross(f, r));           

    glm::mat4 R(1.0f);
    R[0] = glm::vec4(r, 0.0f);
    R[1] = glm::vec4(up, 0.0f);
    R[2] = glm::vec4(f, 0.0f);
    return R;
}

void WormHole::orientTowardOnce(const glm::vec3& targetPos)
{
    glm::vec3 dir = targetPos - position;
    if (glm::length2(dir) < 1e-12f) {
        m_rotation = glm::mat4(1.0f);  
    }
    else {
        m_rotation = lookRotation(glm::normalize(dir), glm::vec3(0, 1, 0));
    }
    m_hasRotation = true;
}

void WormHole::draw() const
{
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    if (m_hasRotation) glMultMatrixf(glm::value_ptr(m_rotation));

    const int   N = 32;      // više segmenata = glađe
    const int   M = 64;
    const float RADIUS = getRadius();
    const float L = RADIUS * 1.6f;
    const float rMin = std::max(0.14f * RADIUS, 0.18f);
    const float rMax = RADIUS;
    const float power = 2.2f;

    auto colorForU = [](float uAbs) {
        // hladna unutra, svetlije ka ustima
        float R = lerp(0.20f, 0.85f, uAbs);
        float G = lerp(0.30f, 0.95f, uAbs);
        float B = lerp(0.65f, 1.00f, uAbs);
        return glm::vec3(R, G, B);
        };

    std::vector<std::vector<glm::vec3>> rings(M + 1);
    for (int i = 0; i <= M; ++i) {
        float u = (float(i) / float(M)) * 2.0f - 1.0f; // [-1..1]
        float z = u * L;
        float k = std::pow(std::abs(u), power);
        float r = lerp(rMin, rMax, k);
        rings[i] = makeRingLocal(r, z, N);
    }

    // === PASS 1: osvetljeno telo (solid) ===
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    setWormholeMaterial();

    for (int i = 1; i <= M; ++i) {
        float uMid = (((i - 0.5f) / float(M)) * 2.0f - 1.0f);
        glm::vec3 c = colorForU(std::abs(uMid));
        glColor3f(c.r, c.g, c.b);
        connectRingsLit(rings[i - 1], rings[i]);
    }

    // Lagano "svetleća" kapa na oba kraja (disk sa emisijom+alfa)
    auto drawCap = [&](const std::vector<glm::vec3>& ring, bool front) {
        // prozirna kapa
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GLfloat emis[] = { 0.35f, 0.55f, 1.0f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emis);

        glBegin(GL_TRIANGLE_FAN);
        // centar
        glm::vec3 center = ring[0];
        center.x = center.y = 0.0f; // centar prstena u ravni
        glNormal3f(0, 0, front ? -1 : 1);
        glColor4f(0.9f, 0.95f, 1.0f, 0.20f);
        glVertex3f(center.x, center.y, ring[0].z);

        // obod
        glColor4f(0.8f, 0.9f, 1.0f, 0.05f);
        for (int k = 0; k <= (int)ring.size(); ++k) {
            const auto& v = ring[k % ring.size()];
            glNormal3f(0, 0, front ? -1 : 1);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();

        GLfloat zero[] = { 0,0,0,1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
        glDisable(GL_BLEND);
        };
    drawCap(rings[0], true);
    drawCap(rings[M], false);

    // === PASS 2: „žičani“ grid preko (bez treperenja) ===
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f); // malo pomeri da ne z-bleeduju linije
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);
    glColor4f(0.1f, 0.15f, 0.25f, 0.55f);

    for (int i = 0; i <= M; ++i) {
        glBegin(GL_LINE_LOOP);
        for (int k = 0; k < N; ++k)
            glVertex3f(rings[i][k].x, rings[i][k].y, rings[i][k].z);
        glEnd();
    }

    const int p = 6;
    for (int k = 0; k < N; k += p) {
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= M; ++i)
            glVertex3f(rings[i][k].x, rings[i][k].y, rings[i][k].z);
        glEnd();
    }

    glPopAttrib();
    glPopMatrix();
}

