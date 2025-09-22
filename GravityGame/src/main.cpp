//g++ -o p main.cpp -lGL -lglut -lGLU

#define GLM_ENABLE_EXPERIMENTAL

#include "./Models/SpaceShip.h"
#include "./Models/Nebo.h"
#include "./Models/Planets/Planet.h"
#include "./Models/WormHole.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp> 
#include <fstream>
#include <sstream>
#include <string>

using namespace glm;
using namespace std;

const float PLANET_DEPTH_STEP = 0.8f;
const float PLANET_MIN_DEPTH = 0.0f;   
const float PLANET_MAX_DEPTH = 120.0f; 

Planet* selectedPlanet = nullptr;

string g_currentLevelPath = "src/Levels/Level1.txt";
int   g_maxPlaceablePlanets = 0;     
int   g_placedThisLevel = 0;     

vec3 g_newPlanetColor = glm::vec3(0.2f, 0.7f, 1.0f);
float     g_newPlanetRadius = 1.6f;
float     g_newPlanetMass = 120.0f;
float     g_newPlanetForward = 10.0f;

WormHole* g_wormhole = nullptr;


bool placingPlanet = false;




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
bool simulationActive = false;
const float PLANET_MOVE_STEP = 0.5f;

static inline std::string trim(const std::string& s) {
	size_t b = s.find_first_not_of(" \t\r\n");
	if (b == std::string::npos) return "";
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}

static bool parseColor(const std::string& csv, glm::vec3& out) {
	std::istringstream ss(csv);
	std::string tok;
	float vals[3]; int i = 0;
	while (std::getline(ss, tok, ',')) {
		if (i >= 3) break;
		vals[i++] = std::stof(trim(tok));
	}
	if (i != 3) return false;
	out = glm::vec3(vals[0], vals[1], vals[2]);
	return true;
}

bool loadLevelConfig(const std::string& path) {
	std::ifstream f(path);
	if (!f) {
		fprintf(stderr, "[LEVEL] Ne mogu da otvorim: %s\n", path.c_str());
		return false;
	}

	int lvlRead = -1;
	int available = 0;
	vec3 color = g_newPlanetColor;
	float rad = g_newPlanetRadius;
	float mass = g_newPlanetMass;
	float fwd = g_newPlanetForward;
	vec3 whPos = glm::vec3(0.0f, 0.0f, 0.0f);
	float whRadius = 2.0f;
	float whMass = 1.0f;
	int   whRings = 10;
	float whSpacing = 0.2f;

	std::string line;
	while (std::getline(f, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#') continue;
		size_t eq = line.find('=');
		if (eq == std::string::npos) continue;

		std::string key = trim(line.substr(0, eq));
		std::string val = trim(line.substr(eq + 1));

		if (key == "LEVEL") {
			lvlRead = std::stoi(val);
		}
		else if (key == "AVAILABLE_PLANETS") {
			available = std::stoi(val);
		}
		else if (key == "PLANET_TEMPLATE_COLOR") {
			parseColor(val, color);
		}
		else if (key == "PLANET_TEMPLATE_RADIUS") {
			rad = std::stof(val);
		}
		else if (key == "PLANET_TEMPLATE_MASS") {
			mass = std::stof(val);
		}
		else if (key == "PLANET_TEMPLATE_FORWARD") {
			fwd = std::stof(val);
		}
		else if (key == "WORMHOLE_POS") {
			parseColor(val, whPos);
		}
		else if (key == "WORMHOLE_RADIUS") {
			whRadius = std::stof(val);
		}
		else if (key == "WORMHOLE_MASS") {
			whMass = std::stof(val);
		}
		else if (key == "WORMHOLE_RINGS") {
			whRings = std::stoi(val);
		}
		else if (key == "WORMHOLE_RING_SPACING") {
			whSpacing = std::stof(val);
		}
	}

	g_maxPlaceablePlanets = std::max(0, available);
	g_newPlanetColor = color;
	g_newPlanetRadius = rad;
	g_newPlanetMass = mass;
	g_newPlanetForward = fwd;

	g_placedThisLevel = 0;

	g_wormhole = new WormHole(whPos, whRadius, whMass, whRings, whSpacing);
	printf("[LEVEL] Wormhole @ (%.2f, %.2f, %.2f), R=%.2f, M=%.2f, rings=%d, spacing=%.2f\n",
		whPos.x, whPos.y, whPos.z, whRadius, whMass, whRings, whSpacing);

	printf("[LEVEL] Učitan level (max planets=%d, color=%.2f,%.2f,%.2f, R=%.2f, M=%.2f, F=%.2f)\n",
		g_maxPlaceablePlanets,
		g_newPlanetColor.r, g_newPlanetColor.g, g_newPlanetColor.b,
		g_newPlanetRadius, g_newPlanetMass, g_newPlanetForward);

	return true;
}


static void hudSimButtonGeometry(float panelH, float& bx0, float& by0, float& bx1, float& by1) {
	const float pad = 16.0f;
	const float btnH = 38.0f;
	const float btnW = 150.0f;
	by1 = panelH - pad;       
	by0 = by1 - btnH;         
	bx1 = winW - pad;        
	bx0 = bx1 - btnW;         
}

static void clearPlanets() {
	for (auto* p : planets) delete p;
	planets.clear();
	selectedPlanet = nullptr;
}

static void stopAndReset() {
	simulationActive = false;
	placingPlanet = false;
	selectedPlanet = nullptr;

	clearPlanets();
	g_placedThisLevel = 0;

	CameraPosition = vec3(1.0f, 1.0f, 1.0f);
	LookAt_vector = vec3(0.0f, 0.0f, 0.0f);
	LookUp_vector = vec3(0.0f, 1.0f, 0.0f);
	upDownAngle = 0.0;
	currentSpeed = 1;
	arrowRotate = 0.f;
	cameraDistance = 3.0f;

	ship = SpaceShip();

	if (g_wormhole) { delete g_wormhole; g_wormhole = nullptr; }
	loadLevelConfig(g_currentLevelPath);
}

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

void RenderString(float x, float y, void* font, double r, double g, double b)
{
	glColor3f(r, g, b);
	glRasterPos2f(x, y);

	char s[100];
	sprintf_s(s, "x = %.2lf\ny = %.2lf\nz = %.2lf", CameraPosition.x, CameraPosition.y, CameraPosition.z);
	glutBitmapString(font, (const unsigned char*)s);
}

/*--------------------------------------------------*/
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
	char buf[256];
	sprintf_s(
		buf,
		"poluprecnik = %.2f\nmasa = %.2f%s\npreostalo: %d/%d",
		g_newPlanetRadius,
		g_newPlanetMass,
		placingPlanet ? "\n[klik na scenu za postavljanje]" : "",
		std::max(0, g_maxPlaceablePlanets - g_placedThisLevel),
		g_maxPlaceablePlanets
	);
	glRasterPos2f(textX, textY);
	glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)buf);

	float bx0, by0, bx1, by1;
	hudSimButtonGeometry(panelH, bx0, by0, bx1, by1);

	if (simulationActive) glColor4f(0.10f, 0.55f, 0.20f, 0.95f);   
	else                  glColor4f(0.35f, 0.35f, 0.35f, 0.95f);  

	glBegin(GL_QUADS);
	glVertex2f(bx0, by0); glVertex2f(bx1, by0); glVertex2f(bx1, by1); glVertex2f(bx0, by1);
	glEnd();

	glColor3f(1, 1, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2f(bx0, by0); glVertex2f(bx1, by0); glVertex2f(bx1, by1); glVertex2f(bx0, by1);
	glEnd();

	const char* label = simulationActive ? "RESET" : "SIMULATE";
	float tx = bx0 + 14.0f;
	float ty = by0 + 11.0f;
	glRasterPos2f(tx, ty);
	glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)label);

	glPopMatrix(); 
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
	nebo.draw();
	ship.draw();
	if (g_wormhole) g_wormhole->draw();
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

	if (simulationActive) {
		ship.update(dt, planets,g_wormhole);
		if (ship.shipCaptured)
		{
			simulationActive = false;
			stopAndReset();
			printf("[GAME] Brod je uspešno stigao do crvotočine! Čestitamo!\n");
		}
	}

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

static void moveSelectedPlanetXY(float dx, float dy)
{
	if (!selectedPlanet) return;
	glm::vec3 pos = selectedPlanet->getPosition();
	pos += glm::vec3(dx, dy, 0.0f);
	selectedPlanet->setPosition(pos);
}

void mouseWheel(int wheel, int direction, int x, int y)
{
	int mods = glutGetModifiers();
	bool shiftHeld = (mods & GLUT_ACTIVE_SHIFT);

	if (shiftHeld && selectedPlanet) {
		if (simulationActive) return;
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

void specialKeyPress(int key, int x, int y)
{
	if (simulationActive) return;
	int mods = glutGetModifiers();
	float step = PLANET_MOVE_STEP;
	if (mods & GLUT_ACTIVE_SHIFT) step *= 0.25f;
	if (mods & GLUT_ACTIVE_CTRL)  step *= 3.0f; 

	switch (key)
	{
	case GLUT_KEY_LEFT:  
		moveSelectedPlanetXY(-step, 0.0f);
		break;
	case GLUT_KEY_RIGHT:  
		moveSelectedPlanetXY(+step, 0.0f);
		break;
	case GLUT_KEY_UP:     
		moveSelectedPlanetXY(0.0f, +step);
		break;
	case GLUT_KEY_DOWN:   
		moveSelectedPlanetXY(0.0f, -step);
		break;
	default:
		return;
	}

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

	float bx0, by0, bx1, by1;
	hudSimButtonGeometry(panelH, bx0, by0, bx1, by1);

	if (x >= bx0 && x <= bx1 && yBL >= by0 && yBL <= by1) {
		if (simulationActive) {
			stopAndReset();          
		}
		else {
			simulationActive = true;
		}
		glutPostRedisplay();
		return;
	}

	float dx = x - cx;
	float dy = yBL - cy;
	if ((dx * dx + dy * dy) <= rad * rad) {
		if (g_placedThisLevel < g_maxPlaceablePlanets) {
			placingPlanet = !placingPlanet;
			if (placingPlanet) selectedPlanet = nullptr;
		}
		else {
			placingPlanet = false;
		}
		glutPostRedisplay();
		return;
	}

	if (placingPlanet) {
		if (yBL <= panelH) return; 

		if (g_placedThisLevel >= g_maxPlaceablePlanets) {
			placingPlanet = false;
			glutPostRedisplay();
			return;
		}

		vec3 ro, rd;
		screenToWorldRay(x, yBL, ro, rd);

		vec3 pos = CameraPosition + rd * g_newPlanetForward;

		Planet* np = new Planet(pos, g_newPlanetColor, g_newPlanetRadius, g_newPlanetMass);
		planets.push_back(np);
		g_placedThisLevel++;
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
	case ' ':
		if (simulationActive) {
			stopAndReset();          
		}
		else {
			simulationActive = true;
		}
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
	glutDisplayFunc(display);
	glutTimerFunc(100, timer, 0);
	glutReshapeFunc(reshape);

	glutMouseFunc(mousePress);
	glutMouseWheelFunc(mouseWheel);
	glutKeyboardFunc(keyPress);
	glutSpecialFunc(specialKeyPress);

	initGL();
	loadLevelConfig(g_currentLevelPath);
	glutMainLoop();

	return 0;
}
