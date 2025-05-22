#define NOMINMAX
#include <iostream>
#include <memory>
#include <filesystem>
#include <algorithm>
#include "lsystem.hpp"
#include "util.hpp"
#include <GL/freeglut.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
namespace fs = std::filesystem;

// Menu identifiers
const int MENU_OBJBASE = 64;
const int MENU_PREVITER = 2;
const int MENU_NEXTITER = 3;
const int MENU_REPARSE = 4;
const int MENU_EXIT = 1;
std::vector<std::string> modelFilenames;

// OpenGL state
GLuint groundVAO = 0, groundVBO = 0;
GLuint groundShader = 0;
int width, height;
std::unique_ptr<LSystem> lsystem;
unsigned int iter = 0;
std::string lastFilename;
int lastFilenameIdx = -1;
float yaw = -45.0f, pitch = 30.0f, radius = 8.0f;
bool dragging = false;
int lastX = 0, lastY = 0;

// Initialization functions
void initGLUT(int* argc, char** argv);
void initMenu();
void findModelFiles();
void setupGroundPlane();
void setupGroundShader();
void setupSkyShader();

// Sky shader state
GLuint skyVAO = 0, skyVBO = 0, skyShader = 0;

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyPress(unsigned char key, int x, int y);
void keyRelease(unsigned char key, int x, int y);
void keySpecial(int key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();

// Program entry point
int main(int argc, char** argv) {
	std::string configFile = "models/tree3D1.txt";
	if (argc > 1)
		configFile = std::string(argv[1]);

	try {
		initGLUT(&argc, argv);
		initMenu();

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);

		setupGroundPlane();
		setupGroundShader();
		setupSkyShader();

		lsystem.reset(new LSystem);
		try {
			if (!configFile.empty()) {
				lsystem->parseFile(configFile);
				lastFilename = configFile;
				for (unsigned int i = 0; i < modelFilenames.size(); i++) {
					if (configFile == modelFilenames[i])
						lastFilenameIdx = i;
				}
				iter = lsystem->getNumIter() - 1;
				std::cout << "Iteration " << iter << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Parse error: " << e.what() << std::endl;
		}

	}
	catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		cleanup();
		return -1;
	}

	glutMainLoop();
	return 0;
}

// Setup window and callbacks
void initGLUT(int* argc, char** argv) {
	width = 800; height = 600;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("3D L-System with Sky and Ground");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPress);
	glutKeyboardUpFunc(keyRelease);
	glutSpecialFunc(keySpecial);
	glutMouseFunc(mouseBtn);
	glutMotionFunc(mouseMove);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
}

void initMenu() {
	findModelFiles();
	int objMenu = glutCreateMenu(menu);
	for (int i = 0; i < modelFilenames.size(); i++) {
		glutAddMenuEntry(modelFilenames[i].c_str(), MENU_OBJBASE + i);
	}
	glutCreateMenu(menu);
	glutAddSubMenu("View L-System", objMenu);
	glutAddMenuEntry("Prev iter", MENU_PREVITER);
	glutAddMenuEntry("Next iter", MENU_NEXTITER);
	glutAddMenuEntry("Reparse", MENU_REPARSE);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void findModelFiles() {
	fs::path modelsDir = "models";
	for (auto& di : fs::directory_iterator(modelsDir)) {
		if (di.is_regular_file() && di.path().extension() == ".txt")
			modelFilenames.push_back(di.path().string());
	}
	std::sort(modelFilenames.begin(), modelFilenames.end());
}

void setupGroundPlane() {
	float size = 10.0f;
	float y = -1.0f;
	float verts[] = {
		-size, y, -size,
		 size, y, -size,
		 size, y,  size,
		-size, y,  size
	};
	if (groundVAO == 0) {
		glGenVertexArrays(1, &groundVAO);
		glGenBuffers(1, &groundVBO);
		glBindVertexArray(groundVAO);
		glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);
	}
}

void setupGroundShader() {
	GLuint vert = compileShader(GL_VERTEX_SHADER, "shaders/ground_v.glsl");
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, "shaders/ground_f.glsl");
	std::vector<GLuint> shaders = { vert, frag };
	groundShader = linkProgram(shaders);
	for (auto s : shaders) glDeleteShader(s);
}

// Sky: simple gradient quad covering the screen
void setupSkyShader() {
	// Fullscreen quad (NDC)
	float verts[] = {
		-1, -1,
		 1, -1,
		 1,  1,
		-1,  1
	};
	if (skyVAO == 0) {
		glGenVertexArrays(1, &skyVAO);
		glGenBuffers(1, &skyVBO);
		glBindVertexArray(skyVAO);
		glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);
	}
	GLuint vert = compileShader(GL_VERTEX_SHADER, "shaders/sky_v.glsl");
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, "shaders/sky_f.glsl");
	std::vector<GLuint> shaders = { vert, frag };
	skyShader = linkProgram(shaders);
	for (auto s : shaders) glDeleteShader(s);
}

// Called whenever a screen redraw is requested
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw sky (disable depth so it doesn't interfere)
	glDisable(GL_DEPTH_TEST);
	glUseProgram(skyShader);
	glBindVertexArray(skyVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);

	float aspect = (float)width / (float)height;
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

	float camX = radius * cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	float camY = radius * sin(glm::radians(pitch));
	float camZ = radius * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	glm::vec3 camPos = glm::vec3(camX, camY, camZ);
	glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 viewProj = proj * view;

	// Draw ground plane
	glUseProgram(groundShader);
	glm::mat4 groundXform = viewProj;
	glUniformMatrix4fv(glGetUniformLocation(groundShader, "xform"), 1, GL_FALSE, glm::value_ptr(groundXform));
	glBindVertexArray(groundVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);

	if (lsystem && lsystem->getNumIter() > 0)
		lsystem->drawIter(iter, viewProj);

	glutSwapBuffers();
}

void reshape(GLint w, GLint h) {
	width = w; height = h;
	glViewport(0, 0, w, h);
}

void keyPress(unsigned char key, int x, int y) {
	switch (key) {
	case ' ':
		menu(MENU_REPARSE);
		break;
	}
}

void keyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
		menu(MENU_EXIT);
		break;
	}
}

void keySpecial(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
		menu(MENU_PREVITER);
		break;
	case GLUT_KEY_RIGHT:
		menu(MENU_NEXTITER);
		break;
	case GLUT_KEY_UP: {
		int idx = lastFilenameIdx;
		if (idx < 0) idx = 0;
		idx--;
		idx = (idx + modelFilenames.size()) % modelFilenames.size();
		menu(MENU_OBJBASE + idx);
		break;
	}
	case GLUT_KEY_DOWN: {
		int idx = lastFilenameIdx;
		idx++;
		idx = idx % modelFilenames.size();
		menu(MENU_OBJBASE + idx);
		break;
	}
	}
}

void mouseBtn(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		dragging = (state == GLUT_DOWN);
		lastX = x; lastY = y;
	}
}

void mouseMove(int x, int y) {
	if (dragging) {
		float dx = float(x - lastX);
		float dy = float(y - lastY);
		yaw += dx * 0.5f;
		pitch -= dy * 0.5f;
		pitch = glm::clamp(pitch, -89.0f, 89.0f);
		lastX = x; lastY = y;
		glutPostRedisplay();
	}
}

void idle() {}

void menu(int cmd) {
	switch (cmd) {
	case MENU_EXIT:
		glutLeaveMainLoop();
		break;
	case MENU_PREVITER:
		if (iter != 0) {
			iter--;
			std::cout << "Iteration " << iter << std::endl;
			glutPostRedisplay();
		}
		break;
	case MENU_NEXTITER:
		if (!lsystem->getNumIter()) break;
		try {
			if (iter + 1 >= lsystem->getNumIter())
				lsystem->iterate();
			iter++;
			std::cout << "Iteration " << iter << std::endl;
			glutPostRedisplay();
		}
		catch (const std::exception& e) {
			std::cerr << "Too many iterations: " << e.what() << std::endl;
		}
		break;
	case MENU_REPARSE:
		if (!lastFilename.empty()) {
			try {
				lsystem->parseFile(lastFilename);
				iter = lsystem->getNumIter() - 1;
				std::cout << "Iteration " << iter << std::endl;
				glutPostRedisplay();
			}
			catch (const std::exception& e) {
				std::cerr << "Parse error: " << e.what() << std::endl;
			}
		}
		break;
	default:
		if (cmd >= MENU_OBJBASE) {
			try {
				lsystem->parseFile(modelFilenames[cmd - MENU_OBJBASE]);
				lastFilename = modelFilenames[cmd - MENU_OBJBASE];
				lastFilenameIdx = cmd - MENU_OBJBASE;
				iter = lsystem->getNumIter() - 1;
				std::cout << "Iteration " << iter << std::endl;
				glutPostRedisplay();
			}
			catch (const std::exception& e) {
				std::cerr << "Parse error: " << e.what() << std::endl;
			}
		}
		break;
	}
}

void cleanup() {
	lsystem.reset(nullptr);
	if (groundVAO) glDeleteVertexArrays(1, &groundVAO);
	if (groundVBO) glDeleteBuffers(1, &groundVBO);
	if (groundShader) glDeleteProgram(groundShader);
	if (skyVAO) glDeleteVertexArrays(1, &skyVAO);
	if (skyVBO) glDeleteBuffers(1, &skyVBO);
	if (skyShader) glDeleteProgram(skyShader);
}