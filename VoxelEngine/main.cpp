#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <glm/glm.hpp>

#include <memory>

#include "logger.h"
#include "profiler.h"

#include "chunk.h"
#include "chunk_generator_strategy.h"
#include "application.h"

// current TODOs:
// - (feat) adjusting scene's chunks to be meshed tunning parameter during runtime (increase/decrease depending on how long last frame took)
// - (perf) gpu frustum culling shared varaibles optimizations
// - (fix) rendering with chunksize 32, quad32 cannot be packed need extra 1 bit for xyzwh, 5 extra bits
// - (feat): seperation between voxel engine library & application [Note: This will allow for mutliple applications for testing and showcasing purposes]
// - (build): cmake build system with chunk size param, logging param, profiling param (current infrastructure supports this)
// - (feat): per-vertex ambient occlusion

// future TODOs:
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
// - camera editor controller
// - dear imgui wrapper & engine gui

// Future Future:
// - physics system
// - ECS https://github.com/skypjack/entt
// - character/npc model rendering (non-voxel)
// - audio system
// - animation system
// - pathfinding system
// - networking system
// - resource manager/loader https://giordi91.github.io/post/resourcesystem/


extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

using ChunkType = Chunk8;

int main() 
{
	LogManager::Initialize();
	constexpr unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

	//LogManager::GetInstance()->GetLogger(EngineSystem::RENDERER)->set_level(spdlog::level::debug);

	PROFILE_BEGIN_SESSION("Startup", "../Profile-Startup.json");

	//TODO: clean this up, so it is simpler to use. Ideally user would not have to use a unique ptr to define generation stratgy
	std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generationStratgy = std::make_unique<
		HeightmapChunkGeneration<ChunkType>>("Grand_Canyon.png", 
											800.0f // max height
			);
	//std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generationStratgy = std::make_unique<
	//	Simple3DPerlinNoiseGeneration<ChunkType>>();
	auto* app = new Application<ChunkType>(SCREEN_WIDTH, SCREEN_HEIGHT, "VoxelEngine", std::move(generationStratgy));
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
