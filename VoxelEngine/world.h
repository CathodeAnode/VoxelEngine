#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>
#include <memory>

#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "voxel_renderer.h"
#include "3d_ring_buffer.h"

// TEMPORARY Cache Currently set to 25mb (need testing to find optimal sizing)
// Memory overhead from renderer obj on CPU side is ~28.8kb for 25mb cache size (heap)
#define CACHE_PAGE_SIZE 50
#define CACHE_NUM_OF_PAGES 125000
#define AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK 3

template<typename ChunkType>
class ChunkGeneratorStrategy;

// class only handles chunk generation, chunk unload/loading & chunk pooling
// NOTE: chunk generation is injected via stratgy pattern through constructor.
// chunk generation stratgies include: no generation, flat world generation, height map generation, perlin noise generation, and more if needed
template<typename ChunkType> 
class World 
{
public:
	explicit World(const glm::vec3& playerWorldCoords, unsigned int loadedChunksDistance, std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generator);
	// TODO: Create world from file
	World(const char* filePath); 

	// NOTE: assumes player cannont move faster than one chunk per frame (will likely break if player moves faster than 1 chunk per frame)
	void Update(const glm::vec3& playerWorldCoords);

	// TEMPORARY
	void UpdateRender(const glm::vec3& playerWorldCoords);
	void Render();

	inline VoxelObjectID GetChunkID(const glm::ivec3& chunkCoords);

	// Using this function will mark chunk as dirty, thus saving chunk to disk
	std::shared_ptr<ChunkType> GetChunk(const glm::ivec3& chunkCoords);
	std::shared_ptr<const ChunkType> GetChunk(const glm::ivec3& chunkCoords) const;

	// TODO
	void SaveWorld(const char* filePath);

private:
	VoxelMesher<ChunkType> m_Mesher;
	VoxelRenderer<ChunkType> m_Renderer;
	RingBuffer3D<std::shared_ptr<ChunkType>> m_LoadedChunks; // chunks loaded around player in distance of loadedChunksDistance/2 in box volume
	std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> m_ChunkGenerator;

	glm::ivec3 m_LastPlayerGridCoords;

private:
	inline std::shared_ptr<ChunkType> _LoadChunk(const glm::ivec3& chunkCoords) const;
};

typedef World<Chunk8> World8;
typedef World<Chunk16> World16;
typedef World<Chunk32> World32;

#include "world.tpp"

#endif
