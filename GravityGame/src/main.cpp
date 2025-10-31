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
#include "./Models/Satelit.h"
#include "./Models/Pulsar.h"
#include <glm/gtc/type_ptr.hpp> 


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static inline void setPointerCursor(bool isPointer)
{
#ifdef _WIN32
	SetCursor(LoadCursor(NULL, isPointer ? IDC_HAND : IDC_ARROW));
#else
#ifdef FREEGLUT
	glutSetCursor(isPointer ? GLUT_CURSOR_RIGHT_ARROW : GLUT_CURSOR_LEFT_ARROW);
#endif
#endif
}

using namespace glm;
using namespace std;

const float PLANET_DEPTH_STEP = 0.8f;
const float PLANET_MIN_DEPTH = 0.0f;   
const float PLANET_MAX_DEPTH = 120.0f; 

Planet* selectedPlanet = nullptr;

bool  g_orbiting = false;
int   g_lastX = 0;
float g_orbitAngle = 0.0f;     
float g_orbitRadius = 3.0f;    
float g_camHeight = 1.0f;

string g_currentLevelPath = "src/Levels/Level1.txt";
int   g_maxPlaceablePlanets = 0;     
int   g_placedThisLevel = 0;     

vec3 g_newPlanetColor = glm::vec3(0.2f, 0.7f, 1.0f);
float     g_newPlanetRadius = 1.6f;
float     g_newPlanetMass = 120.0f;
float     g_newPlanetForward = 10.0f;

WormHole* g_wormhole = nullptr;
Satelit* g_sat = nullptr;
Satelit* g_sat2 = nullptr;
Planet* g_planetPlaced = nullptr;
Pulsar* g_pulsar = nullptr;

enum class GameState { TITLE, RUNNING };
GameState g_state = GameState::TITLE;

static float g_btnX0, g_btnY0, g_btnX1, g_btnY1;
static float g_btnStartX0, g_btnStartY0, g_btnStartX1, g_btnStartY1;
static float g_btnExitX0, g_btnExitY0, g_btnExitX1, g_btnExitY1;
static bool  g_hoverStart = false, g_hoverExit = false;
static bool  g_hoverCircle = false, g_hoverSim = false;
static int   g_mouseX = 0, g_mouseY = 0;
int   g_lastY = 0;
float g_orbitPitch = 0.0f;

bool placingPlanet = false;

#define MOVING_CONST 0.1
#define ROTATION_CONST 3.14f / 180.f
#define LOOK_MOVEMENT_CONST 0.1f

char title[] = "Mataverse";
int FPS = 60;
vec3 CameraPosition(1.0, 1.0, 1.0);
vec3 LookAt_vector(0.0, 0.0, 0.0);
vec3 LookUp_vector(0.0, 1.0, 0.0);
vector<vec3> coordinateSystem;
SpaceShip ship;
Nebo nebo;
vector<Planet*> planets;
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

static const float PITCH_LIMIT = glm::radians(85.0f);
static bool g_draggingPlanet = false;
static float g_dragDepth = 0.0f;
static vec3 g_dragPlaneNormal(0.0f);
static vec3 g_dragOffset(0.0f);

static void orthoBegin() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, winW, 0, winH, -1, 1);
	glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
}
static void orthoEnd() {
	glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
}

static void drawCenteredBitmapString(float cx, float cy, void* font, const std::string& s) {
	float w = 0.f;
	for (unsigned char c : s) w += glutBitmapWidth(font, c);
	glRasterPos2f(cx - w * 0.5f, cy);
	glutBitmapString(font, (const unsigned char*)s.c_str());
}

static void initOrbitFromCurrent() {
	glm::vec3 d = CameraPosition - LookAt_vector;
	g_orbitRadius = glm::length(d);
	if (g_orbitRadius < 1e-6f) g_orbitRadius = 1e-6f;
	g_orbitAngle = std::atan2(d.x, d.z);                           // yaw
	g_orbitPitch = std::atan2(d.y, glm::length(glm::vec2(d.x, d.z))); // pitch
	
	g_orbitPitch = glm::clamp(g_orbitPitch, -PITCH_LIMIT, PITCH_LIMIT);
}


static void drawRoundedRect(float x0, float y0, float x1, float y1, float r, int seg = 20) {
	const float w = x1 - x0, h = y1 - y0;
	r = std::max(0.f, std::min(r, std::min(w, h) * 0.5f));
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f((x0 + x1) * 0.5f, (y0 + y1) * 0.5f);
	for (int corner = 0; corner < 4; ++corner) {
		float cx = (corner == 0 || corner == 3) ? x0 + r : x1 - r;
		float cy = (corner < 2) ? y1 - r : y0 + r;
		float a0 = (float)(corner * 90.0 + 0.0) * 3.14159265f / 180.f;
		float a1 = (float)(corner * 90.0 + 90.0) * 3.14159265f / 180.f;
		for (int i = 0; i <= seg; ++i) {
			float t = a0 + (a1 - a0) * (i / (float)seg);
			glVertex2f(cx + std::cos(t) * r, cy + std::sin(t) * r);
		}
	}
	glEnd();
}

static void drawAxisGizmo()
{
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	const int size = 96;     
	const int margin = 12;     
	const int cornerX = margin;
	const int cornerY = winH - size - margin;

	glViewport(cornerX, cornerY, size, size);

	glEnable(GL_SCISSOR_TEST);
	glScissor(cornerX, cornerY, size, size);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(35.0, 1.0, 0.01, 10.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.f, 0.f, -3.0f); 

	glm::mat4 V = glm::lookAt(CameraPosition, LookAt_vector, LookUp_vector);
	glm::mat3 R = glm::mat3(V);          
	glm::mat3 Ri = glm::transpose(R);      
	glm::mat4 R4(1.0f);
	R4[0] = glm::vec4(Ri[0], 0.0f);
	R4[1] = glm::vec4(Ri[1], 0.0f);
	R4[2] = glm::vec4(Ri[2], 0.0f);
	glMultMatrixf(glm::value_ptr(R4));

	glDisable(GL_LIGHTING);
	glLineWidth(2.0f);

	const float L = 0.9f;

	glColor3f(1.f, 0.f, 0.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f); glVertex3f(L, 0.f, 0.f);
	glEnd();

	glColor3f(0.f, 1.f, 0.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f); glVertex3f(0.f, L, 0.f);
	glEnd();

	glColor3f(0.f, 0.6f, 1.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f); glVertex3f(0.f, 0.f, L);
	glEnd();

	auto drawLabel2D = [](const char* s, const glm::vec3& P3)
		{
			GLdouble model[16], proj[16];
			GLint vp[4];
			glGetDoublev(GL_MODELVIEW_MATRIX, model);
			glGetDoublev(GL_PROJECTION_MATRIX, proj);
			glGetIntegerv(GL_VIEWPORT, vp);

			GLdouble wx, wy, wz;
			if (!gluProject(P3.x, P3.y, P3.z, model, proj, vp, &wx, &wy, &wz)) return;

			const int pad = 8; 
			int x = (int)std::max<double>(pad, std::min<double>(wx, vp[2] - pad));
			int y = (int)std::max<double>(pad, std::min<double>(wy, vp[3] - pad));

			glDisable(GL_DEPTH_TEST);
			glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, vp[2], 0, vp[3], -1, 1);
			glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

			glColor3f(1.f, 1.f, 1.f);
			glRasterPos2i(x, y);
			for (const char* c = s; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

			glPopMatrix();
			glMatrixMode(GL_PROJECTION); glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glEnable(GL_DEPTH_TEST);
		};

	drawLabel2D("X", glm::vec3(L * 0.98f, 0.f, 0.f));
	drawLabel2D("Y", glm::vec3(0.f, L * 0.98f, 0.f));
	drawLabel2D("Z", glm::vec3(0.f, 0.f, L * 0.98f));

	glPopMatrix(); 
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}



static void drawPillButton(float x0, float y0, float x1, float y1, bool primary, float pulse) {
	const float r = (y1 - y0) * 0.5f;
	const int   seg = 40;

	glColor4f(0, 0, 0, 0.35f);

	glBegin(GL_QUADS);
	glVertex2f(x0 + 4 + r, y0 - 4); glVertex2f(x1 + 4 - r, y0 - 4);
	glVertex2f(x1 + 4 - r, y1 - 4); glVertex2f(x0 + 4 + r, y1 - 4);
	glEnd();

	if (primary) glColor4f(0.15f, 0.55f, 1.0f, pulse);
	else         glColor4f(0.15f, 0.15f, 0.18f, 0.92f);

	glBegin(GL_QUADS);
	glVertex2f(x0 + r, y0); glVertex2f(x1 - r, y0);
	glVertex2f(x1 - r, y1); glVertex2f(x0 + r, y1);
	glEnd();

	glColor3f(1, 1, 1);
	glBegin(GL_LINE_LOOP);
	glVertex2f(x0 + r, y0); glVertex2f(x1 - r, y0);
	glVertex2f(x1 - r, y1); glVertex2f(x0 + r, y1);
	glEnd();
}


static void layoutTitleButtons() {
	const float btnW = 320.f, btnH = 64.f, gap = 22.f;
	const float cx = winW * 0.5f;

	const float groupH = btnH * 2.f + gap;
	const float cy = winH * 0.5f;         

	g_btnStartX0 = cx - btnW * 0.5f;
	g_btnStartX1 = cx + btnW * 0.5f;
	g_btnStartY1 = cy + (groupH * 0.5f) - 0.0f;  
	g_btnStartY0 = g_btnStartY1 - btnH;

	g_btnExitX0 = g_btnStartX0;
	g_btnExitX1 = g_btnStartX1;
	g_btnExitY1 = g_btnStartY0 - gap;      
	g_btnExitY0 = g_btnExitY1 - btnH;
}




static inline std::string trim(const std::string& s) {
	size_t b = s.find_first_not_of(" \t\r\n");
	if (b == std::string::npos) return "";
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}

static bool parseColor(const std::string& csv, glm::vec3& out) {
	istringstream ss(csv);
	string tok;
	float vals[3]; int i = 0;
	while (std::getline(ss, tok, ',')) {
		if (i >= 3) break;
		vals[i++] = std::stof(trim(tok));
	}
	if (i != 3) return false;
	out = vec3(vals[0], vals[1], vals[2]);
	return true;
}

static void updateOrbitPos() {
	const float cp = std::cos(g_orbitPitch);
	const float sp = std::sin(g_orbitPitch);
	const float sy = std::sin(g_orbitAngle);
	const float cy = std::cos(g_orbitAngle);

	CameraPosition.x = LookAt_vector.x + g_orbitRadius * cp * sy;
	CameraPosition.y = LookAt_vector.y + g_orbitRadius * sp;
	CameraPosition.z = LookAt_vector.z + g_orbitRadius * cp * cy;

	glm::vec3 forward = glm::normalize(LookAt_vector - CameraPosition);

	glm::vec3 worldUp(0, 1, 0);
	if (std::abs(glm::dot(forward, worldUp)) > 0.9995f) {
		worldUp = glm::vec3(0, 0, 1);
	}

	glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
	glm::vec3 up = glm::normalize(glm::cross(forward, right));

	LookUp_vector = up;
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
	vec3  satPos = vec3(0.0f, 1.2f, -4.0f);
	vec3  satBodyColor = vec3(0.75f, 0.8f, 0.9f);
	vec3  satPanelCol = vec3(0.15f, 0.5f, 1.0f);
	float satScale = 1.2f;
	float satRotY = 0.0f;
	int satEnabled = 0;

	int sat2Enabled = 0;
	vec3  satPos2 = vec3(0.0f, 1.2f, -4.0f);
	vec3  satBodyColor2 = vec3(0.75f, 0.8f, 0.9f);
	vec3  satPanelCol2 = vec3(0.15f, 0.5f, 1.0f);
	float satScale2 = 1.2f;
	float satRotY2 = 0.0f;


	vec3 colorPlaced = g_newPlanetColor;
	float radPlaced = g_newPlanetRadius;
	float massPlaced = g_newPlanetMass;
	int placedEnabled = 0;
	vec3 posPlaced = glm::vec3(0.0f, 0.0f, 0.0f);

	int   pulsarEnabled = 0;
	vec3 pulsarPos = glm::vec3(0.f, 0.5f, -3.f);
	float pulsarRadius = 0.6f;
	float pulsarBeamLen = 3.0f;
	float pulsarRotDeg = 55.0f;

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
		else if (key == "SATELLITE_POS") {          
			parseColor(val, satPos);                 
		}
		else if (key == "SATELLITE_BODY_COLOR") {  
			parseColor(val, satBodyColor);
		}
		else if (key == "SATELLITE_PANEL_COLOR") {
			parseColor(val, satPanelCol);
		}
		else if (key == "SATELLITE_SCALE") {
			satScale = std::stof(val);
		}
		else if (key == "SATELLITE_ROT_Y") {
			satRotY = std::stof(val);
		}
		else if (key == "SATELLITE_ENABLED") {
			satEnabled = std::stoi(val);
		}

		else if (key == "SATELLITE2_POS") {
			parseColor(val, satPos2);
		}
		else if (key == "SATELLITE2_BODY_COLOR") {
			parseColor(val, satBodyColor2);
		}
		else if (key == "SATELLITE2_PANEL_COLOR") {
			parseColor(val, satPanelCol2);
		}
		else if (key == "SATELLITE2_SCALE") {
			satScale2 = std::stof(val);
		}
		else if (key == "SATELLITE2_ROT_Y") {
			satRotY2 = std::stof(val);
		}
		else if (key == "SATELLITE2_ENABLED") {
			sat2Enabled = std::stoi(val);
		}
		else if (key == "PLACED_PLANET_COLOR") {
			parseColor(val, colorPlaced);
		}
		else if (key == "PLACED_PLANET_RADIUS") {
			radPlaced = std::stof(val);
		}
		else if (key == "PLACED_PLANET_MASS") {
			massPlaced = std::stof(val);
		}
		else if (key == "PLACED_PLANET_ENABLED")
		{
			placedEnabled = std::stoi(val);
		}
		else if (key == "PLACED_PLANET_POS")
		{
			parseColor(val, posPlaced);
		}
		else if (key == "PULSAR_ENABLED") {
			pulsarEnabled = std::stoi(val);
		}
		else if (key == "PULSAR_POS") {    
			parseColor(val, pulsarPos);        
		}
		else if (key == "PULSAR_RADIUS") {
			pulsarRadius = std::stof(val);
		}
		else if (key == "PULSAR_BEAM_LENGTH") {
			pulsarBeamLen = std::stof(val);
		}
		else if (key == "PULSAR_ROT_SPEED") {   
			pulsarRotDeg = std::stof(val);
		}
	}

	g_maxPlaceablePlanets = std::max(0, available);
	g_newPlanetColor = color;
	g_newPlanetRadius = rad;
	g_newPlanetMass = mass;
	g_newPlanetForward = fwd;

	g_placedThisLevel = 0;

	if (g_pulsar) { delete g_pulsar; g_pulsar = nullptr; }
	if (pulsarEnabled) {
		g_pulsar = new Pulsar(pulsarPos, pulsarRadius, pulsarBeamLen, pulsarRotDeg);
	}

	g_wormhole = new WormHole(whPos, whRadius, whMass, whRings, whSpacing);
	g_wormhole->orientTowardOnce(ship.getPosition());

	if (satEnabled) {
		g_sat = new Satelit(satPos, satBodyColor);
		g_sat->setPanelColor(satPanelCol);
		g_sat->setScale(satScale);
		g_sat->setRotationY(satRotY);
	}
	if (sat2Enabled) {
		g_sat2 = new Satelit(satPos2 + vec3(2.0f, 0.0f, 0.0f), satBodyColor2);
		g_sat2->setPanelColor(satPanelCol2);
		g_sat2->setScale(satScale2);
		g_sat2->setRotationY(satRotY2 + 45.0f);
	}
	if (placedEnabled)
		g_planetPlaced = new Planet(posPlaced, colorPlaced,radPlaced, massPlaced);
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

static void applyMaxScrollZoom()
{
	// postavi logiku zoom-a na maksimum (isti mehanizam kao mouseWheel)
	cameraDistance = maxDistance;

	// zadrži isti pravac gledanja, samo odmakni kameru
	glm::vec3 viewDirection = glm::normalize(CameraPosition - LookAt_vector);
	if (!std::isfinite(viewDirection.x) || !std::isfinite(viewDirection.y) || !std::isfinite(viewDirection.z) ||
		length(viewDirection) < 1e-8f)
	{
		// fallback ako je kamera “u tački” sa LookAt-om
		viewDirection = glm::vec3(0, 0, 1);
	}
	CameraPosition = LookAt_vector + viewDirection * cameraDistance;

	// sinhronizuj orbit parametre sa novom pozicijom (desni klik orbit nastavlja normalno)
	initOrbitFromCurrent();
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
	if (g_sat) { delete g_sat; g_sat = nullptr; }
	if (g_sat2) { delete g_sat2; g_sat2 = nullptr; }
	if (g_planetPlaced) { delete g_planetPlaced; g_planetPlaced = nullptr; }
	if (g_pulsar) { delete g_pulsar; g_pulsar = nullptr; }
	loadLevelConfig(g_currentLevelPath);
	applyMaxScrollZoom();
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

	vec4 startW = invPV * startNDC; 
	startW /= startW.w;
	vec4 endW = invPV * endNDC;   
	endW /= endW.w;

	orig = vec3(startW);
	dir = normalize(glm::vec3(endW - startW));
}

void drawHUD()
{
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, winW, 0, winH, -1, 1);
	glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

	float panelH, cx, cy, rad; hudCircleGeometry(panelH, cx, cy, rad);

	glColor4f(0.12f, 0.12f, 0.12f, 0.85f);
	glBegin(GL_QUADS);
	glVertex2f(0, 0); glVertex2f(winW, 0); glVertex2f(winW, panelH); glVertex2f(0, panelH);
	glEnd();

	if (simulationActive) glColor3f(0.35f, 0.35f, 0.35f);
	else                  glColor3f(g_hoverCircle ? 0.35f : 0.2f, 0.6f, 1.0f);
	drawFilledCircle2D(cx, cy, g_hoverCircle ? rad + 2.0f : rad, 48);

	glColor3f(1, 1, 1);
	float textX = cx + rad + 10.0f;
	float textY = panelH - 16.0f - 6.0f;
	char buf[256];
	sprintf_s(buf,
		"poluprecnik = %.2f\nmasa = %.2f%s\npreostalo: %d/%d",
		g_newPlanetRadius,
		g_newPlanetMass,
		(!simulationActive && placingPlanet) ? "\n[klik na scenu za postavljanje]" : "",
		std::max(0, g_maxPlaceablePlanets - g_placedThisLevel),
		g_maxPlaceablePlanets
	);
	glRasterPos2f(textX, textY);
	glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)buf);

	float bx0, by0, bx1, by1;
	hudSimButtonGeometry(panelH, bx0, by0, bx1, by1);

	if (simulationActive) {
		if (g_hoverSim) glColor4f(0.18f, 0.90f, 0.35f, 1.0f);
		else            glColor4f(0.10f, 0.80f, 0.25f, 1.0f);
	}
	else {
		if (g_hoverSim) glColor4f(0.55f, 0.55f, 0.60f, 0.98f);
		else            glColor4f(0.35f, 0.35f, 0.35f, 0.98f);
	}

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

	glColor3f(0.85f, 0.85f, 0.85f);
	if (g_hoverCircle) {
		glRasterPos2f(cx + rad + 10.0f, cy + 4.0f);

	}
	if (g_hoverSim) {
		glRasterPos2f(bx0, by0 - 16.0f);
		glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)(simulationActive ? "Klik za reset simulacije" : "Klik da pokrenes simulaciju"));
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}


static void layoutTitleButton() {
	const float btnW = 280.f, btnH = 64.f;
	g_btnX0 = (winW - btnW) * 0.5f;
	g_btnX1 = g_btnX0 + btnW;
	g_btnY0 = winH * 0.35f;
	g_btnY1 = g_btnY0 + btnH;
}

static void drawTitleScreen() {
	orthoBegin();
	glBegin(GL_QUADS);
	glColor4f(0.0f, 0.0f, 0.0f, 0.18f); glVertex2f(0, 0);
	glColor4f(0.0f, 0.0f, 0.0f, 0.18f); glVertex2f(winW, 0);
	glColor4f(0.0f, 0.0f, 0.0f, 0.28f); glVertex2f(winW, winH);
	glColor4f(0.0f, 0.0f, 0.0f, 0.28f); glVertex2f(0, winH);
	glEnd();

	glColor3f(0.2f, 0.8f, 1.0f);
	drawCenteredBitmapString(winW * 0.5f, winH * 0.78f, GLUT_BITMAP_HELVETICA_18, "MATAVERSE");

	float growStart = g_hoverStart ? 6.0f : 0.0f;
	float growExit = g_hoverExit ? 6.0f : 0.0f;

	float sX0 = g_btnStartX0 - growStart;
	float sX1 = g_btnStartX1 + growStart;
	float sY0 = g_btnStartY0 - growStart * 0.5f;
	float sY1 = g_btnStartY1 + growStart * 0.5f;

	float eX0 = g_btnExitX0 - growExit;
	float eX1 = g_btnExitX1 + growExit;
	float eY0 = g_btnExitY0 - growExit * 0.5f;
	float eY1 = g_btnExitY1 + growExit * 0.5f;

	layoutTitleButtons();
	float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	float pulse = 0.90f + 0.10f * (0.5f * (std::sin(t * 3.0f) + 1.0f));

	drawPillButton(sX0, sY0, sX1, sY1, true, g_hoverStart ? 1.0f : pulse);
	glColor3f(1, 1, 1);
	float labelOffset = ((g_btnStartY1 - g_btnStartY0) - 18.f) * 0.5f + 4.f;
	drawCenteredBitmapString((g_btnStartX0 + g_btnStartX1) * 0.5f, g_btnStartY0 + labelOffset, GLUT_BITMAP_HELVETICA_18, "ZAPOCNI IGRU");

	drawPillButton(eX0, eY0, eX1, eY1, false, g_hoverExit ? 1.0f : 0.92f);
	drawCenteredBitmapString((g_btnExitX0 + g_btnExitX1) * 0.5f, g_btnExitY0 + labelOffset, GLUT_BITMAP_HELVETICA_18, "NAPUSTI IGRU");

	glColor3f(0.85f, 0.85f, 0.85f);
	drawCenteredBitmapString(winW * 0.5f, winH * 0.18f, GLUT_BITMAP_HELVETICA_12, "ENTER/SPACE za start, ESC za izlaz");

	orthoEnd();
}




static void drawLaser(const Satelit* sat, float range)
{
	if (!sat) return;

	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);        
	glLineWidth(1.5f);
	glColor3f(1.0f, 0.0f, 0.0f);

	glm::vec3 S = sat->getPosition();
	glm::vec3 E = S + sat->forward() * range;

	glBegin(GL_LINES);
	glVertex3f(S.x, S.y, S.z);
	glVertex3f(E.x, E.y, E.z);
	glEnd();

	glPopAttrib();
}

static void drawJammedShell(const Planet* p)
{
	if (!p) return;

	const glm::vec3 C = p->getPosition();
	const float R = p->getRadius();
	const float scale = 1.04f;

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
		GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT);

	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(GL_FALSE);

	glPushMatrix();
	glTranslatef(C.x, C.y, C.z);

	glColor4f(1.0f, 0.1f, 0.1f, 0.22f);
	glutSolidSphere(R * scale, 32, 20);

	glLineWidth(1.5f);
	glColor4f(1.0f, 0.2f, 0.2f, 0.75f);
	glutWireSphere(R * scale * 1.006f, 24, 16);

	glPopMatrix();

	glDepthMask(GL_TRUE);
	glPopAttrib();
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// PROJECTION
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.0, 16.0 / 9.0, 0.1, 500.0);

	// MODELVIEW
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		CameraPosition.x, CameraPosition.y, CameraPosition.z,
		LookAt_vector.x, LookAt_vector.y, LookAt_vector.z,
		LookUp_vector.x, LookUp_vector.y, LookUp_vector.z
	);

	// --- POZICIJE SVETALA (svaki frame, posle kamere!) ---
	{
		// postojeća svetla
		GLfloat L0_pos[] = { 12.0f, 8.0f, -6.0f, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, L0_pos);

		GLfloat L1_pos[] = { -8.0f, 3.0f, 6.0f, 1.0f };
		glLightfv(GL_LIGHT1, GL_POSITION, L1_pos);

		// HEAD-LIGHT: direkciono u pravcu gledanja
		GLfloat L2_dir[] = { 0.0f, 0.0f, -1.0f, 0.0f }; // w=0 => direkciono
		glLightfv(GL_LIGHT2, GL_POSITION, L2_dir);
	}

	// --- crtaj scenu ---
	nebo.draw();

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);        
	glEnable(GL_COLOR_MATERIAL);

	if (g_state == GameState::TITLE) {
		drawTitleScreen();
		glutSwapBuffers();
		return;
	}

	ship.draw();

	const float JAM_RANGE = 20.0f;
	if (g_sat) { g_sat->addSpin(0.5f);   g_sat->draw();  drawLaser(g_sat, JAM_RANGE); }
	if (g_sat2) { g_sat2->addSpin(-0.5f); g_sat2->draw(); drawLaser(g_sat2, JAM_RANGE); }

	if (g_wormhole) g_wormhole->draw();

	float Jam_GetRemainingFor(const Planet * p);
	if (g_planetPlaced) {
		g_planetPlaced->draw();
		if (Jam_GetRemainingFor(g_planetPlaced) > 0.0f) drawJammedShell(g_planetPlaced);
	}
	for (auto& p : planets) {
		p->draw();
		if (Jam_GetRemainingFor(p) > 0.0f) drawJammedShell(p);
	}

	if (g_pulsar) g_pulsar->draw();

	drawHUD();
	drawAxisGizmo();

	glutSwapBuffers();
}




static int g_currentLevelIndex = 1;

static int parseLevelIndexFromPath(const std::string& path) {
	size_t L = path.rfind("Level");
	if (L == string::npos) return 1;
	size_t dot = path.rfind(".txt");
	if (dot == string::npos || dot <= L + 5) return 1;
	string num = path.substr(L + 5, dot - (L + 5));
	try { return std::max(1, std::stoi(num)); }
	catch (...) { return 1; }
}

static std::string makeLevelPath(int idx) {
	char buf[128];
	snprintf(buf, sizeof(buf), "src/Levels/Level%d.txt", idx);
	return string(buf);
}

static bool fileExists(const std::string& p) {
	std::ifstream f(p);
	return (bool)f;
}

static void gotoLevel(int idx) {
	if (idx < 1) idx = 1;
	string p = makeLevelPath(idx);
	if (!fileExists(p)) {
		std::printf("[LEVEL] Level%d ne postoji (%s). Vracam na Level1.\n", idx, p.c_str());
		idx = 1;
		p = makeLevelPath(1);
	}
	g_currentLevelIndex = idx;
	g_currentLevelPath = p;
	stopAndReset();
}

static void advanceToNextLevel() {
	int next = g_currentLevelIndex + 1;
	std::string np = makeLevelPath(next);
	if (!fileExists(np)) {
		std::printf("[LEVEL] Naredni Level%d ne postoji. Vracam na Level1.\n", next);
		next = 1;
	}
	std::printf("[GAME] Level kompletiran! Prelazim na Level%d...\n", next);
	gotoLevel(next);
}

void timer(int v)
{
	static int lastMs = glutGet(GLUT_ELAPSED_TIME);
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	float dt = (nowMs - lastMs) / 1000.0f;
	lastMs = nowMs;
	if (g_pulsar) g_pulsar->update(dt);
	if (simulationActive) {
		vector<Planet*> allPlanets = planets;
		if (g_planetPlaced) allPlanets.push_back(g_planetPlaced);
		ship.update(dt, allPlanets,g_wormhole,g_sat);
		
		if (ship.shipCaptured)
		{
			advanceToNextLevel();
		}

		if (ship.hitsAny(allPlanets)) {
			simulationActive = false;
			stopAndReset();
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
	layoutTitleButtons();
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
	if (selectedPlanet && !simulationActive) {
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
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	// --- Lighting on ---
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0); // key light
	glEnable(GL_LIGHT1); // fill light (slabo)

	// Global ambient (niska vrednost da senke ostanu tamne)
	{
		GLfloat globalAmb[] = { 0.22f, 0.22f, 0.25f, 1.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

	}

	// LIGHT0: jače, belo
	{
		GLfloat diff0[] = { 1.15f, 1.15f, 1.15f, 1.0f };
		GLfloat spec0[] = { 1.0f,  1.0f,  1.0f,  1.0f };
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diff0);
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
	}

	// LIGHT1: slabo “popunjava” senku
	{
		GLfloat diff1[] = { 0.55f, 0.55f, 0.60f, 1.0f };
		GLfloat amb1[] = { 0.10f, 0.10f, 0.12f, 1.0f };
		glLightfv(GL_LIGHT1, GL_DIFFUSE, diff1);
		glLightfv(GL_LIGHT1, GL_AMBIENT, amb1);
	}

	glEnable(GL_LIGHT2);
	GLfloat diff2[] = { 0.55f, 0.55f, 0.58f, 1.0f };
	GLfloat amb2[] = { 0.06f, 0.06f, 0.07f, 1.0f };
	glLightfv(GL_LIGHT2, GL_DIFFUSE, diff2);
	glLightfv(GL_LIGHT2, GL_AMBIENT, amb2);

	// Materijal i normalizacija
	glEnable(GL_NORMALIZE);

	// Ako koristiš per-vertex boje (glColor*), ostavi COLOR_MATERIAL:
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	GLfloat matSpec[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.0f);

	// Transparencija (za HUD/efekte)
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

static bool rayPlaneHit(const glm::vec3& ro, const glm::vec3& rd,
	const glm::vec3& p0, const glm::vec3& n, float& t)
{
	float denom = glm::dot(n, rd);
	if (std::fabs(denom) < 1e-6f) return false;
	t = glm::dot(p0 - ro, n) / denom;
	return t >= 0.0f;
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
	if (g_state == GameState::TITLE) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			int yBL = winH - y;
			layoutTitleButtons();

			bool startHit = (x >= g_btnStartX0 && x <= g_btnStartX1 && yBL >= g_btnStartY0 && yBL <= g_btnStartY1);
			bool exitHit = (x >= g_btnExitX0 && x <= g_btnExitX1 && yBL >= g_btnExitY0 && yBL <= g_btnExitY1);

			if (startHit) {
				g_state = GameState::RUNNING;
				stopAndReset();
				simulationActive = false;
				glutPostRedisplay();
			}
			else if (exitHit) {
				exit(0);
			}
		}
		return;
	}

	if (button == GLUT_RIGHT_BUTTON) {
		if (state == GLUT_DOWN) {
			g_orbiting = true;
			g_lastX = x;
			g_lastY = y;
			initOrbitFromCurrent();
		}
		else {
			g_orbiting = false;
		}
		return;
	}

	int yBL = winH - y;

	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		g_draggingPlanet = false;
		return;
	}

	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

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

	if (simulationActive) {
		placingPlanet = false;
		selectedPlanet = nullptr;
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

	if (selectedPlanet && !simulationActive) {
		g_draggingPlanet = true;

		glm::vec3 ro, rd;
		screenToWorldRay(x, yBL, ro, rd);

		g_dragPlaneNormal = glm::normalize(LookAt_vector - CameraPosition);

		g_dragDepth = glm::dot(selectedPlanet->getPosition() - CameraPosition, g_dragPlaneNormal);

		float t;
		glm::vec3 planePoint = CameraPosition + g_dragPlaneNormal * g_dragDepth;
		if (rayPlaneHit(ro, rd, planePoint, g_dragPlaneNormal, t)) {
			glm::vec3 hit = ro + rd * t;
			g_dragOffset = selectedPlanet->getPosition() - hit;
		}
		else {
			g_dragOffset = glm::vec3(0.0f);
		}
		glutPostRedisplay();
		return;
	}

	glutPostRedisplay();
}


void mouseMotion(int x, int y) {
	int yBL = winH - y;

	if (g_draggingPlanet && selectedPlanet && !simulationActive) {
		glm::vec3 ro, rd; screenToWorldRay(x, yBL, ro, rd);
		glm::vec3 planePoint = CameraPosition + g_dragPlaneNormal * g_dragDepth;

		float t;
		if (rayPlaneHit(ro, rd, planePoint, g_dragPlaneNormal, t)) {
			glm::vec3 hit = ro + rd * t;
			glm::vec3 newPos = hit + g_dragOffset;

			int mods = glutGetModifiers();
			if (mods & GLUT_ACTIVE_SHIFT) {
				newPos.y = selectedPlanet->getPosition().y;
			}
			selectedPlanet->setPosition(newPos);
			glutPostRedisplay();
		}
		return;
	}

	if (!g_orbiting) return;
	int dx = x - g_lastX;
	int dy = y - g_lastY;
	g_lastX = x;
	g_lastY = y;

	const float sens = 0.01f;
	const float sensY = 0.02f;

	g_orbitAngle += dx * sens;      
   

	const float PI = 3.14159265358979323846f;
	const float TWO_PI = 2.0f * PI;

	if (g_orbitAngle > PI) g_orbitAngle -= TWO_PI;
	if (g_orbitAngle <= -PI) g_orbitAngle += TWO_PI;

	g_orbitPitch += -dy * sensY;
	g_orbitPitch = glm::clamp(g_orbitPitch, -PITCH_LIMIT, PITCH_LIMIT);

	updateOrbitPos();
	glutPostRedisplay();
}



void passiveMotion(int x, int y) {
	g_mouseX = x; g_mouseY = y;
	int yBL = winH - y;

	layoutTitleButtons();
	g_hoverStart = (x >= g_btnStartX0 && x <= g_btnStartX1 && yBL >= g_btnStartY0 && yBL <= g_btnStartY1);
	g_hoverExit = (x >= g_btnExitX0 && x <= g_btnExitX1 && yBL >= g_btnExitY0 && yBL <= g_btnExitY1);

	float panelH, cx, cy, rad; hudCircleGeometry(panelH, cx, cy, rad);
	float dx = x - cx, dy = yBL - cy;
	g_hoverCircle = ((dx * dx + dy * dy) <= rad * rad);

	float bx0, by0, bx1, by1; hudSimButtonGeometry(panelH, bx0, by0, bx1, by1);
	g_hoverSim = (x >= bx0 && x <= bx1 && yBL >= by0 && yBL <= by1);

	if (g_state == GameState::TITLE) {
		bool onBtn = (g_hoverStart || g_hoverExit);
		setPointerCursor(onBtn);
	}
	else {
		bool anyHUD = (g_hoverCircle || g_hoverSim);
		setPointerCursor(anyHUD);
	}

	glutPostRedisplay();


}


static bool deletePlanet(Planet* target)
{
	if (!target) return false;

	auto it = std::find(planets.begin(), planets.end(), target);
	if (it != planets.end()) {
		delete* it;
		planets.erase(it);

		if (g_placedThisLevel > 0) g_placedThisLevel--;

		if (selectedPlanet == target) selectedPlanet = nullptr;
		return true;
	}
	return false;
}

void keyPress(unsigned char key, int x, int y)
{
	if (g_state == GameState::TITLE) {
		if (key == 13 || key == ' ') { 
			g_state = GameState::RUNNING;
			stopAndReset();
			simulationActive = false;
			glutPostRedisplay();
		}
		else if (key == 27) { 
			exit(0);
		}
		return;
	}


	switch (key)
	{
	case 27:
		g_state = GameState::TITLE;
		break;
	case 'w':
		MoveForward();
		break;
	case 's':
		MoveBackward();
		break;
	case 'a':
		MoveLeft();
		break;
	case 'd':
		MoveRight();
		break;
	case 'u':
		SpeedUp();
		break;
	case 'j':

		SpeedDown();
		break;
	case 'x':
		if (simulationActive) return;
		if (deletePlanet(selectedPlanet)) {
			glutPostRedisplay();
		}
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

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(height * ratio, height);
	glutInitWindowPosition(150, 50);
	glutCreateWindow(title);
	glutDisplayFunc(display);
	initOrbitFromCurrent();
	glutTimerFunc(100, timer, 0);
	glutReshapeFunc(reshape);

	glutMouseFunc(mousePress);
	glutMouseWheelFunc(mouseWheel);
	glutKeyboardFunc(keyPress);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(passiveMotion);

	initGL();
	loadLevelConfig(g_currentLevelPath);
	applyMaxScrollZoom();
	g_currentLevelIndex = parseLevelIndexFromPath(g_currentLevelPath);
	glutMainLoop();

	return 0;
}
