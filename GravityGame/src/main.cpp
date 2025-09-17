//g++ -o p main.cpp -lGL -lglut -lGLU

#define GLM_ENABLE_EXPERIMENTAL

#include "./Models/SpaceShip.h"
#include "./Models/Nebo.h"
#include "./Models/Planets/Planet.h"


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
	for (auto& p : planets) p->draw();
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

void reshape(GLsizei width, GLsizei height)
{
	if (height * ratio <= width)
		width = ratio * height;
	else
		height = width / ratio;
	glViewport(0, 0, width, height);
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

void mouseWheel(int wheel, int direction, int x, int y)
{
	if (direction > 0)
	{
		cameraDistance -= zoomStep;
	}
	else
	{
		cameraDistance += zoomStep;
	}

	if (cameraDistance < minDistance)
	{
		cameraDistance = minDistance;
	}
	else if (cameraDistance > maxDistance)
	{
		cameraDistance = maxDistance;
	}
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
		// LookUp_vector = mt * LookUp_vector;

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
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
}

void mousePress(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			//FUNKCIJA
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
		{
			//FUNKCIJA
		}
		break;
	default:
		break;
	}
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
	Planet planet(vec3(10, 0, 1), vec3(1, 1, 1), 3, 100);
	Planet planet2(vec3(5, 0, 5), vec3(0.5, 0.5, 1), 2, 100);
	planets.push_back(&planet);
	planets.push_back(&planet2);
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
