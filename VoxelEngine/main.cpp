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

#include "logger.h"
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
#include "voxel_edit.h"
#include "chunk_manager.h"
#include "scene.h"
#include "voxel_ray_cast.h"

// Agenda for 12/21/2025
// - sperate chunk implementation from header file (chunk.tpp)
// - remove friend classes from chunk & replace with func that returns raw buffer pointers
// - fix voxel ray traversal algorithm (https://mxcop.github.io/mxcop-dev/)

// future TODOs:
// - implement old chunk id system (chunk pos encoded into 64-bit int, msb specifies temp objects & voxel enities objects)
// - main/engine class for main loop and tie all classes together
// - editor-like camera controller (similar to unity)
// - debugging gui using dear imgui
// - sampler performance profiler
// - terrain perlin noise generator
// - terrain height map generator to generate locations from real world map data (https://tangrams.github.io/heightmapper/)
// - use logging system
// - implement multi-threading class (thread pool) to handle chunk generation & chunk meshing
// - implement queue system for chunk loading to distrubite loading chunks over multiple frames (consumer-producer)
// - different build types for each chunk size
// - project file structure
// - namespacing
// - design & implement gpu frustum occlustion culling archititure
// - lighting
// - SSAO

#define TIME_FUNCTION(func_call) \
    do { \
        auto start_time = std::chrono::high_resolution_clock::now(); \
        func_call; \
        auto end_time = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count(); \
        std::cout << "Function '" << #func_call << "' executed in " << duration << " microseconds." << std::endl; \
    } while (0)


// TEMPORARY Cache Currently set to 25mb (need testing to find optimal sizing)
// Memory overhead from renderer obj on CPU side is ~28.8kb for 25mb cache size (heap)
#define CACHE_PAGE_SIZE 50
#define CACHE_NUM_OF_PAGES 125000
#define AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK 3

void processInput(Screen& screen, double dt);
void displayFPSOnWindow(float frameFPS, int numOfFrames, glm::vec3 cameraPos);

template<typename ChunkType> 
ChunkType GenerateRampChunk();

int countFPS = 0;
float sumFPS = 0;

Joystick mainJ(0);

unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

Camera camera(glm::vec3(1.0f, 1.0f, 1.0f), SCREEN_WIDTH, SCREEN_HEIGHT, 0.1f, 1000.0f);

float deltaTime = 0.0f;
float fps = 0.0f;
float lastFrame = 0.0f;

Screen screen(SCREEN_WIDTH, SCREEN_HEIGHT, "VoxelEngine");


int main() 
{
	if (!screen.init()) {
		return -1;
	}

	screen.enableInputs();
	screen.toggleCursor();

	mainJ.Update();
	if (mainJ.isPresent()) {
		std::cout << mainJ.getName() << " is connected.\n";
	}
	else {
		std::cout << "Joystick not connected.\n";
	}

	constexpr unsigned int loadedChunkDistance = 9;

	std::unique_ptr<FlatChunkGeneration<Chunk8>> flatGenerator = std::make_unique<FlatChunkGeneration<Chunk8>>(0);
	std::unique_ptr<SinusoidalChunkGeneration<Chunk8>> sineGenerator = std::make_unique<SinusoidalChunkGeneration<Chunk8>>(5);
	ChunkManager8 world(camera.pos, loadedChunkDistance, std::move(flatGenerator));

	std::unique_ptr<VoxelMesher8> greedyMesher = std::make_unique<VoxelMesher8>();
	VoxelRenderer8 renderer(std::move(greedyMesher));
	renderer.Init(CACHE_NUM_OF_PAGES, CACHE_PAGE_SIZE, AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK * pow(loadedChunkDistance, 3));

	Scene8 scene(camera, world, renderer);
	VoxelEdit<Chunk8, decltype(world)> worldEdit(world, renderer);

	//camera.speed = 40.0f;

	while (screen.isOpen()) 
	{
		double currTime = glfwGetTime();
		deltaTime = currTime - lastFrame;
		fps = 1 / deltaTime;
		lastFrame = currTime;

		// process input
		processInput(screen, deltaTime);
		if (Mouse::buttonUp(MOUSE_BUTTON_LEFT))
		{
			glm::ivec3 voxelCoords;
			bool voxelHit = VoxelRayCast8::cast(Ray(camera.pos, camera.front, 10), world, voxelCoords);
			if (voxelHit)
			{
				std::cout << "Voxel hit at: " << voxelCoords.x << ", " << voxelCoords.y << ", " << voxelCoords.z << std::endl;
				worldEdit.RemoveVoxel(voxelCoords);
			}
		}

		scene.Update();
		// render
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene.Render();

		// send back buffer to front buffer
		screen.Update();
		displayFPSOnWindow(fps, 400, camera.pos);
	}

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) 
	{
		std::cerr << "GL ERROR: " << std::hex << err << std::endl;
	}

	glfwTerminate();

	return 0;
}

void processInput(Screen& screen, double dt) {
	if (Keyboard::key(GLFW_KEY_ESCAPE)) 
	{
		screen.close();
	}

	if (Keyboard::key(GLFW_KEY_W)) 
	{
		camera.UpdateCameraPos(CameraDirection::FORWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_S)) 
	{
		camera.UpdateCameraPos(CameraDirection::BACKWARD, dt);
	}

	if (Keyboard::key(GLFW_KEY_D)) 
	{
		camera.UpdateCameraPos(CameraDirection::RIGHT, dt);
	}

	if (Keyboard::key(GLFW_KEY_A)) 
	{
		camera.UpdateCameraPos(CameraDirection::LEFT, dt);
	}

	if (Keyboard::key(GLFW_KEY_SPACE)) 
	{
		camera.UpdateCameraPos(CameraDirection::UP, dt);
	}

	if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) 
	{
		camera.UpdateCameraPos(CameraDirection::DOWN, dt);
	}

	double dx = Mouse::getDX(), dy = Mouse::getDY();
	if (dx != 0 || dy != 0) 
	{
		camera.UpdateCameraDirection(dx, dy);
	}

	double scrollDY = Mouse::getScrollDY();
	if (scrollDY != 0) 
	{
		camera.UpdateCameraZoom(scrollDY);
	}

	mainJ.Update();
}

void displayFPSOnWindow(float frameFPS, int numOfFrames, glm::vec3 cameraPos) 
{
	if (countFPS > numOfFrames) 
	{
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