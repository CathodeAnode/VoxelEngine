#include <iostream>
#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>

#include <windows.h>

#include "shader.h"
#include "mouse.h"
#include "keyboard.h"
#include "joystick.h"
#include "camera.h"

#include "screen.h"

#include "chunk.h"
#include "chunk_grid.h"
#include "voxel_renderer.h"
#include "voxel_mesher.h"
#include "world.h"



void processInput(Screen& screen, double dt);
void displayFPSOnWindow(float frameFPS, int numOfFrames);

int countFPS = 0;
float sumFPS = 0;

Joystick mainJ(0);

unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));

float deltaTime = 0.0f;
float fps = 0.0f;
float lastFrame = 0.0f;

Screen screen(800, 600, "VoxelEngine");


int main() {
	//ChunkGrid8 world;
	//VoxelMesher8 mesher;
	//world.addChunk(Chunk8(true), glm::ivec3(0, 0, 0));
	//ChunkQuads quads  = mesher.meshChunk(world, glm::ivec3(0, 0, 0));
	//world.addChunk(Chunk8(true), glm::ivec3(0, 1, 0));

	//std::cout << quads.quadData.size() << std::endl;
	//std::cout << "rrrrdddhhhhhwwwwwzzzzzyyyyyxxxxx\n";
	//for (const auto& quad : quads.quadData) {
	//	std::cout << std::bitset<32>(quad) << std::endl;
	//}


	if (!screen.init()) {
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	screen.enableInputs();
	screen.toggleCursor();

	mainJ.update();
	if (mainJ.isPresent()) {
		std::cout << mainJ.getName() << " is connected.\n";
	}
	else {
		std::cout << "Joystick not connected.\n";
	}


	/*
	-----------------------
		Shaders
	-----------------------
	*/
	World8 world(10);

	Shader shaderPrgm = Shader("core_vertex_shader.glsl", "core_fragment_shader.glsl");

	shaderPrgm.use();

	// create transformation for screen
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	double lastToggleTime = 0.0;
	int i = 0;
	while (screen.isOpen()) {
		double currTime = glfwGetTime();
		deltaTime = currTime - lastFrame;
		fps = 1 / deltaTime;
		lastFrame = currTime;

		// process input
		processInput(screen, deltaTime * 5);

		// render
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		view = camera.getViewMatrix();
		projection = glm::perspective(glm::radians(camera.zoom), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);

		// draw
		shaderPrgm.use();
		shaderPrgm.setMat4("view", view);
		shaderPrgm.setMat4("projection", projection);
		world.render();


		// send back buffer to front buffer
		screen.update();
		displayFPSOnWindow(fps, 1000);
	}

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "GL ERROR: " << std::hex << err << std::endl;
	}

	glfwTerminate();

	return 0;
}

void processInput(Screen& screen, double dt) {
	if (Keyboard::key(GLFW_KEY_ESCAPE)) {
		screen.close();
	}

	if (Keyboard::key(GLFW_KEY_W)) {
		camera.updateCameraPos(CameraDirection::FORWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_S)) {
		camera.updateCameraPos(CameraDirection::BACKWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_D)) {
		camera.updateCameraPos(CameraDirection::RIGHT, dt);
	}

	if (Keyboard::key(GLFW_KEY_A)) {
		camera.updateCameraPos(CameraDirection::LEFT, dt);
	}

	if (Keyboard::key(GLFW_KEY_SPACE)) {
		camera.updateCameraPos(CameraDirection::UP, dt);
	}

	if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
		camera.updateCameraPos(CameraDirection::DOWN, dt);
	}

	double dx = Mouse::getDX(), dy = Mouse::getDY();
	if (dx != 0 || dy != 0) {
		camera.updateCameraDirection(dx, dy);
	}

	double scrollDY = Mouse::getScrollDY();
	if (scrollDY != 0) {
		camera.updateCameraZoom(scrollDY);
	}

	mainJ.update();
}

void displayFPSOnWindow(float frameFPS, int numOfFrames) {
	if (countFPS > numOfFrames) {
		screen.setTitle(("VoxelEngine - FPS: " + std::to_string(sumFPS/ countFPS)).c_str());
		countFPS = 0;

	}
	else
	{
		sumFPS += frameFPS;
		countFPS++;
	}
}