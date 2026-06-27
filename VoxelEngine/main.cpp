#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <glm/glm.hpp>

#include <memory>

#include "logger.h"
#include "profiler.h"

#include "chunk.h"
#include "chunk_generator_strategy.h"
#include "application.h"

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
