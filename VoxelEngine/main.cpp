#define VE_LOGGING_ENABLED 1
#define VE_PROFILING_ENABLED 1


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
// - (feat) gizmo to draw cube around camera of loaded chunk distance
// - (feat) adjusting scene's chunks to be meshed tunning parameter at runtime (increase/decrease depending on how long last frame took)
// - (feat) implement multi-threading class (thread pool) to handle chunk generation, chunk meshing & chunk uploading to gpu
// - (perf) gpu frustum culling shared varaibles optimizations
// - (perf) configure opengl face culling
// - (feat) complete/redesign voxel edit pipeline
// - (fix) rendering with chunksize 16, 32 (recent bug, make a single source of truth for chunk sizing)
//		- current known areas where chunk size is set to const 8:
//			1. terrian_culling compute shader
//			2. voxel math helper funcs? (note: ideally change helper functions to take size as template param rather than function param)
// - (test) write unit tests for gpu cache (test with clock policy, currently unknow if algo correct)
// - (fix)  cache clock eviction does not work?

// future TODOs:
// - terrain height map generator to generate locations from real world map data (https://tangrams.github.io/heightmapper/)
// - seperation between voxel engine lib and generation strat application, single umbrella engine header (UnityEngine-style) that has core engine component includes
// - region-based world generation. Each region maps with user-defined chunk generation strategy, and chunks are generated according to the strategy defined for that region
// - build config file for logger, chunktype, asserts
// - python script engine for terrian generation

// codebase clean up todos:
// - project file structure
// - namespacing
// - PCH file
// - Cmake /w chunk size param
// - define a macro for compiling out logging & profiling instead of using #if 1

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

	LogManager::GetInstance()->GetLogger(EngineSystem::RENDERER)->set_level(spdlog::level::debug);

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
