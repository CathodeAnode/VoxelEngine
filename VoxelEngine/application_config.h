#ifndef _VE_APPLICATION_CONFIG_H
#define _VE_APPLICATION_CONFIG_H

#include <glm/glm.hpp>
#include <thread>

struct ApplicationConfig
{
	unsigned int ScreenWidth = 800;
	unsigned int ScreenHeight = 600;
	const char* Name = "VoxelEngine";
    unsigned int ThreadWorkers = std::thread::hardware_concurrency();

    // Cache configuration
    int cachePageSize = 400;
    int cacheNumOfPages = 62500;
    int averageIndirectCmdsPerChunk = 3;

    // Chunk distances
    int loadedChunkDistance = 15;
    int renderChunkDistance = 15;

    // Starting world position
    glm::vec3 startingWorldPos = glm::vec3(1.0f);

};

#endif

