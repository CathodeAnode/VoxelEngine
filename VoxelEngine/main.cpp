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
#include "profiler.h"
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
#include "application.h"

// Agenda for 1/5/2026 - Week
// - use logging system
// - implement multi-threading class (thread pool) to handle chunk generation, chunk meshing & chunk uploading to gpu
// - implement queue system for chunk loading to distrubite loading chunks over multiple frames (consumer-producer)
// - make gpu buffers thread-safe & implment multi-thread architecture
// - desgining & implementing gpu frustum culling to take advantage of indirect commands & caching system (compute shader)

// future TODOs:
// - terrain height map generator to generate locations from real world map data (https://tangrams.github.io/heightmapper/)
// - project file structure
// - namespacing
// - build config file for logger, chunktype, asserts
// - python script engine for terrian generation

// Futures: 
// - region-based file saving system to save voxels/chunks of world (fixed sized files for regions of the world, i.e. 1 file save a volume of 16x16x16 chunks & multiple files for regions in world)
// - lighting & SSAO
// - more voxel edit functionality
// - visuals for voxel editting functions (highlighting voxel camera is aiming at, highlighting selected volume, etc...)
// - camera editor controller
// - ECS system
// - entity rendering
// - dear imgui wrapper & engine gui

// Future Future:
// - physics system
// - audio system
// - animation system
// - pathfinding system
// - networking system

int main() 
{
	LogManager::Initialize();
	constexpr unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;


	PROFILE_BEGIN_SESSION("Startup", "../Profile-Startup.json");
	auto* app = new Application<Chunk8>(SCREEN_WIDTH, SCREEN_HEIGHT, "VoxelEngine");
	app->Init();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Runtime", "../Profile-Runtime.json");
	app->Run();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Shutdown", "../Profile-Shutdown.json");
	delete app;
	PROFILE_END_SESSION();

	LogManager::Shutdown();

	return 0;
}
