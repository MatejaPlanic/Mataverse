//g++ -o p main.cpp -lGL -lglut -lGLU

#define GLM_ENABLE_EXPERIMENTAL

#include "./Models/SpaceShip.h"
#include "./Models/Nebo.h"
#include "./Models/Planets/Planet.h"
#include "./Models/WormHole.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp> 

const float PLANET_DEPTH_STEP = 0.8f;
const float PLANET_MIN_DEPTH = 0.0f;   
const float PLANET_MAX_DEPTH = 120.0f; 

Planet* selectedPlanet = nullptr;

const glm::vec3 NEW_PLANET_COLOR(0.2f, 0.7f, 1.0f);
const float     NEW_PLANET_RADIUS = 1.6f;
const float     NEW_PLANET_MASS = 120.0f;
const float NEW_PLANET_FORWARD = 10.0f;


bool placingPlanet = false;


using namespace glm;
using namespace std;

#define MOVING_CONST 0.1
#define ROTATION_CONST 3.14f / 180.f
#define LOOK_MOVEMENT_CONST 0.1f

/*--------------------------------------------------*/

char title[] = "Prozor";
int FPS = 60;
vec3 CameraPosition(1.0, 1.0, 1.0);
vec3 LookAt_vector(0.0, 0.0, 0.0);
vec3 LookUp_vector(0.0, 1.0, 0.0);
vector<vec3> coordinateSystem;
SpaceShip ship;
Nebo nebo;
std::vector<Planet*> planets;
//WormHole(vec3 pos, float rad,float mass, int brojPrstenova, float rastojanjePrstenova);
WormHole wormhole(vec3(0.0f, 0.0f, 0.0f), 2.0f, 1.0f, 10, 0.2f);
const int circle_dots = 50;
const float height = 480;
const float ratio = 16.f / 9.f;
double upDownAngle = 0;
int currentSpeed = 1;
float arrowRotate = 0.f;
float cameraDistance = 3.0f; 
const float minDistance = 1.0f;
const float maxDistance = 10.0f;
const float zoomStep = 0.5f;
int winW = int(height * ratio), winH = int(height);

vec3 operator* (mat4x4 mat, vec3 vec)
{
	vec4 v(vec.x, vec.y, vec.z, 1.f);
	v = mat * v;
	return vec3(v.x, v.y, v.z);
}

vector<vec3> operator* (mat4x4 mat, vector<vec3> vectors)
{
	for (int i = 0; i < vectors.size(); i++)
		vectors[i] = mat * vectors[i];
	return vectors;
}


//o GLUT_BITMAP_TIMES_ROMAN_24
//o GLUT_BITMAP_TIMES_ROMAN_10
//o GLUT_BITMAP_HELVETICA_18

void RenderString(float x, float y, void* font, double r, double g, double b)
{
	glColor3f(r, g, b);
	glRasterPos2f(x, y);

	char s[100];
	sprintf_s(s, "x = %.2lf\ny = %.2lf\nz = %.2lf", CameraPosition.x, CameraPosition.y, CameraPosition.z);
	glutBitmapString(font, (const unsigned char*)s);
}

/*--------------------------------------------------*/
void createCoordinates()
{
	coordinateSystem.resize(4);
	coordinateSystem[0] = vec3(0.0, 0.0, 0.0);
	coordinateSystem[1] = vec3(1.0, 0.0, 0.0);
	coordinateSystem[2] = vec3(0.0, 1.0, 0.0);
	coordinateSystem[3] = vec3(0.0, 0.0, 1.0);
}

void drawCoordinates()
{
	glLineWidth(2.0);
	glBegin(GL_LINES);
	// X axis
	glColor3f(1.0f, 0.0f, 0.0f); // Red
	glVertex3d(coordinateSystem[0].x, coordinateSystem[0].y, coordinateSystem[0].z);
	glVertex3d(coordinateSystem[1].x, coordinateSystem[1].y, coordinateSystem[1].z);
	// Y axis
	glColor3f(0.0f, 1.0f, 0.0f); // Green
	glVertex3d(coordinateSystem[0].x, coordinateSystem[0].y, coordinateSystem[0].z);
	glVertex3d(coordinateSystem[2].x, coordinateSystem[2].y, coordinateSystem[2].z);
	// Z axis
	glColor3f(0.0f, 0.0f, 1.0f); // Blue
	glVertex3d(coordinateSystem[0].x, coordinateSystem[0].y, coordinateSystem[0].z);
	glVertex3d(coordinateSystem[3].x, coordinateSystem[3].y, coordinateSystem[3].z);
	glEnd();
}

static void hudCircleGeometry(float& panelH, float& cx, float& cy, float& rad)
{
	const float pad = 16.0f;
	rad = 12.0f;
	panelH = winH * 0.25f;
	cx = pad + rad;
	cy = panelH - pad - rad; 
}

static void drawFilledCircle2D(float cx, float cy, float r, int segments = 48)
{
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(cx, cy);
	for (int i = 0; i <= segments; ++i) {
		float a = (2.0f * 3.14159265f * i) / segments;
		glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
	}
	glEnd();
}

static void screenToWorldRay(int mx, int myBL, glm::vec3& orig, glm::vec3& dir)
{
	float nx = 2.0f * mx / float(winW) - 1.0f;
	float ny = 2.0f * myBL / float(winH) - 1.0f;

	mat4 P = glm::perspective(glm::radians(80.0f), float(winW) / float(winH), 0.1f, 500.0f);
	mat4 V = glm::lookAt(CameraPosition, LookAt_vector, LookUp_vector);
	mat4 invPV = glm::inverse(P * V);

	vec4 startNDC(nx, ny, -1.0f, 1.0f);
	vec4 endNDC(nx, ny, 1.0f, 1.0f);

	vec4 startW = invPV * startNDC; startW /= startW.w;
	vec4 endW = invPV * endNDC;   endW /= endW.w;

	orig = vec3(startW);
	dir = normalize(glm::vec3(endW - startW));
}

void drawHUD()
{
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, winW, 0, winH, -1, 1);
	glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

	float panelH, cx, cy, rad; hudCircleGeometry(panelH, cx, cy, rad);

	glColor4f(0.12f, 0.12f, 0.12f, 0.85f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0); glVertex2f(winW, 0); glVertex2f(winW, panelH); glVertex2f(0, panelH);
	glEnd();

	glColor3f(0.2f, 0.6f, 1.0f);
	drawFilledCircle2D(cx, cy, rad, 48);

	glColor3f(1, 1, 1);
	float textX = cx + rad + 10.0f;
	float textY = panelH - 16.0f - 6.0f;
	char buf[128];
	sprintf_s(buf, "poluprecnik = %.2f\nmasa = %.2f%s",
		1.60,120.00, placingPlanet ? "\n[klik na scenu za postavljanje]" : "");
	glRasterPos2f(textX, textY);
	glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)buf);

	glPopMatrix(); // MODELVIEW
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
}



void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(80.0, 16.0 / 9.0, 0.1, 500.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		CameraPosition.x, CameraPosition.y, CameraPosition.z,
		LookAt_vector.x, LookAt_vector.y, LookAt_vector.z,
		LookUp_vector.x, LookUp_vector.y, LookUp_vector.z
	);
	//drawCoordinates();
	nebo.draw();
	ship.draw();
	/*wormhole.draw();*/
	for (auto& p : planets) p->draw();
	drawHUD();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glutSwapBuffers();
}

void timer(int v)
{
	static int lastMs = glutGet(GLUT_ELAPSED_TIME);
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	float dt = (nowMs - lastMs) / 1000.0f;
	lastMs = nowMs;


	ship.update(dt, planets);

	glutTimerFunc(1000 / FPS, timer, v);
	glutPostRedisplay();
}

void reshape(GLsizei width, GLsizei heightIn)
{
	if (heightIn * ratio <= width)
		width = ratio * heightIn;
	else
		heightIn = width / ratio;
	glViewport(0, 0, width, heightIn);
	winW = width; winH = heightIn;
}

void PrintVector(vec3 vec)
{
	printf("%.2f %.2f %.2f\n", vec.x, vec.y, vec.z);
}

void MoveForward()
{
	mat4x4 mt;
	vec3 v = LookAt_vector - CameraPosition;
	v = normalize(v);
	v.y = 0.f;
	v = v * (currentSpeed * (float)MOVING_CONST);

	mt = translate(mat4x4(1.f), v);

	LookAt_vector = mt * LookAt_vector;
	CameraPosition = mt * CameraPosition;
}

void MoveBackward()
{
	mat4x4 mt;
	vec3 v = LookAt_vector - CameraPosition;
	v.y = 0.f;
	v = normalize(v);

	v = -v * (currentSpeed * (float)MOVING_CONST);

	mt = translate(mat4x4(1.f), v);
	LookAt_vector = mt * LookAt_vector;
	CameraPosition = mt * CameraPosition;
}

static void nudgeSelectedPlanetDepth(int wheelDirection)
{
	if (!selectedPlanet) return;

	glm::vec3 fwd = glm::normalize(LookAt_vector - CameraPosition);

	glm::vec3 pos = selectedPlanet->getPosition();
	float currDepth = glm::dot(pos - CameraPosition, fwd);
	glm::vec3 lateral = pos - (CameraPosition + fwd * currDepth);

	float delta = (wheelDirection > 0 ? -PLANET_DEPTH_STEP : PLANET_DEPTH_STEP);

	float newDepth = currDepth + delta;
	newDepth = std::max(PLANET_MIN_DEPTH, std::min(PLANET_MAX_DEPTH, newDepth));

	glm::vec3 newPos = CameraPosition + fwd * newDepth + lateral;
	selectedPlanet->setPosition(newPos);
}

void mouseWheel(int wheel, int direction, int x, int y)
{
	int mods = glutGetModifiers();
	bool shiftHeld = (mods & GLUT_ACTIVE_SHIFT);

	if (shiftHeld && selectedPlanet) {
		nudgeSelectedPlanetDepth(direction);
		glutPostRedisplay();
		return;
	}

	if (direction > 0) cameraDistance -= zoomStep;
	else               cameraDistance += zoomStep;

	if (cameraDistance < minDistance) cameraDistance = minDistance;
	else if (cameraDistance > maxDistance) cameraDistance = maxDistance;

	vec3 viewDirection = normalize(CameraPosition - LookAt_vector);
	CameraPosition = LookAt_vector + viewDirection * cameraDistance;

	glutPostRedisplay();
}


void MoveLeft()
{
	mat4x4 mt;
	vec3 f = LookAt_vector - CameraPosition;
	vec3 w = cross(LookUp_vector, f);

	w = normalize(w);
	w = w * (currentSpeed * (float)MOVING_CONST);

	mt = translate(mat4x4(1.f), w);

	CameraPosition = mt * CameraPosition;
	LookAt_vector = mt * LookAt_vector;
}

void MoveRight()
{
	mat4x4 mt;
	vec3 f = LookAt_vector - CameraPosition;
	vec3 w = cross(LookUp_vector, f);

	w = normalize(w);
	w = -w * (currentSpeed * (float)MOVING_CONST);

	mt = translate(mat4x4(1.f), w);

	CameraPosition = mt * CameraPosition;
	LookAt_vector = mt * LookAt_vector;
}

void TurnLeft()
{
	mat4x4 mt, identityMat, mt1, mt2, mtr;

	identityMat = mat4x4(1.f);
	mt1 = translate(identityMat, vec3(CameraPosition.x, CameraPosition.y, CameraPosition.z));
	mtr = rotate(identityMat, ROTATION_CONST, vec3(0.f, 1.f, 0.f));
	mt2 = translate(identityMat, vec3(-CameraPosition.x, -CameraPosition.y, -CameraPosition.z));

	mt = mt1 * mtr * mt2;
	LookAt_vector = mt * LookAt_vector;

	arrowRotate += ROTATION_CONST;
}

void TurnRight()
{
	mat4x4 mt, identityMat, mt1, mt2, mtr;

	identityMat = mat4x4(1.f);
	mt1 = translate(identityMat, vec3(CameraPosition.x, CameraPosition.y, CameraPosition.z));
	mtr = rotate(identityMat, -ROTATION_CONST, vec3(0.f, 1.f, 0.f));
	mt2 = translate(identityMat, vec3(-CameraPosition.x, -CameraPosition.y, -CameraPosition.z));

	mt = mt1 * mtr * mt2;
	LookAt_vector = mt * LookAt_vector;

	arrowRotate -= ROTATION_CONST;
}

void TurnUp()
{
	mat4x4 mt, mt1, mt2, mtr;
	vec3 f = LookAt_vector - CameraPosition;
	vec3 w = cross(f, LookUp_vector);

	w = normalize(w);

	if (upDownAngle + ROTATION_CONST < (float)3.14)
	{
		mt1 = translate(mat4x4(1.f), vec3(-CameraPosition.x, -CameraPosition.y, -CameraPosition.z));
		mtr = rotate(mat4x4(1.f), ROTATION_CONST, w);
		mt2 = translate(mat4x4(1.f), vec3(CameraPosition.x, CameraPosition.y, CameraPosition.z));

		mt = mt2 * mtr * mt1;

		LookAt_vector = mt * LookAt_vector;

		upDownAngle += ROTATION_CONST;
	}
}

void TurnDown()
{
	mat4x4 mt, mt1, mt2, mtr;
	vec3 f = LookAt_vector - CameraPosition;
	vec3 w = cross(f, LookUp_vector);

	w = normalize(w);

	if (upDownAngle - ROTATION_CONST > (float)-3.14)
	{
		mt1 = translate(mat4x4(1.f), vec3(-CameraPosition.x, -CameraPosition.y, -CameraPosition.z));
		mtr = rotate(mat4x4(1.f), -ROTATION_CONST, w);
		mt2 = translate(mat4x4(1.f), vec3(CameraPosition.x, CameraPosition.y, CameraPosition.z));

		mt = mt2 * mtr * mt1;

		LookAt_vector = mt * LookAt_vector;

		upDownAngle -= ROTATION_CONST;
	}
}

void SpeedUp()
{
	if (currentSpeed < 5)
		++currentSpeed;
}

void SpeedDown()
{
	if (currentSpeed > 0)
		--currentSpeed;
}

void initGL(void)
{
	glClearColor(0, 0, 0, 1);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static bool raySphereHit(const glm::vec3& ro, const glm::vec3& rd,
	const glm::vec3& C, float R, float& tHit)
{

	glm::vec3 oc = ro - C;
	float b = 2.0f * glm::dot(oc, rd);
	float c = glm::dot(oc, oc) - R * R;
	float D = b * b - 4.0f * c;    
	if (D < 0.0f) return false;
	float sD = std::sqrt(D);
	float t0 = (-b - sD) * 0.5f;
	float t1 = (-b + sD) * 0.5f;
	float t = t0;
	if (t < 0.0f) t = t1;
	if (t < 0.0f) return false;
	tHit = t;
	return true;
}

static Planet* pickPlanetAt(int mx, int myBL)
{
	glm::vec3 ro, rd;
	screenToWorldRay(mx, myBL, ro, rd);

	Planet* best = nullptr;
	float bestT = 1e9f;

	for (auto* p : planets) {
		float tHit;
		if (raySphereHit(ro, rd, p->getPosition(), p->getRadius(), tHit)) {
			if (tHit < bestT) { bestT = tHit; best = p; }
		}
	}
	return best;
}




void mousePress(int button, int state, int x, int y)
{
	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

	int yBL = winH - y;

	float panelH, cx, cy, rad; hudCircleGeometry(panelH, cx, cy, rad);

	float dx = x - cx;
	float dy = yBL - cy;
	if ((dx * dx + dy * dy) <= rad * rad) {
		placingPlanet = !placingPlanet;
		if (placingPlanet) selectedPlanet = nullptr;
		glutPostRedisplay();
		return;
	}

	if (placingPlanet) {
		if (yBL <= panelH) return; 

		glm::vec3 ro, rd;
		screenToWorldRay(x, yBL, ro, rd);

		glm::vec3 pos = CameraPosition + rd * NEW_PLANET_FORWARD;

		Planet* np = new Planet(pos, NEW_PLANET_COLOR, NEW_PLANET_RADIUS, NEW_PLANET_MASS);
		planets.push_back(np);

		placingPlanet = false;  
		selectedPlanet = np;
		glutPostRedisplay();
		return;
	}
	selectedPlanet = pickPlanetAt(x, yBL);
	glutPostRedisplay();
}


void keyPress(unsigned char key, int x, int y)
{


	switch (key)
	{
	case 27: //ESC key
		exit(0);
		break;
	case 'w':
		MoveForward();
		break;
	case 's':
		MoveBackward();
		break;
	case 'a':
		//FUNCTION
		MoveLeft();
		break;
	case 'd':
		//FUNCTION
		MoveRight();
		break;
	case 'u':
		SpeedUp();
		break;
	case 'j':

		SpeedDown();
		break;
	case 52:

		TurnLeft();
		break;
	case 54:

		TurnRight();
		break;
	case 50:
		TurnDown();
		break;
	case 56:
		TurnUp();
		break;
	}

}

/*--------------------------------------------------*/

int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(height * ratio, height);
	glutInitWindowPosition(150, 50);
	glutCreateWindow(title);
	createCoordinates();
	glutDisplayFunc(display);
	glutTimerFunc(100, timer, 0);
	glutReshapeFunc(reshape);

	glutMouseFunc(mousePress);
	glutMouseWheelFunc(mouseWheel);
	glutKeyboardFunc(keyPress);

	initGL();
	glutMainLoop();

	return 0;
}
