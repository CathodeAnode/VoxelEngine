#include <iostream>
#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <memory>
#include <chrono>

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
#include "chunk_generator_strategy.h"
#include "voxel_world_editor.h"
#include "world.h"
#include "scene.h"
#include "voxel_ray_cast.h"

#define TIME_FUNCTION(func_call) \
    do { \
        auto start_time = std::chrono::high_resolution_clock::now(); \
        func_call; \
        auto end_time = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count(); \
        std::cout << "Function '" << #func_call << "' executed in " << duration << " microseconds." << std::endl; \
    } while (0)


void processInput(Screen& screen, double dt);
void displayFPSOnWindow(float frameFPS, int numOfFrames, glm::vec3 cameraPos);

template<typename ChunkType> 
ChunkType GenerateRampChunk();

int countFPS = 0;
float sumFPS = 0;

Joystick mainJ(0);

unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), SCREEN_WIDTH, SCREEN_HEIGHT, 0.1f, 100.0f);

float deltaTime = 0.0f;
float fps = 0.0f;
float lastFrame = 0.0f;

Screen screen(SCREEN_WIDTH, SCREEN_HEIGHT, "VoxelEngine");


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

	mainJ.Update();
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
	Chunk8 filledChunk(true);
	std::unique_ptr<FlatChunkGeneration<Chunk8>> flatGenerator = std::make_unique<FlatChunkGeneration<Chunk8>>(0);
	World8 world(11u, std::move(flatGenerator));
	VoxelWorldEditor<Chunk8> test(world);



	Shader shaderPrgm = Shader({ 
		{"voxel_shader.vert.glsl", GL_VERTEX_SHADER},	
		{"voxel_shader.frag.glsl", GL_FRAGMENT_SHADER}
		});

	shaderPrgm.Use();
	//camera.speed = 40.0f;

	double lastToggleTime = 0.0;
	int i = 0;
	while (screen.isOpen()) {
		double currTime = glfwGetTime();
		deltaTime = currTime - lastFrame;
		fps = 1 / deltaTime;
		lastFrame = currTime;

		// process input
		processInput(screen, deltaTime);

		// render
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		camera.Update();

		// draw
		shaderPrgm.Use();
		shaderPrgm.SetMat4("view", camera.GetViewMatrix());
		shaderPrgm.SetMat4("projection", camera.GetProjMatrix());
		world.Update(camera.pos);
		world.UpdateRender(camera.pos);
		world.Render();
		//std::cout << camera.pos.x << ", " << camera.pos.y << ", " << camera.pos.z << std::endl;


		// send back buffer to front buffer
		screen.Update();
		displayFPSOnWindow(fps, 400, camera.pos);
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
		camera.UpdateCameraPos(CameraDirection::FORWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_S)) {
		camera.UpdateCameraPos(CameraDirection::BACKWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_D)) {
		camera.UpdateCameraPos(CameraDirection::RIGHT, dt);
	}

	if (Keyboard::key(GLFW_KEY_A)) {
		camera.UpdateCameraPos(CameraDirection::LEFT, dt);
	}

	if (Keyboard::key(GLFW_KEY_SPACE)) {
		camera.UpdateCameraPos(CameraDirection::UP, dt);
	}

	if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
		camera.UpdateCameraPos(CameraDirection::DOWN, dt);
	}

	double dx = Mouse::getDX(), dy = Mouse::getDY();
	if (dx != 0 || dy != 0) {
		camera.UpdateCameraDirection(dx, dy);
	}

	double scrollDY = Mouse::getScrollDY();
	if (scrollDY != 0) {
		camera.UpdateCameraZoom(scrollDY);
	}

	mainJ.Update();
}

void displayFPSOnWindow(float frameFPS, int numOfFrames, glm::vec3 cameraPos) {
	if (countFPS > numOfFrames) {
		// this is taking alot of cpu cycles
		std::string title = "VoxelEngine - FPS: " + std::to_string(sumFPS / countFPS) +
			" | Pos(" +
			std::to_string(cameraPos.x) + ", " +
			std::to_string(cameraPos.y) + ", " +
			std::to_string(cameraPos.z) + ")";

		screen.setTitle(title.c_str());
		countFPS = 0;
		sumFPS = 0;

	}
	else
	{
		sumFPS += frameFPS;
		countFPS++;
	}

}

template<typename ChunkType>
ChunkType GenerateRampChunk()
{
	ChunkType result;
	RGBAColor color = 0x808080FF;


	for (int y = 0; y < ChunkType::Size; y++)
	{
		for (int x = 0; x < ChunkType::Size; x++)
		{
			for (int z = 0; z < ChunkType::Size; z++)
			{
				if (y < x + (ChunkType::Size / 4))
				{
					result.SetVoxel(x, y, z, color);
				}
			}
		}
	}

	return result;
}