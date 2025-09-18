#include"WormHole.h"

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

WormHole::WormHole(vec3 pos, float rad, float mass, int brojPrstenova, float rastojanjePrstenova) : NebeskoTelo(pos, rad, mass), brojPrstenova(brojPrstenova), rastojanjePrstenova(rastojanjePrstenova) {
}

WormHole::~WormHole() {}

void WormHole::nacrtaj(GLenum mode, const vector<vec3>& tacke) const
{
	glBegin(mode);
	for (int i = 0; i < (int)tacke.size(); i++)
		glVertex3f(tacke[i].x, tacke[i].y, tacke[i].z);
	glEnd();
}

void WormHole::spojiKruznice(const vector<vec3>& prva, const vector<vec3>& druga) const
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

vector<vec3> WormHole::kreirajKruznicu(vec3 centar, float r, int brojTacaka) const
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

vector<vector<vec3>> WormHole::kreirajTorus(float R, float r, int slicesMajor, int slicesMinor) const
{
    vector<vector<vec3>> torus;
    if (slicesMajor < 3 || slicesMinor < 3) return torus;

    torus.resize(slicesMajor);

    torus[0] = kreirajKruznicu(vec3(R, -0.02, 0), r, slicesMinor);

    float fi = 2.0f * 3.14f / (float)slicesMajor;
    mat4x4 rotZ = rotate(fi, vec3(0, 0, 1));
    for (int i = 1; i < slicesMajor; ++i)
        torus[i] = rotZ * torus[i - 1];

    return torus;
}

void WormHole::drawPrsten() const
{
    const int   M = 24;
    const int   N = 24;
    const float r = radius / 4.0f;                 
    const float dR = radius / float(brojPrstenova); 

    float red = 0.0f, green = 1.0f, blue = 1.0f;

    for (int i = 0; i < brojPrstenova; ++i)
    {
        float R = dR * (i + 1); 

        auto torus = kreirajTorus(R, r, M, N);


        float zOffset = i * rastojanjePrstenova;

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, zOffset);   

        glColor3f(red, green, blue);

        for (int j = 1; j < M; ++j)
            spojiKruznice(torus[j - 1], torus[j]);
        spojiKruznice(torus[M - 1], torus[0]);

        glPopMatrix();

        red = std::min(red + float((1.0f/float(brojPrstenova))), 1.0f);
    }
}


void WormHole::draw() const
{
	glPushMatrix();
	glTranslatef(position.x, position.y, position.z);
	drawPrsten();
	glPopMatrix();
}