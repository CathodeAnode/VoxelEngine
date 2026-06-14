#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <glm/glm.hpp>

#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <memory>
#include <chrono>

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

// current TODOs:
// - (feat) adjusting scene's chunks to be meshed tunning parameter at runtime (increase/decrease depending on how long last frame took)
// - (perf) gpu frustum culling shared varaibles optimizations
// - (fix) rendering with chunksize 32, quad cannot be packed into 5 bits each for xyzwh, need extra 5 extra bits
// - (feat): seperation between voxel engine library & application [Note: This will allow for mutliple applications for testing and showcasing purposes]
// - (fix): more robust frame buffers sizing, indirect buffer, chunk position buffer, gpu chunk requests buffer
// - (build): cmake build system with chunk size param, logging param, profiling param (current infrastructure supports this)


// future TODOs:
// - terrain height map generator to generate locations from real world map data (https://tangrams.github.io/heightmapper/)
// - region-based world generation. Each region maps with user-defined chunk generation strategy, and chunks are generated according to the strategy defined for that region
// - build config file for logger, chunktype, asserts
// - python script engine for terrian generation

// codebase clean up todos:
// - project file structure
// - namespacing
// - PCH file
// - Cmake /w chunk size param
// - better logging

// Futures: 
// - region-based file saving system to save voxels/chunks of world (fixed sized files for regions of the world, i.e. 1 file save a volume of 16x16x16 chunks & multiple files for regions in world)
// - lighting & SSAO
// - more voxel edit functionality
// - visuals for voxel editting functions (highlighting voxel camera is aiming at, highlighting selected volume, etc...)
// - camera editor controller
// - dear imgui wrapper & engine gui

// Future Future:
// - physics system
// - audio system
// - animation system
// - pathfinding system
// - networking system

extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

int main() 
{
	LogManager::Initialize();
	constexpr unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

	//LogManager::GetInstance()->GetLogger(EngineSystem::VOXEL_MESHER)->set_level(spdlog::level::trace);

	PROFILE_BEGIN_SESSION("Startup", "../Profile-Startup.json");
	auto* app = new Application<Chunk8>(SCREEN_WIDTH, SCREEN_HEIGHT, "VoxelEngine");
	app->Init();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Runtime", "../Profile-Runtime.json");
	app->Run();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Shutdown", "../Profile-Shutdown.json");
	app->Shutdown();
	PROFILE_END_SESSION();

	//LogManager::Shutdown();
}
