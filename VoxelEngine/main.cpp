#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <glm/glm.hpp>

#include <memory>
#include <thread>
#include <iostream>

#include "logger.h"
#include "profiler.h"

#include "chunk.h"
#include "chunk_generator_strategy.h"
#include "application.h"
#include "application_config.h"

extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

#if defined(VE_CHUNK_TYPE_8)
using ChunkType = Chunk8;
#elif defined(VE_CHUNK_TYPE_16)
using ChunkType = Chunk16;
#elif defined(VE_CHUNK_TYPE_32)
using ChunkType = Chunk32;
#error "Chunk32 is not supported yet!"
#else
using ChunkType = Chunk8;
#endif

int main() 
{
	LogConfig logConfig = {
		.CORE = LogLevel::Info,
		.RENDERER = LogLevel::Info,
		.INPUTS = LogLevel::Info,
		.CHUNK = LogLevel::Info,
		.VOXEL_MESHER = LogLevel::Info,
		.SCENE = LogLevel::Info,
		.GPU_BUFFER = LogLevel::Info,
		.VOXEL_ENGINE = LogLevel::Info,
	};
	LogManager::Initialize(logConfig);

	PROFILE_BEGIN_SESSION("Startup", "../Profile-Startup.json");

	//TODO: clean this up, so it is simpler to use. Ideally user would not have to use a unique ptr to define generation stratgy
	std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generationStratgy = std::make_unique<
		HeightmapChunkGeneration<ChunkType>>("Grand_Canyon.png", 
											800.0f // max height
			);
	//std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generationStratgy = std::make_unique<
	//	Simple3DPerlinNoiseGeneration<ChunkType>>();

	ApplicationConfig appConfig = {
		.ScreenWidth = 800,
		.ScreenHeight = 600,
		.Name = "VoxelEngine",
		.ThreadWorkers = std::thread::hardware_concurrency(),

		// Cache configuration
		.cachePageSize = 400,
		.cacheNumOfPages = 62500,
		.averageIndirectCmdsPerChunk = 3,

		// Chunk distances
		.loadedChunkDistance = 15,
		.renderChunkDistance = 15,

		// Starting world position
		.startingWorldPos = glm::vec3(1),
	};

	auto* app = new Application<ChunkType>(appConfig, std::move(generationStratgy));
	app->Init(appConfig);
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Runtime", "../Profile-Runtime.json");
	app->Run();
	PROFILE_END_SESSION();

	PROFILE_BEGIN_SESSION("Shutdown", "../Profile-Shutdown.json");
	delete app;
	PROFILE_END_SESSION();

	//LogManager::Shutdown();

	std::cin.get(); // pause for the user to press Enter
}
